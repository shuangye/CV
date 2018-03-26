/*
* Created by Liu Papillon, on Dec 19, 2017.
*/

#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <osa/osa.h>
#include <dscv/dscv.h>
#include <mio/image_manager/image_manager.h>
#include <mediad/mediad_config.h>
#include "mediad_pri.h"
#include "debug.h"
#include "command.h"
#include "config_pri.h"


MIO_ImageManager_ProducerHandle                   MEDIAD_gImageProducerHandles[MEDIAD_IMAGE_PRODUCERS_COUNT];
MIO_ImageManager_Handle                           MEDIAD_gImageManagerHandle;


static pthread_t                                  gImageProducerTasks[MEDIAD_IMAGE_PRODUCERS_COUNT];    /* this program is driven by the producer */
static Bool                                       gImageProducerTasksShouldQuit[MEDIAD_IMAGE_PRODUCERS_COUNT];


static void * MEDIAD_imageProducerMain(void *pArg)
{
    int ret;
    unsigned int i = 0;
    unsigned int successiveFailuresCount = 0;
    unsigned int framesCount = 0;
    const int kProducerId = (int)pArg;
    const MIO_ImageManager_ProducerHandle producerHandle = MEDIAD_gImageProducerHandles[kProducerId];
    DSCV_Frame frame;
    

    OSA_info("Created image producer %d. Process %ld, thread %ld.\n", kProducerId, (long)OSA_getpid(), (long)OSA_gettid());
        
    for (; ;) {
        if (gImageProducerTasksShouldQuit[kProducerId]) {
            break;
        }

        OSA_msleep(30);

        if (!MIO_imageManager_producerIsStreaming(producerHandle)) {
            ++i;
            if (0 == i % 128) {
                /* Give debugger a hint, but do not be too frequent. */
                OSA_info("Producer %d is not streaming.\n", kProducerId);
            }
            continue;
        }

        ret = MIO_imageManager_producerGetFrame(producerHandle, &frame);
        if (OSA_isFailed(ret)) {
            ++successiveFailuresCount;
            if (successiveFailuresCount >= MEDIAD_MAX_SUCCESSIVE_CAMERA_FAILURES_COUNT) {
                system("reboot");  /* TODO: this is a bad design, because handling camera failure is not the responsibility of thid module */ 
            }

            if (OSA_STATUS_EAGAIN == ret) {
                OSA_debug("Try to get frame from producer %d next time.\n", kProducerId);
                continue;
            }
        
            OSA_warn("Failed to get frame from image producer %d: %d.\n", kProducerId, ret);
            continue;
        }
        
        OSA_debug("Producer %d got frame %u. Counter = %u.\n", kProducerId, frame.index, framesCount);

#ifdef OSA_DEBUG
        Char _label[64];
        snprintf(_label, sizeof(_label), "DumpFrame_%d", kProducerId);
        MEDIAD_debugDumpData(_label, frame.pData, frame.dataLen);
#endif

        ret = MIO_imageManager_writeFrame(MEDIAD_gImageManagerHandle, kProducerId, &frame);
        if (OSA_isFailed(ret)) {
            ++successiveFailuresCount;
            if (successiveFailuresCount >= MEDIAD_MAX_SUCCESSIVE_CAMERA_FAILURES_COUNT) {
                system("reboot");  /* TODO: this is a bad design, because handling camera failure is not the responsibility of thid module */ 
            }

            OSA_warn("Failed to write frame %u from producer %d to memory: %d.\n", frame.index, kProducerId, ret);
        }

        successiveFailuresCount = 0;

        OSA_debug("Producer %d wrote frame %u to memory region. Counter = %u.\n", kProducerId, frame.index, framesCount);

        ret = MIO_imageManager_producerPutFrame(producerHandle, &frame);
        if (OSA_isFailed(ret)) {
            OSA_warn("Failed to put frame %u to image producer %d: %d.\n", frame.index, kProducerId, ret);
            continue;
        }

        OSA_debug("Producer %d released frame %u. Counter = %u.\n", kProducerId, frame.index, framesCount);
    }

    OSA_info("Image producer %d, process %ld thread %ld quiting...\n", kProducerId, (long)OSA_getpid(), (long)OSA_gettid());
    return NULL;
}



int MEDIAD_init()
{
    int ret;
    int i;
    MIO_ImageManager_Config imageManagerConfig;
    MIO_ImageManager_ProducerFormat format;
    const unsigned int frameRate = 30;


    OSA_clear(&MEDIAD_gImageManagerHandle);
    OSA_clear(&MEDIAD_gImageProducerHandles);
    OSA_clear(&gImageProducerTasks);

    OSA_clear(&imageManagerConfig);
    imageManagerConfig.isProducer = 1;
    imageManagerConfig.frameFormat = DSCV_FRAME_TYPE_YUYV; // DSCV_FRAME_TYPE_JPG;
    imageManagerConfig.frameSize.w = 640;
    imageManagerConfig.frameSize.h = 480;

    OSA_clear(&format);
    format.frameType = imageManagerConfig.frameFormat;    
    format.frameSize = imageManagerConfig.frameSize;

    ret = MIO_imageManager_init(&imageManagerConfig, &MEDIAD_gImageManagerHandle);
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to init image manager: %d.\n", ret);
        return ret;
    }

    for (i = 0; i < OSA_arraySize(MEDIAD_gImageProducerHandles); ++i) {
        ret = MIO_imageManager_producerInit(i, &MEDIAD_gImageProducerHandles[i]);
        if (OSA_isFailed(ret)) {
            OSA_error("Failed to init image producer %d: %d.\n", i, ret);
            goto _failure;
        }

        #if 0    /* leave this operation on user requests */
        ret = MIO_imageManager_producerOpen(MEDIAD_gImageProducerHandles[i]);
        if (OSA_isFailed(ret)) {
            OSA_error("Failed to open image producer %d: %d.\n", i, ret);
            goto _failure;
        }

        ret = MIO_imageManager_producerSetFormat(MEDIAD_gImageProducerHandles[i], &format);
        if (OSA_isFailed(ret)) {
            OSA_error("Failed to set format on image producer %d: %d.\n", i, ret);
            goto _failure;
        }

        ret = MIO_imageManager_producerSetFrameRate(MEDIAD_gImageProducerHandles[i], frameRate);
        if (OSA_isFailed(ret)) {
            /* this is not a fatal error */
            OSA_warn("Failed to set frame rate to %u on image producer %d: %d.\n", frameRate, i, ret);            
        }

        ret = MIO_imageManager_producerStartStreaming(MEDIAD_gImageProducerHandles[i]);
        if (OSA_isFailed(ret)) {
            OSA_error("Failed to start streaming on image producer %d: %d.\n", i, ret);
            goto _failure;
        }
        #endif

        gImageProducerTasksShouldQuit[i] = OSA_False;
        ret = pthread_create(&gImageProducerTasks[i], NULL, MEDIAD_imageProducerMain, (void *)i);
        if (0 != ret) {
            ret = errno;
            OSA_error("Failed to create image producer %d task: %d.\n", i, ret);
            goto _failure;
        }
    }

    ret = MEDIAD_cmdInit();
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to init command related functions: %d.\n", ret);
        goto _failure;
    }

    return ret;

_failure:
    MEDIAD_deinit();
    return ret;
}


int MEDIAD_deinit()
{
    int i;


    MEDIAD_cmdDeinit();

    for (i = 0; i < OSA_arraySize(MEDIAD_gImageProducerHandles); ++i) {
        gImageProducerTasksShouldQuit[i] = OSA_True;
    }

    MEDIAD_wait();

    for (i = 0; i < OSA_arraySize(MEDIAD_gImageProducerHandles); ++i) {
        MIO_imageManager_producerStopStreaming(MEDIAD_gImageProducerHandles[i]);
        MIO_imageManager_producerClose(MEDIAD_gImageProducerHandles[i]);
        MIO_imageManager_producerDeinit(MEDIAD_gImageProducerHandles[i]);
        MEDIAD_gImageProducerHandles[i] = NULL;
    }

    MIO_imageManager_deinit(MEDIAD_gImageManagerHandle);
    MEDIAD_gImageManagerHandle = NULL;

    return OSA_STATUS_OK;
}


int MEDIAD_wait()
{
    int i;

    for (i = 0; i < OSA_arraySize(MEDIAD_gImageProducerHandles); ++i) {
        /*  If that thread has already terminated, then pthread_join() returns immediately. */
        pthread_join(gImageProducerTasks[i], NULL);
    }

    return OSA_STATUS_OK;
}
