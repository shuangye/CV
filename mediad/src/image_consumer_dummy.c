/*
* Created by Liu Papillon, on Dec 23, 2017.
*/

#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <osa/osa.h>
#include <dscv/dscv.h>
#include <mio/image_manager/image_manager.h>
#include "debug.h"
#include "config_pri.h"


static pthread_t                                     gConsumerTask;
static int                                           gConsumerTaskShouldQuit = 0;
static MIO_ImageManager_Handle                       gImageManagerHandle = NULL;


static void* imageConsumerMain(void *arg)
{
    int ret;
    DSCV_Frame frame;


    OSA_info("Created image producer. Process %ld, thread %ld.\n", (long)OSA_getpid(), (long)OSA_gettid());

    for (; ;) {
        if (gConsumerTaskShouldQuit) {
            break;
        }

        OSA_msleep(40);

        ret = MIO_imageManager_readFrame(gImageManagerHandle, 0, MIO_IMAGE_MANAGER_CONSUMER_ROLE_PREVIEWER, &frame);
        if (OSA_isFailed(ret)) {
            OSA_warn("Failed to read frame: %d.\n", ret);
            continue;
        }

        OSA_info("Got one frame from media memory region, index %u, type %d, size %dx%d, data @%p length %u, timestamp %llu.\n",
            frame.index, frame.type, frame.size.w, frame.size.h, frame.pData, frame.dataLen, frame.timestamp);

        MIO_imageManager_releaseFrame(gImageManagerHandle, 0, MIO_IMAGE_MANAGER_CONSUMER_ROLE_PREVIEWER, &frame);
    }


    OSA_info("Image consumer, process %ld thread %ld quiting...\n", (long)OSA_getpid(), (long)OSA_gettid());
    return NULL;
}


int MEDIAD_imageConsumerInit()
{
    int ret;
    MIO_ImageManager_Config config;


    OSA_clear(&config);
    config.isProducer = 0;
    ret = MIO_imageManager_init(&config, &gImageManagerHandle);
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to init image manager: %d.\n", ret);
        return ret;
    }

    gConsumerTaskShouldQuit = 0;

    ret = pthread_create(&gConsumerTask, NULL, imageConsumerMain, NULL);
    if (0 != ret) {
        ret = errno;
        OSA_error("Failed to create consumer task: %d.\n", ret);
        return ret;
    }

    return OSA_STATUS_OK;
}


int MEDIAD_imageConsumerDeinit()
{
    gConsumerTaskShouldQuit = 1;
    pthread_join(gConsumerTask, NULL);

    MIO_imageManager_deinit(gImageManagerHandle);
    gImageManagerHandle = NULL;

    return OSA_STATUS_OK;
}
