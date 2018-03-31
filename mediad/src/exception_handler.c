/*
* Created by Liu Papillon, on Mar 31, 2018.
*/

#include <unistd.h>
#include <sys/syscall.h>
#include <osa/osa.h>
#include <mio/mio.h>
#include "exception_handler.h"
#include "config_pri.h"


static int reinitProducer(MIO_ImageManager_ProducerHandle handle)
{
    int ret;
    int isStreaming = MIO_imageManager_producerIsStreaming(handle);

    MIO_imageManager_producerStopStreaming(handle);
    MIO_imageManager_producerClose(handle);

    ret = syscall(384, 1);  // custom syscall, to reinit the whole camera module
    OSA_info("syscall 384 returned %d.\n", ret);

    ret = MIO_imageManager_producerOpen(handle);

    if (isStreaming) {
        ret = MIO_imageManager_producerStartStreaming(handle);
    }
    
    return ret;
}


static int handleCamera(MEDIAD_Exception *pException)
{   
    int ret = OSA_STATUS_OK;

    switch (pException->type)
    {
    case MEDIAD_EXCEPTION_TYPE_CAMERA_NO_IMAGE:
        if (pException->successiveErrorsCount >= MEDIAD_MAX_SUCCESSIVE_CAMERA_FAILURES_COUNT) {
            ret = reinitProducer(pException->producerHandle);
        }
        break;
    case MEDIAD_EXCEPTION_TYPE_IMAGE_MANAGER_WRITING_FAILED:
        ret = reinitProducer(pException->producerHandle);
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