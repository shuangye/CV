/*
* Created by Liu Papillon, on Dec 19, 2017.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <pthread.h>
#include <osa/osa.h>
#include <dscv/dscv.h>
#include <mio/image_manager/image_manager.h>

#define MIO_IMAGE_MANAGER_PRODUCER_V4L2_FRAMES_COUNT                      4
#define MIO_IMAGE_MANAGER_PRODUCER_V4L2_PIX_FORMAT_PRINT_FOURCC(code)     OSA_info("%c%c%c%c\n",                 \
                                                                                  (char)(code),                  \
                                                                                  (char)(code >> 8),             \
                                                                                  (char)(code >> 16),            \
                                                                                  (char)(code >> 24))
#define USE_GCC_ATOMIC_OPERATION                                          OSA_True



/*
 VIDIOC_STREAMOFF: I/O returns to the same state as after calling VIDIOC_REQBUFS and can be restarted accordingly.
 */


/************************************ type definitions ************************************/


typedef struct MIO_ImageManager_Producer_V4l2_Item {
    struct v4l2_buffer               driverBuffer;
    void                            *pMappedAddr;
    size_t                           mappedLen;
    Uint32                           isUsed       : 1;
    Uint32                           padding1     : 31;
} MIO_ImageManager_Producer_V4l2_Item;


typedef struct LVC_Device {
    Char                                  devicePath[32];
    MIO_ImageManager_ProducerFormat       format;
    int                                   fd;
    unsigned int                          driverBuffersCount;  /* actual buffers count allocated in the driver */
    MIO_ImageManager_Producer_V4l2_Item   items[MIO_IMAGE_MANAGER_PRODUCER_V4L2_FRAMES_COUNT];
    Int32                                 usedItemsCount;
    Uint32                                isStreaming;
} MIO_ImageManager_Producer_V4l2Device;



/************************************ global variables ************************************/


static const Char                         *gpDevicePaths[MIO_IMAGE_MANAGER_PRODUCERS_COUNT] = { "/dev/video0", "/dev/video1" };
static const enum v4l2_buf_type            gkBufferType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
static const unsigned int                  gkMemoryType = V4L2_MEMORY_MMAP;


/************************************ local functions ************************************/


#pragma region Local Private Functions


static int mapFrameFormat(const int format)
{
    int ret = V4L2_PIX_FMT_YVU420;  /* set a default value */

    switch (format) {
    case DSCV_FRAME_TYPE_NV21:
    case DSCV_FRAME_TYPE_NV12:
        ret = V4L2_PIX_FMT_YVU420;
        break;
    case DSCV_FRAME_TYPE_YUV422:
    case DSCV_FRAME_TYPE_YUYV:
        ret = V4L2_PIX_FMT_YUYV;
        break;
    case DSCV_FRAME_TYPE_JPG:
        ret = V4L2_PIX_FMT_JPEG;
        break;
    default:
        OSA_warn("Unsupported frame format %d. Defaults to YUV420SP.\n", format);
        break;
    }

    return ret;
}


static void enumFrameIntervals(const MIO_ImageManager_Producer_V4l2Device *pDevice, const unsigned int pixelFormat, const unsigned int w, const unsigned int h)
{
    int ret;
    const int fd = pDevice->fd;
    struct v4l2_frmivalenum frameIntervalEnum;

    /* This ioctl allows applications to enumerate all frame intervals that the device supports for the given pixel format and frame size. 
       The supported pixel formats and frame sizes can be obtained by using the ioctl VIDIOC_ENUM_FMT and ioctl VIDIOC_ENUM_FRAMESIZES functions.
       arg: Pointer to a struct v4l2_frmivalenum structure that contains a pixel format and size and receives a frame interval.
    */
    OSA_clear(&frameIntervalEnum);
    frameIntervalEnum.pixel_format = pixelFormat;
    frameIntervalEnum.width = w;
    frameIntervalEnum.height = h;

    for (frameIntervalEnum.index = 0; ; ++frameIntervalEnum.index) {
        ret = ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frameIntervalEnum);
        if (0 != ret) {
            break;
        }

        OSA_info("Under pixel format %u and frame size %ux%u, supported frame intervals [%u]:\n",
            frameIntervalEnum.pixel_format, frameIntervalEnum.width, frameIntervalEnum.height, frameIntervalEnum.index);
        switch (frameIntervalEnum.type) {
        case V4L2_FRMIVAL_TYPE_DISCRETE:
            OSA_info("Discrete frame interval %u/%u.\n",
                frameIntervalEnum.discrete.numerator, frameIntervalEnum.discrete.denominator);
            break;
        case V4L2_FRMIVAL_TYPE_CONTINUOUS:
            OSA_info("Continuous frame interval [%u/%u, %u/%u].\n",
                frameIntervalEnum.stepwise.min.numerator, frameIntervalEnum.stepwise.min.denominator, frameIntervalEnum.stepwise.max.numerator, frameIntervalEnum.stepwise.max.denominator);
            break;
        case V4L2_FRMIVAL_TYPE_STEPWISE:
            OSA_info("Stepwise frame interval [%u/%u, %u/%u], step %u/%u.\n",
                frameIntervalEnum.stepwise.min.numerator, frameIntervalEnum.stepwise.min.denominator, frameIntervalEnum.stepwise.max.numerator, frameIntervalEnum.stepwise.max.denominator,
                frameIntervalEnum.stepwise.step.numerator, frameIntervalEnum.stepwise.step.denominator);
            break;
        default:
            break;
        }
    }
}


static void enumFrameSizes(const MIO_ImageManager_Producer_V4l2Device *pDevice, const unsigned int pixelFormat)
{
    int ret;
    const int fd = pDevice->fd;
    struct v4l2_frmsizeenum frameSizeEnum;

    /* This ioctl allows applications to enumerate all frame sizes (i. e. width and height in pixels) that the device supports for the given pixel format.
       The supported pixel formats can be obtained by using the ioctl VIDIOC_ENUM_FMT function.
       arg: Pointer to a struct v4l2_frmsizeenum that contains an index and pixel format and receives a frame width and height.
    */
    OSA_clear(&frameSizeEnum);
    frameSizeEnum.pixel_format = pixelFormat;
    
    for (frameSizeEnum.index = 0; ; ++frameSizeEnum.index) {
        ret = ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frameSizeEnum);
        if (0 != ret) {
            break;
        }

        OSA_info("Under pixel format %u, supported frame sizes [%u]:\n", frameSizeEnum.pixel_format, frameSizeEnum.index);
        switch (frameSizeEnum.type) {
        case V4L2_FRMSIZE_TYPE_DISCRETE:
            OSA_info("Discrete frame size discrete %ux%u.\n",
                frameSizeEnum.discrete.width, frameSizeEnum.discrete.height);
            enumFrameIntervals(pDevice, pixelFormat, frameSizeEnum.discrete.width, frameSizeEnum.discrete.height);
            break;
        case V4L2_FRMSIZE_TYPE_CONTINUOUS:
            OSA_info("Continuous frame size [%ux%u, %ux%u].\n",
                frameSizeEnum.stepwise.min_width, frameSizeEnum.stepwise.min_height, frameSizeEnum.stepwise.max_width, frameSizeEnum.stepwise.max_height);
            enumFrameIntervals(pDevice, pixelFormat, frameSizeEnum.stepwise.min_width, frameSizeEnum.stepwise.min_height);
            enumFrameIntervals(pDevice, pixelFormat, frameSizeEnum.stepwise.max_width, frameSizeEnum.stepwise.max_height);
            break;
        case V4L2_FRMSIZE_TYPE_STEPWISE:
            OSA_info("Stepwise frame size [%ux%u, %ux%u], step %ux%u.\n",
                frameSizeEnum.stepwise.min_width, frameSizeEnum.stepwise.min_height, frameSizeEnum.stepwise.max_width, frameSizeEnum.stepwise.max_height,
                frameSizeEnum.stepwise.step_width, frameSizeEnum.stepwise.step_height);
            enumFrameIntervals(pDevice, pixelFormat, frameSizeEnum.stepwise.min_width, frameSizeEnum.stepwise.min_height);
            enumFrameIntervals(pDevice, pixelFormat, frameSizeEnum.stepwise.max_width, frameSizeEnum.stepwise.max_height);
            break;
        default:
            break;
        }
    }
}


static int queryDevice(const MIO_ImageManager_Producer_V4l2Device *pDevice)
{
    int ret;
    const int fd = pDevice->fd;
    struct v4l2_capability capability;
    struct v4l2_fmtdesc desc;
    struct v4l2_queryctrl queryControl;        


    ret = ioctl(pDevice->fd, VIDIOC_QUERYCAP, &capability);
    if (0 == ret) {
        OSA_info("device %s: driver = %s, card = %s, bus info = %s, version = %u, capabilities = %#x.\n",
            pDevice->devicePath, capability.driver, capability.card, capability.bus_info, capability.version,
            capability.capabilities);
    }  
    else {
        OSA_warn("Failed to query device capabilities: %d.\n", errno);
    }
    
    for (int i = V4L2_CID_BASE; i < V4L2_CID_LASTP1; ++i) {
        OSA_clear(&queryControl);
        queryControl.id = i;
        ret = ioctl(fd, VIDIOC_QUERYCTRL, &queryControl);
        if (0 != ret) {
            continue;
        }
        OSA_info("Query control ID %u: type %u, name %s, range [%d, %d], step %d, default %d, flags %u.\n",
            i, queryControl.type, queryControl.name, queryControl.minimum, queryControl.maximum, queryControl.step, queryControl.default_value, queryControl.flags);
    }

    OSA_clear(&desc);
    desc.type = gkBufferType;
    for (desc.index = 0; ; ++desc.index) {
        ret = ioctl(fd, VIDIOC_ENUM_FMT, &desc);
        if (0 != ret) {
            break;
        }
        OSA_info("Device %s supported formats [%u]: pixel format %u, desc %s.\n", pDevice->devicePath, desc.index, desc.pixelformat, desc.description);
        enumFrameSizes(pDevice, desc.pixelformat);
    }
        
    /* VIDIOC_SUBSCRIBE_EVENT */

    return OSA_STATUS_OK;
}


static int releaseDriverBuffers(MIO_ImageManager_Producer_V4l2Device *pDevice)
{
    int ret;
    enum v4l2_buf_type type = gkBufferType;
    struct v4l2_requestbuffers req;
    unsigned int i;
    unsigned int count;
    

    /* 
      Issue A: When requesting to release driver buffers, these buffers may still being used by consumers. So wait before releasing driver buffers.
      Issue B: Calling to `releaseOneFrameToDriver` may fail with errno 16 (device or resource busy). As a result, the used buffers `count` keeps being > 0 here,
               and it spins forever in this loop. So lets set a threshold. 
      Solution to issue B is ugly, since it confilicts with the solution to issue A.
     */
    i = 0;
    while ((count = __atomic_load_n(&pDevice->usedItemsCount, __ATOMIC_RELAXED)) > 0) {
        OSA_debug("Waiting for all buffers to be released. There is/are still %u buffers.", count);
        OSA_msleep(30);
        ++i;
        if (i > 5) {
            break;
        }
    }

    /* When frame size or pixel format is changed, per-buffer size may change as well. 
    So it is necessary to re-allocate the driver buffers.
    On the other hand, when any buffer has been mapped, `VIDIOC_REQBUFS` will fail.
    */    
    for (i = 0; i < pDevice->driverBuffersCount; ++i) {
        if (NULL == pDevice->items[i].pMappedAddr) {
            continue;
        }
        ret = munmap(pDevice->items[i].pMappedAddr, pDevice->items[i].mappedLen);
        if (0 != ret) {
            OSA_warn("unmapping driver buffer %u failed with code %d: %s.\n", i, errno, strerror(errno));
        }

        pDevice->items[i].pMappedAddr = NULL;
        pDevice->items[i].mappedLen = 0;
        pDevice->items[i].isUsed = 0;
    }

    OSA_clear(&req);
    req.count               = 0;  /* free all allocated buffers */
    req.type                = gkBufferType;
    req.memory              = gkMemoryType;
    ret = ioctl(pDevice->fd, VIDIOC_REQBUFS, &req);
    if (0 != ret) {
        ret = errno;
        OSA_error("VIDIOC_REQBUFS failed with code %d.\n", ret);
        return ret;
    }

    OSA_clear(&pDevice->items);
    pDevice->driverBuffersCount = 0;
    return ret;
}


static int requestDriverBuffers(MIO_ImageManager_Producer_V4l2Device *pDevice)
{
    int ret;
    unsigned int i;
    struct v4l2_requestbuffers req;
    struct v4l2_buffer drvierBuffer;


    /* request video buffers */
    OSA_clear(&req);
    req.count               = MIO_IMAGE_MANAGER_PRODUCER_V4L2_FRAMES_COUNT;  /* actual count may be modified by the driver */
    req.type                = gkBufferType;
    req.memory              = V4L2_MEMORY_MMAP;
    ret = ioctl(pDevice->fd, VIDIOC_REQBUFS, &req);
    if (0 != ret) {
        ret = errno;
        OSA_error("VIDIOC_REQBUFS failed with %d.\n", ret);
        return ret;
    }

    if (req.count < MIO_IMAGE_MANAGER_PRODUCER_V4L2_FRAMES_COUNT) {
        OSA_warn("driver allocated insufficient count %u of buffers.\n", req.count);
        goto _failure;
    }

    pDevice->driverBuffersCount = req.count;

    for (i = 0; i < req.count; ++i) {        
        OSA_clear(&drvierBuffer);
        drvierBuffer.type        = gkBufferType;
        drvierBuffer.memory      = gkMemoryType;
        drvierBuffer.index       = i;
        ret = ioctl(pDevice->fd, VIDIOC_QBUF, &drvierBuffer);  /* enqueue buffer */
        if (0 != ret) {
            ret = errno;
            OSA_error("VIDIOC_QBUF failed with %d.\n", ret);
            goto _failure;
        }

        ret = ioctl(pDevice->fd, VIDIOC_QUERYBUF, &drvierBuffer);
        if (0 != ret) {
            ret = errno;
            OSA_error("VIDIOC_QUERYBUF failed with %d.\n", ret);
            goto _failure;
        }

        pDevice->items[i].mappedLen = drvierBuffer.length;
        pDevice->items[i].pMappedAddr = mmap(NULL, drvierBuffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, pDevice->fd, drvierBuffer.m.offset);
        if (MAP_FAILED == pDevice->items[i].pMappedAddr) {
            ret = errno;
            OSA_error("Map driver memory failed with %d.\n", ret);
            goto _failure;
        }        
    }
    
    return OSA_STATUS_OK;

_failure:
    releaseDriverBuffers(pDevice);
    return ret;
}


static int requestOneFrameFromDriver(MIO_ImageManager_Producer_V4l2Device *pDevice, DSCV_Frame *pFrame)
{
    int ret;
    struct v4l2_buffer driverBuffer;
    unsigned int index;


    OSA_debug("Will get frame from device %s\n", pDevice->devicePath);

    /* dequeue one frame */
    OSA_clear(&driverBuffer);
    driverBuffer.type = gkBufferType;
    driverBuffer.memory = gkMemoryType;
    ret = ioctl(pDevice->fd, VIDIOC_DQBUF, &driverBuffer);
    if (0 != ret) {
        ret = errno;
        OSA_debug("VIDIOC_DQBUF failed with %d.\n", ret);
        return ret;
    }

    OSA_debug("Got one frame from device %s\n", pDevice->devicePath);

    index = driverBuffer.index;
    if (index >= OSA_arraySize(pDevice->items)) {
        OSA_error("Driver gave a buffer with an expected index %u.\n", index);
        return OSA_STATUS_EAGAIN;
    }
    
    pFrame->type           = pDevice->format.frameType;
    pFrame->size           = pDevice->format.frameSize;
    pFrame->pData          = pDevice->items[index].pMappedAddr;    
    pFrame->dataLen        = driverBuffer.bytesused;
    pFrame->index          = index;
    pFrame->timestamp      = driverBuffer.timestamp.tv_sec * 1000 + driverBuffer.timestamp.tv_usec / 1000;    /* in ms */

    pDevice->items[driverBuffer.index].driverBuffer = driverBuffer;
    pDevice->items[driverBuffer.index].isUsed = 1;
    __atomic_add_fetch(&pDevice->usedItemsCount, 1, __ATOMIC_RELAXED);    /* ++pDevice->usedItemsCount; */

    OSA_debug("Got one frame from device %s, index %u, type %d, size %dx%d, data @%p length %u, timestamp %llu.\n",
        pDevice->devicePath, pFrame->index, pFrame->type, pFrame->size.w, pFrame->size.h, pFrame->pData, pFrame->dataLen, pFrame->timestamp);
    
    return OSA_STATUS_OK;
}


static int releaseOneFrameToDriver(MIO_ImageManager_Producer_V4l2Device *pDevice, const DSCV_Frame *pFrame)
{
    int ret;
    struct v4l2_buffer driverBuffer;
    unsigned int index;
    
    index = pFrame->index;
    if (index >= OSA_arraySize(pDevice->items)) {
        OSA_error("Invalid video frame ID %u.\n", index);
        return OSA_STATUS_EINVAL;
    }

    if (!pDevice->items[index].isUsed) {
        return OSA_STATUS_OK;
    }

    driverBuffer = pDevice->items[index].driverBuffer;
    ret = ioctl(pDevice->fd, VIDIOC_QBUF, &driverBuffer);
    if (0 != ret) {
        ret = errno;
        OSA_error("VIDIOC_QBUF failed with %d.\n", ret);
        return ret;
    }
    
    pDevice->items[index].isUsed = 0;
    __atomic_sub_fetch(&pDevice->usedItemsCount, 1, __ATOMIC_RELAXED);    /* --pDevice->usedItemsCount; */
    return OSA_STATUS_OK;
}


static void setStreaming(MIO_ImageManager_Producer_V4l2Device *pDevice, const Bool streaming)
{   
#if USE_GCC_ATOMIC_OPERATION
    __atomic_store_n(&pDevice->isStreaming, !!streaming, __ATOMIC_RELAXED);
#else
    pthread_rwlock_wrlock(&pDevice->lock);
    pDevice->isStreaming = !!streaming;
    pthread_rwlock_unlock(&pDevice->lock);
#endif
    OSA_debug("Device %s streaming was set to %d.\n", pDevice->devicePath, streaming);
}


#pragma endregion


/************************************ public functions ************************************/


#pragma region Public Functions


int MIO_imageManager_producerInit(const int producerId, MIO_ImageManager_ProducerHandle *pHandle)
{
    int ret;
    MIO_ImageManager_Producer_V4l2Device *pDevice = NULL;
    

    if (!OSA_isInRange(producerId, 0, OSA_arraySize(gpDevicePaths)) || NULL == pHandle) {
        return OSA_STATUS_EINVAL;
    }

    pDevice = calloc(1, sizeof(*pDevice));
    if (NULL == pDevice) {
        OSA_error("Failed to allocate memory.\n");
        return OSA_STATUS_ENOMEM;
    }

    OSA_strncpy(pDevice->devicePath, gpDevicePaths[producerId]);
    pDevice->fd = -1;    /* open as needed, thus to avoid using the device all the time */
    pDevice->usedItemsCount = 0;
    
    *pHandle = pDevice;  
    OSA_info("Inited device %s by pid %ld tid %ld.\n", pDevice->devicePath, (long)OSA_getpid(), (long)OSA_gettid());
    return OSA_STATUS_OK;
}


int MIO_imageManager_producerDeinit(MIO_ImageManager_ProducerHandle handle)
{
    MIO_ImageManager_Producer_V4l2Device *pDevice = (MIO_ImageManager_Producer_V4l2Device *)handle;


    if (NULL == pDevice) {
        return OSA_STATUS_OK;
    }
        
    MIO_imageManager_producerClose(handle);

    OSA_info("Deinited device %s by pid %ld tid %ld.\n", pDevice->devicePath, (long)OSA_getpid(), (long)OSA_gettid());
    free(pDevice);
    pDevice = NULL;
        
    return OSA_STATUS_OK;
}


int MIO_imageManager_producerOpen(MIO_ImageManager_ProducerHandle handle)
{
    int ret;
    MIO_ImageManager_Producer_V4l2Device *pDevice = (MIO_ImageManager_Producer_V4l2Device *)handle;

    if (NULL == pDevice) {
        return OSA_STATUS_EINVAL;
    }

    if (pDevice->fd >= 0) {
        return OSA_STATUS_OK;
    }

    pDevice->fd = open(pDevice->devicePath, O_RDWR, 0);
    if (pDevice->fd < 0) {
        ret = errno;
        OSA_error("Failed to open device %s.\n", pDevice->devicePath);
        return ret;
    }
    
    OSA_info("Opened device %s.\n", pDevice->devicePath);
    queryDevice(pDevice);
    return OSA_STATUS_OK;
}


int MIO_imageManager_producerClose(MIO_ImageManager_ProducerHandle handle)
{
    int ret;
    MIO_ImageManager_Producer_V4l2Device *pDevice = (MIO_ImageManager_Producer_V4l2Device *)handle;

    if (NULL == pDevice) {
        return OSA_STATUS_EINVAL;
    }

    if (pDevice->fd < 0) {
        return OSA_STATUS_OK;
    }

    MIO_imageManager_producerStopStreaming(handle);

    if (pDevice->driverBuffersCount) {
        releaseDriverBuffers(pDevice);
    }

    fsync(pDevice->fd);
    close(pDevice->fd);
    pDevice->fd = -1;
    OSA_info("Closed device %s.\n", pDevice->devicePath);
    return OSA_STATUS_OK;
}


int MIO_imageManager_producerSetFormat(MIO_ImageManager_ProducerHandle handle, const MIO_ImageManager_ProducerFormat *pFormat)
{
    int ret;
    unsigned int v4lPixelFormat;
    struct v4l2_format targetFormat;    
    MIO_ImageManager_Producer_V4l2Device *pDevice = (MIO_ImageManager_Producer_V4l2Device *)handle;
    int fd;
    int isStreaming;                                   /* backup current status */
    int driverBufferAllocated;
    

    if (NULL == pDevice || NULL == pFormat) {
        return OSA_STATUS_EINVAL;
    }

    if (pDevice->fd < 0) {
        OSA_error("Device is not opened yet.\n");
        return OSA_STATUS_EPERM;
    }

    if (0 == memcmp(&pDevice->format, pFormat, sizeof(pDevice->format))) {
        return OSA_STATUS_OK;
    }

    fd = pDevice->fd;
    isStreaming = MIO_imageManager_producerIsStreaming(handle);
    driverBufferAllocated = pDevice->driverBuffersCount > 0;

    OSA_clear(&targetFormat);
    targetFormat.type = gkBufferType;
    ret = ioctl(fd, VIDIOC_G_FMT, &targetFormat);
    if (0 != ret) {
        ret = errno;
        OSA_error("VIDIOC_G_FMT failed with code %d.\n", ret);
        return ret;
    }    

    /* stop streaming and release driver buffers before changing format */

    if (isStreaming) {
        ret = MIO_imageManager_producerStopStreaming(pDevice);
        if (OSA_isFailed(ret)) {
            OSA_error("Stopping streaming of device %s failed with %d.\n", pDevice->devicePath, ret);
            return ret;
        }
    }

    if (driverBufferAllocated) {
        ret = releaseDriverBuffers(pDevice);
        if (OSA_isFailed(ret)) {
            OSA_error("Releaseing driver buffers of device %s failed with %d.\n", pDevice->devicePath, ret);
            return ret;
        }
    }

    /* now change format */

    v4lPixelFormat = mapFrameFormat(pFormat->frameType);
    
    targetFormat.fmt.pix.width       = pFormat->frameSize.w;
    targetFormat.fmt.pix.height      = pFormat->frameSize.h;
    targetFormat.fmt.pix.pixelformat = v4lPixelFormat;    
    ret = ioctl(fd, VIDIOC_S_FMT, &targetFormat);
    if (0 != ret) {
        ret = errno;
        OSA_error("VIDIOC_S_FMT failed with %d.\n", ret);
        return ret;
    }


#ifdef OSA_DEBUG
    OSA_info("Expected pixel format: ");  MIO_IMAGE_MANAGER_PRODUCER_V4L2_PIX_FORMAT_PRINT_FOURCC(v4lPixelFormat);
    OSA_info("Actual pixel format: ");    MIO_IMAGE_MANAGER_PRODUCER_V4L2_PIX_FORMAT_PRINT_FOURCC(targetFormat.fmt.pix.pixelformat);
#endif
    
    if (targetFormat.fmt.pix.width       != pFormat->frameSize.w ||
        targetFormat.fmt.pix.height      != pFormat->frameSize.h ||
        targetFormat.fmt.pix.pixelformat != v4lPixelFormat) {
        OSA_error("Expected resolution %ux%u, format %d is not effective. Actual resolution %ux%u, format %d.\n",
            pFormat->frameSize.w, pFormat->frameSize.h, v4lPixelFormat,
            targetFormat.fmt.pix.width, targetFormat.fmt.pix.height, targetFormat.fmt.pix.pixelformat);
        return OSA_STATUS_EINVAL;
    }

    OSA_info("Set device %s frame size to %dx%d. Pixel format: ", pDevice->devicePath, pFormat->frameSize.w, pFormat->frameSize.h);
    MIO_IMAGE_MANAGER_PRODUCER_V4L2_PIX_FORMAT_PRINT_FOURCC(v4lPixelFormat);

    /* restore buffers / streaming */

    if (driverBufferAllocated) {
        ret = requestDriverBuffers(pDevice);
        if (OSA_isFailed(ret)) {
            OSA_error("Allocating driver buffers of device %s failed with %d.\n", pDevice->devicePath, ret);
            return ret;
        }
    }

    if (isStreaming) {
        ret = MIO_imageManager_producerStartStreaming(pDevice);
        if (OSA_isFailed(ret)) {
            OSA_error("Starting streaming of device %s failed with %d.\n", pDevice->devicePath, ret);
            return ret;
        }
    }

    pDevice->format = *pFormat;
    
    return ret;
}


int MIO_imageManager_producerSetFrameRate(MIO_ImageManager_ProducerHandle handle, const unsigned int frameRate)
{
    int ret;    
    MIO_ImageManager_Producer_V4l2Device *pDevice = (MIO_ImageManager_Producer_V4l2Device *)handle;
    int fd;
    struct v4l2_streamparm params;


    if (NULL == pDevice) {
        return OSA_STATUS_EINVAL;
    }

    if (pDevice->fd < 0) {
        OSA_error("Device is not opened yet.\n");
        return OSA_STATUS_EPERM;
    }

    fd = pDevice->fd;

    OSA_clear(&params);
    params.type = gkBufferType;    
    ret = ioctl(fd, VIDIOC_G_PARM, &params);
    if (0 != ret) {
        ret = errno;
        OSA_error("VIDIOC_G_PARM failed with %d.\n", ret);
        return ret;
    }

    params.parm.capture.timeperframe.numerator = 1;
    params.parm.capture.timeperframe.denominator = frameRate;
    ret = ioctl(fd, VIDIOC_S_PARM, &params);
    if (0 != ret) {
        ret = errno;
        OSA_error("VIDIOC_S_PARM failed with %d.\n", ret);
        return ret;
    }

    OSA_info("Set device %s frame rate to %u.\n", pDevice->devicePath, frameRate);
    return OSA_STATUS_OK;
}


int MIO_imageManager_producerStartStreaming(MIO_ImageManager_ProducerHandle handle)
{
    MIO_ImageManager_Producer_V4l2Device *pDevice = (MIO_ImageManager_Producer_V4l2Device *)handle;
    int ret;
    enum v4l2_buf_type type = gkBufferType;

    if (NULL == pDevice) {
        return OSA_STATUS_EINVAL;
    }

    if (pDevice->fd < 0) {
        OSA_error("Device is not opened yet.\n");
        return OSA_STATUS_EPERM;
    }

    if (MIO_imageManager_producerIsStreaming(handle)) {
        return OSA_STATUS_OK;
    }

    if (pDevice->driverBuffersCount < 1) {
        ret = requestDriverBuffers(pDevice);
        if (OSA_isFailed(ret)) {
            OSA_error("Failed to request driver buffers: %d.\n", ret);
            return ret;
        }
    }

    ret = ioctl(pDevice->fd, VIDIOC_STREAMON, &type);
    if (0 != ret) {
        ret = errno;
        OSA_error("Failed to start streaming: %d.\n", ret);
        return ret;
    }

    setStreaming(pDevice, OSA_True);
    OSA_info("Device %s started streaming.\n", pDevice->devicePath);
    return OSA_STATUS_OK;
}


int MIO_imageManager_producerStopStreaming(MIO_ImageManager_ProducerHandle handle)
{
    MIO_ImageManager_Producer_V4l2Device *pDevice = (MIO_ImageManager_Producer_V4l2Device *)handle;
    int ret;
    enum v4l2_buf_type type = gkBufferType;

    if (NULL == pDevice) {
        return OSA_STATUS_EINVAL;
    }

    if (pDevice->fd < 0) {
        OSA_error("Device is not opened yet.\n");
        return OSA_STATUS_EPERM;
    }

    if (!MIO_imageManager_producerIsStreaming(handle)) {
        return OSA_STATUS_OK;
    }

    ret = ioctl(pDevice->fd, VIDIOC_STREAMOFF, &type);
    if (0 != ret) {
        ret = errno;
        OSA_error("Failed to stop streaming: %d.\n", ret);
        return ret;
    }

    setStreaming(pDevice, OSA_False);
    OSA_info("Device %s stopped streaming.\n", pDevice->devicePath);
    return OSA_STATUS_OK;
}


int MIO_imageManager_producerIsStreaming(const MIO_ImageManager_ProducerHandle handle)
{
    int ret;
    MIO_ImageManager_Producer_V4l2Device *pDevice = (MIO_ImageManager_Producer_V4l2Device *)handle;
    if (NULL == pDevice) {
        return OSA_False;
    }
#if USE_GCC_ATOMIC_OPERATION
    ret = __atomic_load_n(&pDevice->isStreaming, __ATOMIC_RELAXED);
#else
    pthread_rwlock_rdlock(&pDevice->lock);
    ret = pDevice->isStreaming;
    pthread_rwlock_unlock(&pDevice->lock);
#endif
    return ret;
}


int MIO_imageManager_producerGetFrame(MIO_ImageManager_ProducerHandle handle, DSCV_Frame *pFrame)
{
    int ret;
    unsigned int i; 
    enum v4l2_buf_type type = gkBufferType;
    struct timeval timeout;
    fd_set fds;
    MIO_ImageManager_Producer_V4l2Device *pDevice = (MIO_ImageManager_Producer_V4l2Device *)handle;


    if (NULL == pDevice || NULL == pFrame) {
        OSA_error("Invalid parameter.\n");
        return OSA_STATUS_EINVAL;
    }

    if (!MIO_imageManager_producerIsStreaming(handle)) {
        OSA_error("Device is not streaming.\n");
        return OSA_STATUS_EPERM;
    }
    
    for (i = 0; i < OSA_arraySize(pDevice->items); ++i) {
        if (!pDevice->items[i].isUsed) {
            break;
        }
    }

    if (i >= OSA_arraySize(pDevice->items)) {
        OSA_warn("No free buffers available. Please release some frames first.\n");
        return OSA_STATUS_EPERM;
    }
        
#if 0
    /* wait and read one frame */    
    for (; ;) {
        FD_ZERO(&fds);
        FD_SET(pDevice->fd, &fds);

        /* timeout value may be modified by `select` to the time left, so set it every time */
        timeout.tv_sec = 1;  /* seconds */
        timeout.tv_usec = 0;
        ret = select(pDevice->fd + 1, &fds, NULL, NULL, &timeout);  /* test if readable */
        if (ret > 0) {
            ret = requestOneFrameFromDriver(pDevice, pFrame);            
            break;
        }
        else if (0 == ret) {  /* timeout */            
            continue;  /* if you don't break, it will stuck here. So we breakout, and try the next time. */
        }
        else {
            ret = errno;
            break;
        }
    }
#else
    ret = requestOneFrameFromDriver(pDevice, pFrame);
#endif

    return ret;
}


int MIO_imageManager_producerPutFrame(MIO_ImageManager_ProducerHandle handle, const DSCV_Frame *pFrame)
{
    MIO_ImageManager_Producer_V4l2Device *pDevice = (MIO_ImageManager_Producer_V4l2Device *)handle;

    if (NULL == pDevice || NULL == pFrame) {
        OSA_error("Invalid parameter.\n");
        return OSA_STATUS_EINVAL;
    }

    if (pDevice->fd < 0) {
        OSA_error("Device is not opened yet.\n");
        return OSA_STATUS_EPERM;
    }

    return releaseOneFrameToDriver(pDevice, pFrame);
}


#pragma endregion
