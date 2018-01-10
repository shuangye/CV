/*
* Created by Liu Papillon, on Nov 14, 2017.
*/

#include <pthread.h>
#include <vector>
#include <osa/osa.h>
#include <dscv/dscv.hpp>
#include <cvd/cvd_image_provider.h>
#include <mio/image_manager/image_manager.h>
#include "cvd_image.hpp"
#include "cvd_config.hpp"

#define USE_IMAGE_PROVIDER                                                     0
                                                                               
#if USE_IMAGE_PROVIDER                                                         
#define CVD_IMAGE_PROVIDER                                                     CVD_ImageProviderHandle
#define CVD_IMAGE_PROVIDER_CONFIG                                              CVD_ImageProviderConfig
#define CVD_IMAGE_PROVIDER_INIT(config, provider)                              CVD_imageProviderInit((config), (provider))
#define CVD_IMAGE_PROVIDER_DEINIT(provider)                                    CVD_imageProviderDeinit((provider))
#define CVD_IMAGE_PROVIDER_READ_FRAME(provider, id, frame)                     CVD_imageProviderReadFrame((provider), (id), frame)
#else                                                                          
#define CVD_IMAGE_PROVIDER                                                     MIO_ImageManager_Handle
#define CVD_IMAGE_PROVIDER_CONFIG                                              MIO_ImageManager_Config
#define CVD_IMAGE_PROVIDER_INIT(config, provider)                              MIO_imageManager_init((config), (provider))
#define CVD_IMAGE_PROVIDER_DEINIT(provider)                                    MIO_imageManager_deinit((provider))
#define CVD_IMAGE_PROVIDER_READ_FRAME(provider, id, frame)                     MIO_imageManager_readFrame((provider), (id), MIO_IMAGE_MANAGER_CONSUMER_ROLE_ANALYZER, frame)
#define CVD_IMAGE_PROVIDER_RELEASE_FRAME(provider, id, frame)                  MIO_imageManager_releaseFrame((provider), (id), MIO_IMAGE_MANAGER_CONSUMER_ROLE_ANALYZER, frame)
#endif



// #error "This is Windows"
#if defined(OSA_OS_ANDROID)


static bool                              gIsInited = false;
static CVD_IMAGE_PROVIDER                gImageProvider = NULL;
static pthread_t                         gImageTask;



static void * CVD_imageThreadMain(void *pArg)
{
    int                                 ret;
    const int                           kInterval = 2000;
    CVD_IMAGE_PROVIDER_CONFIG           config = { 0 };
    
    OSA_info("Created image task. Process ID %ld, thread ID %ld.\n", (long)OSA_getpid(), (long)OSA_gettid());
    
    for (; ;) {
        ret = CVD_IMAGE_PROVIDER_INIT(&config, &gImageProvider);
        if (OSA_isSucceeded(ret)) {
            OSA_info("I've inited image provider.\n");
            break;    /* quit this thread upon successful init */
        }
        OSA_debug("Init image provider failed with %d. Will try again later...\n", ret);
        OSA_msleep(kInterval);
    }

    OSA_info("Image provider is inited. Process %ld thread %ld quiting...\n", (long)OSA_getpid(), (long)OSA_gettid());
    return NULL;
}


int CVD_imageInit()
{
    int    ret;    
    int    i;
    const int kInterval = 2000;

    if (gIsInited) {
        return OSA_STATUS_OK;
    }

    /* Producer will init only when camera is running. So we should try more than once until it is inited. */
    /* start a new thread to perform this task, so the program will not block here */
    ret = pthread_create(&gImageTask, NULL, CVD_imageThreadMain, NULL);
    if (0 != ret) {
        ret = errno;
        OSA_error("Creating image task failed with %d.\n", ret);
        return ret;
    }  

    gIsInited = true;
    OSA_info("Inited image related functions.\n");
    return OSA_STATUS_OK;
}


int CVD_imageDeinit()
{
    if (!gIsInited) {
        return OSA_STATUS_OK;
    }

    CVD_IMAGE_PROVIDER_DEINIT(gImageProvider);

    gIsInited = false;    
    OSA_info("Deinited image related functions.\n");
    return OSA_STATUS_OK;
}


int CVD_imageGet(const vector<int> cameras, vector<Mat> &frames)
{
    int ret;
    size_t i;
    DSCV_Frame frame;
    

    if (!gIsInited) {
        OSA_warn("The image related function is not inited yet.\n");
        return OSA_STATUS_EPERM;
    }

    assert(cameras.size() == frames.size());

    for (i = 0; i < cameras.size(); ++i) {
        OSA_clear(&frame);
        ret = CVD_IMAGE_PROVIDER_READ_FRAME(gImageProvider, cameras[i], &frame);
        if (OSA_isFailed(ret)) {
            OSA_debug("Getting frame from camera %d failed with %d (image provider handle %p).\n", cameras[i], ret, gImageProvider);
            return ret;
        }

        OSA_debug("Got one frame index %u size %dx%d, type %d, len %d, data at %p from camera %d.\n",
            frame.index, frame.size.w, frame.size.h, frame.type, frame.dataLen, frame.pData, cameras[i]);

        ret = DSCV_matFromFrame(&frame, frames[i]);
        if (OSA_isFailed(ret)) {
            CVD_IMAGE_PROVIDER_RELEASE_FRAME(gImageProvider, cameras[i], &frame);
            OSA_warn("Converting frame %u to OpenCV Mat failed with %d.\n", i, ret);
            return ret;
        }

        /* OpenCV Mat has it own data copy, so the image from provider can be released */
        CVD_IMAGE_PROVIDER_RELEASE_FRAME(gImageProvider, cameras[i], &frame);

        /* frames[i] = imread(CVD_TEST_IMAGE_PATH); */    /* for debugging */
        if (frames[i].empty()) {
            OSA_warn("Failed to get image.\n");
            return OSA_STATUS_EINVAL;
        }
    }

    return OSA_STATUS_OK;
}


#elif defined(OSA_OS_GNU_LINUX)


int CVD_imageInit()
{
    return OSA_STATUS_OK;
}


int CVD_imageDeinit()
{
    return OSA_STATUS_OK;
}


int CVD_imageGet(const vector<int> cameras, vector<Mat> &frames)
{
    OSA_error("This function is not implemented on Linux.\n");
    return OSA_STATUS_EPERM;
}


#elif defined(OSA_OS_WINDOWS)


#include <mio/mio.hpp>
using namespace mio;

static Camera *gpCamera = NULL;


int CVD_imageInit()
{
    vector<int> cameras;
    cameras.push_back(0);
    cameras.push_back(1);
    gpCamera = new Camera(cameras);
    if (NULL == gpCamera) {
        return OSA_STATUS_ENOMEM;
    }
    return OSA_STATUS_OK;
}


int CVD_imageDeinit()
{
    if (NULL != gpCamera) {
        delete gpCamera;
        gpCamera = NULL;
    }
    return OSA_STATUS_OK;
}


int CVD_imageGet(const vector<int> cameras, vector<Mat> &frames)
{
    int ret;

    if (NULL == gpCamera) {
        OSA_error("Camera is not inited yet.\n");
        return OSA_STATUS_EPERM;
    }

    ret = gpCamera->getFrames(cameras, frames);
    
    return ret;
}


#endif // !OSA_OS_WINDOWS