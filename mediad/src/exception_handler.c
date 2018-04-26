/*
* Created by Liu Papillon, on Mar 31, 2018.
*/

#include <unistd.h>
#include <sys/syscall.h>
#include <osa/osa.h>
#include <mio/mio.h>
#include "exception_handler.h"
#include "config_pri.h"


static int reinitProducer(MEDIAD_Exception *pException)
{
    int ret;    
    int isStreaming[MEDIAD_IMAGE_PRODUCERS_COUNT];
    size_t i;
    MIO_ImageManager_ProducerHandle producerHandle;

    
    for (i = 0; i < OSA_arraySize(pException->producerHandles); ++i) {
        producerHandle = pException->producerHandles[i];
        isStreaming[i] = MIO_imageManager_producerIsStreaming(producerHandle);
        MIO_imageManager_producerStopStreaming(producerHandle);
        MIO_imageManager_producerClose(producerHandle);
    }

    ret = syscall(384, 1);  // custom syscall, to reinit the whole camera module. The 2nd arg is ignored in the kernel.
    OSA_info("syscall 384 returned %d.\n", ret);

    /* the syscall above will re-create /dev/video* nodes, so let's delay a while. */
    OSA_msleep(1000);

    for (i = 0; i < OSA_arraySize(pException->producerHandles); ++i) {
        producerHandle = pException->producerHandles[i];
        ret = MIO_imageManager_producerOpen(producerHandle);
        if (OSA_isFailed(ret)) {
            OSA_error("Failed to open device: %d.\n", ret);
            continue;
        }

        if (isStreaming[i]) {
            ret = MIO_imageManager_producerStartStreaming(producerHandle);
            if (OSA_isFailed(ret)) {
                OSA_error("Failed to start streaming: %d.\n", ret);
                continue;
            }
        }
    }
    
    return ret;
}


static int handleCamera(MEDIAD_Exception *pException)
{   
    int ret = OSA_STATUS_OK;

    switch (pException->type)
    {
    case MEDIAD_EXCEPTION_TYPE_CAMERA_NO_IMAGE:
    case MEDIAD_EXCEPTION_TYPE_IMAGE_MANAGER_WRITING_FAILED:
        if (pException->successiveErrorsCount >= MEDIAD_MAX_SUCCESSIVE_CAMERA_FAILURES_COUNT) {
            ret = reinitProducer(pException);
        }
        break;
    default:
        OSA_warn("Exception type %d is not handled.\n", pException->type);
        ret = OSA_STATUS_EINVAL;
        break;
    }

    return ret;
}


int MEDIAD_handleException(MEDIAD_Exception *pException)
{
    int ret = OSA_STATUS_OK;

    if (NULL == pException) {
        return OSA_STATUS_EINVAL;
    }

    OSA_info("Handling exception %d.\n", pException->type);

    switch (pException->type) {
    case MEDIAD_EXCEPTION_TYPE_CAMERA_NO_IMAGE:
    case MEDIAD_EXCEPTION_TYPE_IMAGE_MANAGER_WRITING_FAILED:
        ret = handleCamera(pException);
        break;
    default:
        OSA_warn("Exception type %d is not handled.\n", pException->type);
        ret = OSA_STATUS_EINVAL;
        break;
    }

    return ret;
}