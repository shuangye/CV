#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <osa/osa.h>
#include "cvd/cvd_image_provider.h"

#define CVD_IMAGE_PROVIDER_FRAMES_COUNT                 2    /* for ping pong */


typedef enum CVD_ImageProviderFrameStatus {
    CVD_IMAGE_PROVIDER_FRAME_STATUS_EMPTY              = 0,
    CVD_IMAGE_PROVIDER_FRAME_STATUS_WRITTEN            = 1,
    CVD_IMAGE_PROVIDER_FRAME_STATUS_READ               = 2,
} CVD_ImageProviderFrameStatus;

typedef struct CVD_ImageMemDesc {
    Int32                 framesStatus[CVD_IMAGE_PROVIDER_FRAMES_COUNT];    /* of type `CVD_ImageProviderFrameStatus`. TODO: use lock mechanism */
    Uint32                rIndex;
    Uint32                wIndex;        
    Uint32                format;    /* NV12, YUYV, etc... */
    OSA_Size              size;      /* width * height */
    Uint32                frameLen;  /* in bytes, every frame has the same length for a given YUV format  */
    Uint32                maxFramesCount;
} CVD_ImageMemDesc;

typedef struct CVD_ImageMemManager {
    Uint64                magic;        
    Uint32                inited : 1;
    Uint32                padding1 : 31;
    CVD_ImageMemDesc      desc[CVD_IMAGE_PROVIDER_COUNT];
} CVD_ImageMemManager;

typedef struct CVD_ImageProvider {
    int                   isProducer;
    int                   fd;                                          /* fd of video provider device */
    CVD_ImageMemManager  *pManager;                                    /* manager of the mapped device mamory; the manager itself is at the beginning of the mapped memory region */
    void*                 imageBaseAddr[CVD_IMAGE_PROVIDER_COUNT];     /* image data area within "pManager" */
} CVD_ImageProvider;



int CVD_imageProviderCalcFrameLen(const DSCV_FrameType type, const OSA_Size size)
{
    int len = 0;

    switch (type) {
    case DSCV_FRAME_TYPE_NV21:
    case DSCV_FRAME_TYPE_NV12:
        len = size.w * size.h / 2 * 3;
        break;
    case DSCV_FRAME_TYPE_YUV422:
    case DSCV_FRAME_TYPE_YUYV:
        len = size.w * size.h * 2;
        break;
    default:
        OSA_error("The frame format %d is not implemented yet.\n", type);
        len = 0;
        break;
    }

    return len;
}


int CVD_imageProviderInit(const CVD_ImageProviderConfig *pConfig, CVD_ImageProviderHandle *pHandle)
{
    int ret;
    int i;
    size_t j;
    CVD_ImageProvider   *pProvider = NULL;
    void                *pMappedRegion = NULL;
    CVD_ImageMemManager *pManager;
    CVD_ImageMemDesc    *pDesc = NULL;
    const Uint32         kPerProviderSize = (CVD_IMAGE_PROVIDER_MEMORY_LEN - sizeof(*pManager)) / CVD_IMAGE_PROVIDER_COUNT;


    if (NULL == pConfig || NULL == pHandle) {
        OSA_error("Invalid parameter %p.\n", pHandle);
        return OSA_STATUS_EINVAL;
    }

    pProvider = calloc(1, sizeof(*pProvider));
    if (NULL == pProvider) {
        OSA_info("Failed to allocate memory for image provider instance.\n");
        return OSA_STATUS_ENOMEM;
    }
        
    pProvider->fd = open(CVD_IMAGE_PROVIDER_DEVICE_PATH, O_RDWR | O_NONBLOCK);
    if (pProvider->fd < 0) {
        ret = errno;
        OSA_error("Failed to open video device %s: %d.\n", CVD_IMAGE_PROVIDER_DEVICE_PATH, ret);
        goto _failure;
    }    

    pMappedRegion = mmap(NULL, CVD_IMAGE_PROVIDER_MEMORY_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, pProvider->fd, 0);
    if (MAP_FAILED == pMappedRegion) {
        ret = errno;
        OSA_error("Failed to map video device: %d.\n", ret);
        goto _failure;
    }

    pProvider->pManager = (CVD_ImageMemManager *)pMappedRegion;
    pProvider->isProducer = pConfig->isProducer;
    pManager = pProvider->pManager;    
    
    /* TODO: do not init if magic is already set */

    /* producer is responsible for filling info into the memory */
    if (pConfig->isProducer) {     
        OSA_clear(pManager);
        pManager->magic = CVD_IMAGE_PROVIDER_MEMORY_MAGIC;
        pManager->inited = 1;
        for (i = 0; i < CVD_IMAGE_PROVIDER_COUNT; ++i) {
            pDesc = &pManager->desc[i];
            pDesc->rIndex = 0;
            pDesc->wIndex = 0;
            pDesc->size = pConfig->frameSize;
            pDesc->format = pConfig->frameFormat;
            pDesc->frameLen = CVD_imageProviderCalcFrameLen(pConfig->frameFormat, pConfig->frameSize);

            #if 1
            pDesc->maxFramesCount = kPerProviderSize / pDesc->frameLen;
            assert(pDesc->maxFramesCount >= CVD_IMAGE_PROVIDER_COUNT);
            pDesc->maxFramesCount = CVD_IMAGE_PROVIDER_COUNT;              /* temporaryly use ping-pong */
            #endif

            for (j = 0; j < OSA_arraySize(pDesc->framesStatus); ++j) {
                pDesc->framesStatus[j] = CVD_IMAGE_PROVIDER_FRAME_STATUS_EMPTY;
            }
        }
    }
    /* consumer checks the flag and use the info filled by producer */
    else {
        if (CVD_IMAGE_PROVIDER_MEMORY_MAGIC != pManager->magic || !pManager->inited) {
            OSA_error("Image memory region is not inited yet. Maybe the camera is not opened?\n");
            ret = OSA_STATUS_EAGAIN;
            goto _failure;
        }
    }

    /* every process shall have its own virt address */
    for (i = 0; i < CVD_IMAGE_PROVIDER_COUNT; ++i) {        
        pProvider->imageBaseAddr[i] = (void *)((size_t)pMappedRegion + sizeof(*pManager) + kPerProviderSize * i);
    }
    
    *pHandle = (CVD_ImageProviderHandle)pProvider;
    OSA_info("Inited image provider instance %p with memory region %p by pid %ld tid %ld. Is producer = %d, frame format %d, size %dx%d.\n", 
        pProvider, pProvider->pManager, (long)OSA_getpid(), (long)OSA_gettid(), pConfig->isProducer, pConfig->frameFormat, pConfig->frameSize.w, pConfig->frameSize.h);
    return OSA_STATUS_OK;

_failure:
    CVD_imageProviderDeinit((CVD_ImageProviderHandle)pProvider);
    return ret;
}


int CVD_imageProviderDeinit(CVD_ImageProviderHandle handle)
{
    CVD_ImageProvider *pProvider = (CVD_ImageProvider *)handle;
        
    OSA_info("Will deinit image provider instance %p by pid %ld, tid %ld.\n", pProvider, (long)OSA_getpid(), (long)OSA_gettid());

    if (NULL == pProvider) {
        return OSA_STATUS_OK;
    }

    if (MAP_FAILED != pProvider->pManager && NULL != pProvider->pManager) {        
        pProvider->pManager->magic = 0;
        pProvider->pManager->inited = 0;
        munmap(pProvider->pManager, CVD_IMAGE_PROVIDER_MEMORY_LEN);
        pProvider->pManager = NULL;
    }            

    if (pProvider->fd >= 0) {
        close(pProvider->fd);
        pProvider->fd = -1;
    }
    
    OSA_info("Deinited image provider instance %p.\n", pProvider);
    free(pProvider);
    return OSA_STATUS_OK;
}


int CVD_imageProviderWriteFrame(CVD_ImageProviderHandle handle, const int id, const DSCV_Frame *pFrame)
{
    size_t               i, j;
    CVD_ImageProvider   *pProvider = (CVD_ImageProvider *)handle;
    CVD_ImageMemManager *pManager = NULL;
    CVD_ImageMemDesc    *pDesc = NULL;
    void                *pDest;
    Bool                 found = OSA_False;


    if (NULL == pProvider || !OSA_isInRange(id, 0, CVD_IMAGE_PROVIDER_COUNT) || NULL == pFrame) {
        OSA_error("Invalid parameters %p, %d, %p.\n", pProvider, id, pFrame);
        return OSA_STATUS_EINVAL;
    }

    pManager = pProvider->pManager;
    pDesc = &pManager->desc[id];

#if 1
    if (0 == access("/data/rk_backup/dump_yuv", F_OK)) {
        OSA_info("Will dump frame size %dx%d, length %u from camera %d.\n", pFrame->size.w, pFrame->size.h, pFrame->dataLen, id);
        char name[64];
        snprintf(name, sizeof(name), "/data/rk_backup/camera%d_%d.yuv", id, (int)time(NULL));
        FILE *fp = fopen(name, "w");
        if (NULL != fp) {
            fwrite(pFrame->pData, 1, pFrame->dataLen, fp);
            fclose(fp);
        }
    }
#endif

    if (pFrame->type != pDesc->format || pFrame->size.w != pDesc->size.w || pFrame->size.h != pDesc->size.h || pFrame->dataLen != pDesc->frameLen) {
        OSA_error("Invalid frame format %d size %dx%d, length %u, expected format %d size %dx%d length %u.\n", 
            pFrame->type, pFrame->size.w, pFrame->size.h, pFrame->dataLen, pDesc->format, pDesc->size.w, pDesc->size.h, pDesc->frameLen);
        return OSA_STATUS_EINVAL;
    }

    for (i = 0, j = pDesc->wIndex; i < OSA_arraySize(pDesc->framesStatus); ++i) {
        if (CVD_IMAGE_PROVIDER_FRAME_STATUS_READ != pDesc->framesStatus[j]) {
            found = OSA_True;
            break;
        }
        ++j;
        j %= pDesc->maxFramesCount;
    }

    if (!found) {
        OSA_info("No place to write new frames for camera %d.\n", id);
        return OSA_STATUS_OK;    /* no slot */
    }

    pDest = (void*)((size_t)(pProvider->imageBaseAddr[id]) + pDesc->frameLen * j);
    memcpy(pDest, pFrame->pData, pDesc->frameLen);    /* do not use the user-provided length, since there may be appended '0's to the YUV data */
    pDesc->framesStatus[j] = CVD_IMAGE_PROVIDER_FRAME_STATUS_WRITTEN;
    pDesc->wIndex = j + 1;
    pDesc->wIndex %= pDesc->maxFramesCount;
    return OSA_STATUS_OK;
}


int CVD_imageProviderReadFrame(CVD_ImageProviderHandle handle, const int id, DSCV_Frame *pFrame)
{
    size_t               i, j;
    CVD_ImageProvider   *pProvider = (CVD_ImageProvider *)handle;
    CVD_ImageMemManager *pManager = NULL;
    CVD_ImageMemDesc    *pDesc = NULL;
    Bool                 found = OSA_False;
    

    if (NULL == pProvider || !OSA_isInRange(id, 0, CVD_IMAGE_PROVIDER_COUNT) || NULL == pFrame) {
        OSA_error("Invalid parameters %p, %d, %p.\n", pProvider, id, pFrame);
        return OSA_STATUS_EINVAL;
    }

    pManager = pProvider->pManager;
    pDesc = &pManager->desc[id];    

    /* find fresh data */
    for (i = 0, j = pDesc->rIndex; i < OSA_arraySize(pDesc->framesStatus); ++i) {
        if (CVD_IMAGE_PROVIDER_FRAME_STATUS_WRITTEN == pDesc->framesStatus[j]) {
            found = OSA_True;
            break;
        }
        ++j;
        j %= pDesc->maxFramesCount;
    }
    
    /* find stale data */
    if (!found) {
        for (i = 0, j = pDesc->rIndex; i < OSA_arraySize(pDesc->framesStatus); ++i) {
            if (CVD_IMAGE_PROVIDER_FRAME_STATUS_READ == pDesc->framesStatus[j]) {
                found = OSA_True;
                break;
            }
            ++j;
            j %= pDesc->maxFramesCount;
        }
    }

    if (!found) {
        OSA_debug("No data from camera %d.\n", id);
        return OSA_STATUS_ENODATA;
    }

    pFrame->type = pDesc->format;
    pFrame->size = pDesc->size;
    pFrame->pData = (void*)((size_t)(pProvider->imageBaseAddr[i]) + pDesc->frameLen * j);
    pFrame->dataLen = pDesc->frameLen;
    pDesc->rIndex = j + 1;
    pDesc->rIndex %= pDesc->maxFramesCount;
    return OSA_STATUS_OK;
}
