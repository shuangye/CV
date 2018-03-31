/*
* Created by Liu Papillon, on Dec 19, 2017.
*/


#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <dscv/dscv.h>
#include <mio/image_manager/image_manager.h>
#include <mio/mio_utils.h>
#include <mio/mjpeg_decoder.h>

#define IMAGE_MANAGER_PROTECT_USED_FRAMES                         1    /* do not overwrite if a frame is being used by a consumer */


/************************************ Type Definitions ************************************/

/*
 NOTE:
 The shared media memory region:
 | H E A D E R | ---------------------- media contens ---------------------- |
 Carefully determine what can be shared in the shared memory region, and what can not. Things cannot be shared shall be placed in the image manager handle.
 */


typedef enum MIO_ImageManager_ImageStatus {
    MIO_IMAGE_MANAGER_IMAGE_STATUS_NONE               = 0,
    MIO_IMAGE_MANAGER_IMAGE_STATUS_FRESH,
    MIO_IMAGE_MANAGER_IMAGE_STATUS_STALE,
    MIO_IMAGE_MANAGER_IMAGE_STATUS_COUNT,
} MIO_ImageManager_ImageStatus;


#pragma pack(push, 4)
typedef struct MIO_ImageManager_FrameHeader {
    Uint8                       status;              /* MIO_ImageManager_ImageStatus */
    Uint8                       padding1[3];
    DSCV_Frame                  frame;
    /* frame data follows */
} MIO_ImageManager_FrameHeader;
#pragma pack(pop)


/* shall be in the shared media memory region */
typedef struct MIO_ImageManager_Distruction {
    Uint32                       rIndex;
    Uint32                       wIndex;    
    Uint32                       maxFramesCount;    /* won't change after initialization */
    pthread_rwlock_t             lock;
    Uint32                       inited : 1;
    Uint32                       padding : 31;
} MIO_ImageManager_Distruction;


/* shall be in the shared media memory region */
typedef struct MIO_ImageManager_Provider {
    Int32                             format;    /* NV12, YUYV, etc... all frames are in the same format */
    Uint32                            frameDataLen;  /* in bytes, each frame has the same length  */
    OSA_Size                          size;      /* width & height, each frame has the same width & height */    
    MIO_ImageManager_Distruction      distributions[MIO_IMAGE_MANAGER_MAX_DISTRIBUTIONS_COUNT];
    MIO_MJPEGDECODER_Handle           mjpegDecoder;
    DSCV_Frame                        decodedFrame;
} MIO_ImageManager_Provider;


/* shall be in the shared media memory region */
typedef struct MIO_ImageManager_MemHeader {
    Uint64                            magic;
    Uint32                            inited : 1;
    Uint32                            padding1 : 31;
    /* TODO: add a lock? */ 
    MIO_ImageManager_Provider         providers[MIO_IMAGE_MANAGER_PRODUCERS_COUNT];
} MIO_ImageManager_MemHeader;


/* shall be in the image manager handle */
typedef struct MEDIAD_VideoManager {
    int                                   isProducer;
    void                                 *pMemRegion;          /* mapped media memory region */
    MIO_ImageManager_MemHeader           *pHeader;             /* header within the mapped media memory region; header shall only be modified by 'producer' */    
    void                                 *pProviderBaseAddr[MIO_IMAGE_MANAGER_PRODUCERS_COUNT];    /* virtual address is on per-process bias */
    void                                 *pDistributionBaseAddr[MIO_IMAGE_MANAGER_PRODUCERS_COUNT][MIO_IMAGE_MANAGER_MAX_DISTRIBUTIONS_COUNT];
} MIO_ImageManager;



/************************************ Local Private Functions ************************************/

#ifdef _MSC_VER
#pragma region Local Private Functions
#endif // _MSC_VER


static inline size_t calcFrameLen(const MIO_ImageManager_Provider *pProvider)
{
    return sizeof(MIO_ImageManager_FrameHeader) + pProvider->frameDataLen;    /* header + content */
}


static void deinitProvider(MIO_ImageManager *pManager)
{
    MIO_ImageManager_MemHeader          *pHeader = pManager->pHeader;
    MIO_ImageManager_Provider           *pProvider = NULL;
    MIO_MJPEGDECODER_Options             decoderOptions;
    size_t                               i;

    for (i = 0; i < OSA_arraySize(pHeader->providers); ++i) {
        pProvider = &pHeader->providers[i];
        if (NULL != pProvider->decodedFrame.pData) {
            free(pProvider->decodedFrame.pData);
            pProvider->decodedFrame.pData = NULL;
        }
        MIO_MJPEGDECODER_deinit(pProvider->mjpegDecoder);
    }
}


static int initProvider(MIO_ImageManager *pManager, const MIO_ImageManager_Config *pConfig)
{
    MIO_ImageManager_MemHeader          *pHeader = pManager->pHeader;
    MIO_ImageManager_Provider           *pProvider = NULL;
    MIO_MJPEGDECODER_Options             decoderOptions;
    size_t                               i;
    int                                  ret = OSA_STATUS_OK;

    decoderOptions.frameSize = pConfig->frameSize;

    for (i = 0; i < OSA_arraySize(pHeader->providers); ++i) {
        pProvider = &pHeader->providers[i];
        pProvider->format = pConfig->frameFormat;
        pProvider->size = pConfig->frameSize;
        pProvider->frameDataLen = MIO_UTL_calcFrameDataLen((DSCV_FrameType)pConfig->frameFormat, pConfig->frameSize);

        if (pConfig->frameFormat == DSCV_FRAME_TYPE_JPG) {
            ret = MIO_MJPEGDECODER_init(&decoderOptions, &pProvider->mjpegDecoder);
            if (OSA_isFailed(ret)) {
                OSA_error("Failed to init MJPEG decoder: %d.\n", ret);
                goto _failure;
            }
            ret = MIO_MJPEGDECODER_decodedFrameDataLen(pProvider->mjpegDecoder, &pProvider->decodedFrame.dataLen);
            if (OSA_isFailed(ret)) {
                OSA_error("Failed to determine decoded frame data length: %d.\n", ret);
                goto _failure;
            }
            pProvider->decodedFrame.pData = malloc(pProvider->decodedFrame.dataLen);
            if (NULL == pProvider->decodedFrame.pData) {
                ret = OSA_STATUS_ENOMEM;
                goto _failure;
            }
        }
    }

    return ret;

_failure:
    deinitProvider(pManager);
    return ret;
}


static Bool checkConfig(const MIO_ImageManager *pManager)
{
    MIO_ImageManager_MemHeader          *pHeader = pManager->pHeader;
    MIO_ImageManager_Provider           *pProvider = NULL;
    MIO_ImageManager_Distruction        *pDistribution = NULL;
    size_t                               i, j;
    Bool                                 isValid = OSA_True;
    
    for (i = 0; i <  OSA_arraySize(pHeader->providers); ++i) {
        pProvider = &pHeader->providers[i];
        OSA_info("Image provider %d base addr %p, format %d, size %dx%d, data length %u.\n",
            i, pManager->pProviderBaseAddr[i], pProvider->format, pProvider->size.w, pProvider->size.h, pProvider->frameDataLen);

        for (j = 0; j < OSA_arraySize(pProvider->distributions); ++j) {
            pDistribution = &pProvider->distributions[j];
            OSA_info("Image provider %d distribution %d base addr %p, max frames count %u.\n",
                i, j, pManager->pDistributionBaseAddr[i][j], pDistribution->maxFramesCount);
            if (pDistribution->maxFramesCount <= 1) {
                OSA_error("The memory region can contain only %u frame(s). it is too small to support %d producers, %d distributions; image format %d, size %dx%d, data length %d.\n",
                    pDistribution->maxFramesCount, MIO_IMAGE_MANAGER_PRODUCERS_COUNT, MIO_IMAGE_MANAGER_MAX_DISTRIBUTIONS_COUNT, 
                    pProvider->format, pProvider->size.w, pProvider->size.h, pProvider->frameDataLen);
                isValid = OSA_False;
            }
        }
    }

    return isValid;
}


#ifdef _MSC_VER
#pragma endregion
#endif // _MSC_VER


/************************************ Global Public Functions ************************************/

#ifdef _MSC_VER
#pragma region Global Public Functions
#endif // _MSC_VER

int MIO_imageManager_init(const MIO_ImageManager_Config *pConfig, MIO_ImageManager_Handle *pHandle)
{
    int ret;
    size_t i, j;
    MIO_ImageManager                    *pManager = NULL;
    MIO_ImageManager_MemHeader          *pHeader = NULL;
    MIO_ImageManager_Provider           *pProvider = NULL;
    MIO_ImageManager_Distruction        *pDistribution = NULL;
    Iptr                                 producersStartAddr = 0;
    const size_t                         kPerProducerMemLen = (MIO_IMAGE_MANAGER_MEM_LEN - sizeof(*pHeader)) / MIO_IMAGE_MANAGER_PRODUCERS_COUNT;  /* equal division */
    const size_t                         kPerDistributionMemLen = kPerProducerMemLen / MIO_IMAGE_MANAGER_MAX_DISTRIBUTIONS_COUNT;           /* equal division */
    pthread_rwlockattr_t                 rwlockAttr;
    

    if (NULL == pConfig || NULL == pHandle) {
        OSA_error("Invalid parameter %p.\n", pHandle);
        return OSA_STATUS_EINVAL;
    }

    pManager = calloc(1, sizeof(*pManager));
    if (NULL == pManager) {
        OSA_info("Failed to allocate memory for image manager.\n");
        return OSA_STATUS_ENOMEM;
    }

    ret = MIO_imageManager_memInit(&pManager->pMemRegion);
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to get video memory region: %d.\n", ret);
        goto _failure;
    }

    /* header is at the beginning of the memory region */
    pManager->pHeader = (MIO_ImageManager_MemHeader *)pManager->pMemRegion;
    pHeader = pManager->pHeader;

#if 0    /* force init. test only */
    if (pConfig->isProducer) {
        OSA_clear(pHeader);
        memset(pManager->pMemRegion, 0, MIO_IMAGE_MANAGER_MEM_LEN);
    }
#endif

    pManager->isProducer = pConfig->isProducer;

    /* producer is responsible for initializing this memory region */
    if (pManager->isProducer) {
        if (MIO_IMAGE_MANAGER_MEM_MAGIC != pHeader->magic || !pHeader->inited) {
            memset(pManager->pMemRegion, 0, MIO_IMAGE_MANAGER_MEM_LEN);
            ret = initProvider(pManager, pConfig);
            if (OSA_isFailed(ret)) {
                goto _failure;
            }
            pHeader->magic = MIO_IMAGE_MANAGER_MEM_MAGIC;
            pHeader->inited = 1;
        }
    }
    else {
        /* consumer checks if the memory region is inited or not */
        if (MIO_IMAGE_MANAGER_MEM_MAGIC != pHeader->magic || !pHeader->inited) {
            OSA_error("Image memory region is not inited yet. It should be inited by producer first.\n");
            ret = OSA_STATUS_EAGAIN;
            goto _failure;
        }
        else {
            OSA_info("Memory region is inited by a producer.\n");
        }
    }

    /* virtual address is on per-process bias, so every process shall calc its own virtual addresses */
    producersStartAddr = (size_t)pManager->pMemRegion + sizeof(*pHeader);
    for (i = 0; i <  OSA_arraySize(pHeader->providers); ++i) {
        pProvider = &pHeader->providers[i];
        pManager->pProviderBaseAddr[i] = (void *)(producersStartAddr + kPerProducerMemLen * i);

        for (j = 0; j < OSA_arraySize(pProvider->distributions); ++j) {
            pDistribution = &pProvider->distributions[j];
            pManager->pDistributionBaseAddr[i][j] = (void *)((Iptr)pManager->pProviderBaseAddr[i] + kPerDistributionMemLen * j);

            /* only producer can do setup jobs */
            if (pManager->isProducer) {
                pDistribution->maxFramesCount = kPerDistributionMemLen / calcFrameLen(pProvider);
                
                pthread_rwlockattr_init(&rwlockAttr);
                pthread_rwlockattr_setpshared(&rwlockAttr, PTHREAD_PROCESS_SHARED);
                pthread_rwlock_init(&pDistribution->lock, &rwlockAttr);
                pthread_rwlockattr_destroy(&rwlockAttr);

                pDistribution->inited = 1;
            }
        }

        MIO_imageManager_reset(pManager, i);
    }
    
    if (!checkConfig(pManager)) {
        goto _failure;
    }
    
    *pHandle = (MIO_ImageManager_Handle)pManager;
    OSA_info("Inited image provider instance %p with memory region %p by pid %ld tid %ld. Is producer = %d, frame format %d, size %dx%d.\n", 
        pManager, pManager->pMemRegion, (long)OSA_getpid(), (long)OSA_gettid(), pConfig->isProducer, pConfig->frameFormat, pConfig->frameSize.w, pConfig->frameSize.h);
    return OSA_STATUS_OK;

_failure:
    MIO_imageManager_deinit((MIO_ImageManager_Handle)pManager);
    return ret;
}


int MIO_imageManager_deinit(MIO_ImageManager_Handle handle)
{
    MIO_ImageManager                    *pManager = (MIO_ImageManager *)handle;
    MIO_ImageManager_MemHeader          *pHeader = NULL;
    MIO_ImageManager_Provider           *pProvider = NULL;
    MIO_ImageManager_Distruction        *pDistribution = NULL;
    size_t                               i, j;


    OSA_info("Will deinit image manager instance %p by pid %ld, tid %ld.\n", pManager, (long)OSA_getpid(), (long)OSA_gettid());

    if (NULL == pManager) {
        return OSA_STATUS_OK;
    }

    pHeader = pManager->pHeader;
    
    if (pManager->isProducer) {
        for (i = 0; i <  OSA_arraySize(pHeader->providers); ++i) {
            pProvider = &pHeader->providers[i];
            for (j = 0; j < OSA_arraySize(pProvider->distributions); ++j) {
                pDistribution = &pProvider->distributions[j];
                if (pDistribution->inited) {                    
                    pthread_rwlock_wrlock(&pDistribution->lock);
                    pDistribution->rIndex = 0;
                    pDistribution->wIndex = 0;
                    pDistribution->maxFramesCount = 0;
                    pDistribution->inited = 0;
                    pthread_rwlock_unlock(&pDistribution->lock);
                    pthread_rwlock_destroy(&pDistribution->lock);
                }
            }
        }

        deinitProvider(pManager);

        pHeader->magic = 0;
        pHeader->inited = 0;
    }

    OSA_info("Deinited image provider instance %p; magic %llu, inited %u.\n", pManager, pHeader->magic, pHeader->inited);

    MIO_imageManager_memDeinit(pManager->pMemRegion);
    pManager = NULL;
    return OSA_STATUS_OK;
}


int MIO_imageManager_reset(MIO_ImageManager_Handle handle, const int producerId)
{
    MIO_ImageManager                    *pManager = (MIO_ImageManager *)handle;
    MIO_ImageManager_Provider           *pProvider = NULL;
    MIO_ImageManager_Distruction        *pDistribution = NULL;
    MIO_ImageManager_FrameHeader        *pFrameHeader = NULL;
    Iptr                                 baseAddr;
    size_t                               i, j;

    if (NULL == pManager || !OSA_isInRange(producerId, 0, MIO_IMAGE_MANAGER_PRODUCERS_COUNT)) {
        return OSA_STATUS_EINVAL;
    }

    pProvider = &pManager->pHeader->providers[producerId];

    for (j = 0; j < OSA_arraySize(pProvider->distributions); ++j) {
        pDistribution = &pProvider->distributions[j];
        baseAddr = (Iptr)(pManager->pDistributionBaseAddr[producerId][j]);    /* keep in mind: all virt addr must be calculated on per-process bias */

        pthread_rwlock_wrlock(&pDistribution->lock);
        for (i = 0; i < pDistribution->maxFramesCount; ++i) {
            pFrameHeader = (MIO_ImageManager_FrameHeader *)(baseAddr + calcFrameLen(pProvider) * i);
            pFrameHeader->status = MIO_IMAGE_MANAGER_IMAGE_STATUS_NONE;
        }
        pDistribution->rIndex = 0;
        pDistribution->wIndex = 0;
        pthread_rwlock_unlock(&pDistribution->lock);
    }

    return OSA_STATUS_OK;
}


int MIO_imageManager_writeFrame(MIO_ImageManager_Handle handle, const int producerId, const DSCV_Frame *pFrame)
{
    size_t                               i, j;
    MIO_ImageManager                    *pManager = (MIO_ImageManager *)handle;
    MIO_ImageManager_Provider           *pProvider = NULL;
    MIO_ImageManager_Distruction        *pDistribution = NULL;
    MIO_ImageManager_FrameHeader        *pFrameHeader = NULL;
    DSCV_Frame                          *pTargetFrame = (DSCV_Frame *)pFrame;
    void                                *pFrameData;
    Iptr                                 baseAddr;
    Bool                                 foundAnSlot;
    size_t                               writingPosition;
    int                                  ret;
    

    if (NULL == pManager || !OSA_isInRange(producerId, 0, MIO_IMAGE_MANAGER_PRODUCERS_COUNT) || NULL == pFrame) {
        OSA_error("Invalid parameters %p, %d, %p.\n", pManager, producerId, pFrame);
        return OSA_STATUS_EINVAL;
    }
    
#if 0
    if (0 == access("/data/rk_backup/dump_provider", F_OK)) {
        OSA_info("Will dump frame size %dx%d, length %u from camera %d.\n", pFrame->size.w, pFrame->size.h, pFrame->dataLen, producerId);
        char name[64];
        snprintf(name, sizeof(name), "/data/rk_backup/camera%d_%d.camera", producerId, (int)time(NULL));
        FILE *fp = fopen(name, "w");
        if (NULL != fp) {
            fwrite(pFrame->pData, 1, pFrame->dataLen, fp);
            fclose(fp);
        }
    }
#endif

    pProvider = &pManager->pHeader->providers[producerId];

    if (pFrame->type != pProvider->format || pFrame->size.w != pProvider->size.w || pFrame->size.h != pProvider->size.h) {
        OSA_error("Invalid frame format %d size %dx%d, expected format %d size %dx%d.\n",
            pFrame->type, pFrame->size.w, pFrame->size.h, pProvider->format, pProvider->size.w, pProvider->size.h);
        return OSA_STATUS_EINVAL;
    }

    if (DSCV_FRAME_TYPE_JPG == pProvider->format) {
        if (pFrame->dataLen > pProvider->frameDataLen) {
            OSA_error("Frame length %u for frame format JPG exceeds the max length %u.\n", pFrame->dataLen, pProvider->frameDataLen);
            return OSA_STATUS_EINVAL;
        }

        ret = MIO_MJPEGDECODER_decodeToNV12(pProvider->mjpegDecoder, pFrame, &pProvider->decodedFrame);
        if (OSA_isFailed(ret)) {
            OSA_error("Failed to convert MJPEG to NV12 from provider %d: %d.\n", producerId, ret);
            return ret;
        }
        pTargetFrame = &pProvider->decodedFrame;
        OSA_info("Decoded JPG frame %d from provider %d.\n", pFrame->index, producerId);

#if 0
        static int counter = 0;
        if (counter % 64 == 0) {
            if (0 == access("/data/rk_backup/dump_decoder", F_OK)) {
                OSA_info("Will dump frame size %dx%d, length %u from camera %d.\n", pFrame->size.w, pFrame->size.h, pFrame->dataLen, producerId);
                char name[64];
                snprintf(name, sizeof(name), "/data/rk_backup/camera%d_%d.decoder", producerId, (int)time(NULL));
                FILE *fp = fopen(name, "w");
                if (NULL != fp) {
                    fwrite(pTargetFrame->pData, 1, pTargetFrame->dataLen, fp);
                    fclose(fp);
                }
            }
        }
        ++counter;
#endif
    }
    else {
        if (pFrame->dataLen != pProvider->frameDataLen) {
            OSA_error("Invalid frame length %u for frame format %d, expected length %u.\n", pFrame->dataLen, pFrame->type, pProvider->frameDataLen);
            return OSA_STATUS_EINVAL;
        }
    }

    /* write to all distributions */
    for (j = 0; j < OSA_arraySize(pProvider->distributions); ++j) {
        pDistribution = &pProvider->distributions[j];
        baseAddr = (Iptr)(pManager->pDistributionBaseAddr[producerId][j]);    /* keep in mind: all virt addr must be calculated on per-process bias */

        pthread_rwlock_wrlock(&pDistribution->lock);
        writingPosition = pDistribution->wIndex;    /* get a copy */
        
#if IMAGE_MANAGER_PROTECT_USED_FRAMES 
        /* firstly, try to write to empty frames */
        foundAnSlot = OSA_False;
        for (i = writingPosition; i < pDistribution->maxFramesCount; ++i) {
            pFrameHeader = (MIO_ImageManager_FrameHeader *)(baseAddr + calcFrameLen(pProvider) * i);
            if (MIO_IMAGE_MANAGER_IMAGE_STATUS_NONE == pFrameHeader->status) {
                writingPosition = i;
                foundAnSlot = OSA_True;
                break;
            }
        }
        if (!foundAnSlot) {
            /* if no empty slot available, try to overwrite old unused frames */
            for (i = writingPosition; i < pDistribution->maxFramesCount; ++i) {
                pFrameHeader = (MIO_ImageManager_FrameHeader *)(baseAddr + calcFrameLen(pProvider) * i);
                if (MIO_IMAGE_MANAGER_IMAGE_STATUS_FRESH == pFrameHeader->status) {
                    writingPosition = i;
                    foundAnSlot = OSA_True;
                    break;
                }
            }
        }
        if (!foundAnSlot) {
#if 0
            /* do not overwrite analyzer's memory, but previewer's memory can be overwritten. */
            /* TODO: tweak read and release functions accordingly */
            if (MIO_IMAGE_MANAGER_CONSUMER_ROLE_ANALYZER == j) {
                pthread_rwlock_unlock(&pDistribution->lock);
                OSA_info("Producer %d all frames are used by consumer %u, and will not overwrite there.\n", producerId, j);
                continue;
            }
            else {
                writingPosition = pDistribution->wIndex;
            }
#else
            pthread_rwlock_unlock(&pDistribution->lock);
            OSA_info("Producer %d all frame buffers are being used by consumer %u, and will not overwrite there.\n", producerId, j);
            continue;
#endif
        }
#else
        /* check if the circular buffer is full */
        if ((pDistribution->wIndex + 1) % pDistribution->maxFramesCount == pDistribution->rIndex) {
            return OSA_STATUS_ENOMEM;
        }
#endif
        
        pFrameHeader = (MIO_ImageManager_FrameHeader *)(baseAddr + calcFrameLen(pProvider) * writingPosition);
        pFrameData = (void *)((Iptr)pFrameHeader + sizeof(*pFrameHeader));    /* data follows immediately after the header */
        pFrameHeader->frame = *pTargetFrame;
        pFrameHeader->frame.pData = pFrameData;
        pFrameHeader->frame.index = writingPosition;
        OSA_debug("Producer %d distribution %d frame index %u, copying %p -> %p with len %u. Dist base %p, write index %u\n", 
            producerId, j, pTargetFrame->index, pTargetFrame->pData, pFrameData, pTargetFrame->dataLen, (void *)baseAddr, writingPosition);
        memcpy(pFrameData, pTargetFrame->pData, pTargetFrame->dataLen);

#if 0
        static int counter = 0;
        if (counter % 64 == 0) {
            if (0 == access("/data/rk_backup/dump_dist", F_OK)) {
                OSA_info("Will dump frame size %dx%d, length %u from camera %d.\n", pFrame->size.w, pFrame->size.h, pFrame->dataLen, producerId);
                char name[64];
                snprintf(name, sizeof(name), "/data/rk_backup/camera%d_%d.dist", producerId, (int)time(NULL));
                FILE *fp = fopen(name, "w");
                if (NULL != fp) {
                    fwrite(pFrameData, 1, pTargetFrame->dataLen, fp);
                    fclose(fp);
                }
            }
        }
        ++counter;
#endif
                
        ++writingPosition;
        writingPosition %= pDistribution->maxFramesCount;
        pDistribution->wIndex = writingPosition;
        pFrameHeader->status = MIO_IMAGE_MANAGER_IMAGE_STATUS_FRESH;

        pthread_rwlock_unlock(&pDistribution->lock);

        OSA_debug("Producer %d distribution %u wrote one frame. Now the write index = %u.\n", producerId, j, writingPosition);
    }

    return OSA_STATUS_OK;
}


int MIO_imageManager_readFrame(MIO_ImageManager_Handle handle, const int producerId, const int consumerRole, DSCV_Frame *pFrame)
{
    MIO_ImageManager                    *pManager = (MIO_ImageManager *)handle;
    MIO_ImageManager_Provider           *pProvider = NULL;
    MIO_ImageManager_Distruction        *pDistribution = NULL;
    MIO_ImageManager_FrameHeader        *pFrameHeader = NULL;
    Iptr                                 baseAddr;
    Bool                                 foundAnSlot;
    Uint32                               rIndex, wIndex;
    size_t                               readingPosition;
    size_t                               i;
    

    if (NULL == pManager || !OSA_isInRange(producerId, 0, MIO_IMAGE_MANAGER_PRODUCERS_COUNT) || !OSA_isInRange(consumerRole, 0, MIO_IMAGE_MANAGER_MAX_DISTRIBUTIONS_COUNT) || NULL == pFrame) {
        OSA_error("Invalid parameters %p, %d, %d, %p.\n", pManager, producerId, consumerRole, pFrame);
        return OSA_STATUS_EINVAL;
    }

    pProvider = &pManager->pHeader->providers[producerId];
    pDistribution = &pProvider->distributions[consumerRole];
    baseAddr = (Iptr)(pManager->pDistributionBaseAddr[producerId][consumerRole]);

    pthread_rwlock_rdlock(&pDistribution->lock);
    readingPosition = pDistribution->rIndex;
    rIndex = pDistribution->rIndex;
    wIndex = pDistribution->wIndex;
    
#if IMAGE_MANAGER_PROTECT_USED_FRAMES 
    /* find a fresh frame */    
    foundAnSlot = OSA_False;
    for (i = readingPosition; i < pDistribution->maxFramesCount; ++i) {
        pFrameHeader = (MIO_ImageManager_FrameHeader *)(baseAddr + calcFrameLen(pProvider) * i);
        if (MIO_IMAGE_MANAGER_IMAGE_STATUS_FRESH == pFrameHeader->status) {
            readingPosition = i;
            foundAnSlot = OSA_True;
            break;
        }
    }
    if (!foundAnSlot) {
        pthread_rwlock_unlock(&pDistribution->lock);
        OSA_debug("Producer %d consumer %d, no fresh frames available.\n", producerId, consumerRole);
        return OSA_STATUS_ENODATA;
    }
#else
    /* check if the circular buffer is empty */
    if (rIndex == wIndex) {
        pthread_rwlock_unlock(&pDistribution->lock);
        OSA_info("Buffer is empty. Producer %d consumer %d, read index %u, write index %u.\n", producerId, consumerRole, rIndex, wIndex);
        return OSA_STATUS_ENODATA;
    }
#endif

    pthread_rwlock_unlock(&pDistribution->lock);
    
    pFrameHeader = (MIO_ImageManager_FrameHeader *)(baseAddr + calcFrameLen(pProvider) * readingPosition);
        
    pthread_rwlock_wrlock(&pDistribution->lock);
    *pFrame = pFrameHeader->frame;
    pFrame->pData = (void *)((Iptr)pFrameHeader + sizeof(*pFrameHeader));    /* data follows immediately after the header */
    ++readingPosition;
    readingPosition %= pDistribution->maxFramesCount;
    pDistribution->rIndex = readingPosition;
    pFrameHeader->status = MIO_IMAGE_MANAGER_IMAGE_STATUS_STALE;
    pthread_rwlock_unlock(&pDistribution->lock);

#if 0
    static int counter = 0;
    if (counter % 64 == 0) {
        if (0 == access("/data/rk_backup/dump_read", F_OK)) {
            OSA_info("Will dump frame size %dx%d, length %u from camera %d.\n", pFrame->size.w, pFrame->size.h, pFrame->dataLen, producerId);
            char name[64];
            snprintf(name, sizeof(name), "/data/rk_backup/camera%d_%d.read", producerId, (int)time(NULL));
            FILE *fp = fopen(name, "w");
            if (NULL != fp) {
                fwrite(pFrame->pData, 1, pFrame->dataLen, fp);
                fclose(fp);
            }
        }
    }
    ++counter;
#endif
    
    OSA_debug("Provided frame index %u at %p from producer %d to consumer %d.\n", pFrame->index, pFrame->pData, producerId, consumerRole);
    return OSA_STATUS_OK;
}


int MIO_imageManager_releaseFrame(MIO_ImageManager_Handle handle, const int producerId, const int consumerRole, const DSCV_Frame *pFrame)
{
    MIO_ImageManager                    *pManager = (MIO_ImageManager *)handle;
    MIO_ImageManager_Provider           *pProvider = NULL;
    MIO_ImageManager_Distruction        *pDistribution = NULL;
    MIO_ImageManager_FrameHeader        *pFrameHeader = NULL;
    Iptr                                 baseAddr;


    if (NULL == pManager || !OSA_isInRange(producerId, 0, MIO_IMAGE_MANAGER_PRODUCERS_COUNT) || !OSA_isInRange(consumerRole, 0, MIO_IMAGE_MANAGER_MAX_DISTRIBUTIONS_COUNT) || NULL == pFrame) {
        OSA_error("Invalid parameters %p, %d, %d, %p.\n", pManager, producerId, consumerRole, pFrame);
        return OSA_STATUS_EINVAL;
    }
        
    pProvider = &pManager->pHeader->providers[producerId];
    pDistribution = &pProvider->distributions[consumerRole];

    if (!OSA_isInRange(pFrame->index, 0, pDistribution->maxFramesCount)) {
        OSA_error("Producer %d consumer role %d. Frame to be released has an invalid index %u.\n", producerId, consumerRole, pFrame->index);
        return OSA_STATUS_EINVAL;
    }

    baseAddr = (Iptr)(pManager->pDistributionBaseAddr[producerId][consumerRole]);
    pFrameHeader = (MIO_ImageManager_FrameHeader *)(baseAddr + calcFrameLen(pProvider) * pFrame->index);

    pthread_rwlock_wrlock(&pDistribution->lock);
    pFrameHeader->status = MIO_IMAGE_MANAGER_IMAGE_STATUS_NONE;
    pthread_rwlock_unlock(&pDistribution->lock);

    OSA_debug("Consumer %d with pid %ld tid %ld released frame of index %u to producer %d.\n", consumerRole, (long)OSA_getpid(), (long)OSA_gettid(), pFrame->index, producerId);
    return OSA_STATUS_OK;
}

#ifdef _MSC_VER
#pragma endregion
#endif // _MSC_VER
