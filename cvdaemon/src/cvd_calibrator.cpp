/*
* Created by Liu Papillon, on Nov 16, 2017.
* 
*/

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <osa/osa.h>
#include <dscv/dscv.hpp>
#include <comm/comm.h>
#include "cvd_config.hpp"
#include "cvd_calibrator.hpp"
#include "cvd_image.hpp"
#include "cvd_pri.hpp"
#include "cvd_face.hpp"
#include "cvd_debug.hpp"


static bool gIsInited = false;
static pthread_t gCalibratorTask;
static bool gCalibratorTaskShouldQuit;
static cv::Size gPatternSize;
static size_t gPatternImagesCount;
static vector<Mat> gLeftPatternImages, gRightPatternImages;
static Calibrator *gpCalibrator = NULL;


static int CVD_stereoCalibratorDeinit()
{
    if (!gIsInited) {
        return OSA_STATUS_OK;
    }

    gLeftPatternImages.clear();
    gRightPatternImages.clear();
    if (NULL != gpCalibrator) {
        delete gpCalibrator;
        gpCalibrator = NULL;
    }

    gIsInited = false;
    return OSA_STATUS_OK;
}


static int CVD_stereoCalibratorInit(cv::Size &patternSize)
{
    int ret;

    gPatternSize = patternSize;
    gpCalibrator = new Calibrator(patternSize);
    if (NULL == gpCalibrator) {
        OSA_error("Failed to allocate memory for calibrator.\n");
        ret = OSA_STATUS_ENOMEM;
        goto _quit;
    }

    gPatternImagesCount = 0;

    OSA_info("Inited calibrator, chessboard pattern size %dx%d.\n", gPatternSize.width, gPatternSize.height);
    gIsInited = true;
    return OSA_STATUS_OK;

_quit:
    CVD_stereoCalibratorDeinit();
    return ret;
}


static int CVD_stereoCalibratorCollectImage()
{
    int                           ret;
    bool                          patternFound = true;
    vector<int>                   cameras(CVD_INPUT_IMAGES_COUNT);
    vector<Mat>                   frames(CVD_INPUT_IMAGES_COUNT);
    

    if (!gIsInited) {
        OSA_error("The calibration related function is not inited yet.\n");
        return OSA_STATUS_EPERM;
    }

    cameras[0] = 0;
    cameras[1] = 1;
    ret = CVD_imageGet(cameras, frames);
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to get frames: %d.\n", ret);
        return ret;
    }
           
    for (size_t i = 0; i < CVD_INPUT_IMAGES_COUNT; ++i) {
        if (!gpCalibrator->peek(frames[i])) {
            patternFound = false;
            break;
        }
    }

    if (patternFound) {
        gLeftPatternImages.push_back(frames[0]);
        gRightPatternImages.push_back(frames[1]);
        ++gPatternImagesCount;
        OSA_info("Current pattern images count = %u.\n", gPatternImagesCount);
    }
    else {
        OSA_info("Patternn was not found.\n");
    }

#if CVD_OPENCV_SUPPORTS_GUI
    imshow("Calibration camera 0", frames[0]);
    imshow("Calibration camera 1", frames[1]);
    waitKey(1000);
#endif

    return patternFound ? OSA_STATUS_OK : OSA_STATUS_EAGAIN;    
}


static int CVD_stereoCalibratorCalibrate()
{
    int ret;
    const string dumpPath = string(CVD_STEREO_CALIBRATION_PATH);
    StereoCalibrator stereoCalibrator(gPatternSize);
    vector<Mat> dumpImages;


    if (!gIsInited) {
        OSA_error("The calibration related function is not inited yet.\n");
        ret = OSA_STATUS_EPERM;
        goto _quit;
    }

    stereoCalibrator.setPatternImages(gLeftPatternImages, gRightPatternImages);

#ifdef OSA_DEBUG 
    dumpImages.reserve(gLeftPatternImages.size() + gRightPatternImages.size());
    dumpImages.insert(dumpImages.end(), gLeftPatternImages.begin(), gLeftPatternImages.end());
    dumpImages.insert(dumpImages.end(), gRightPatternImages.begin(), gRightPatternImages.end());
    CVD_debugDumpImages(string("DumpPatternImages"), dumpImages);
#endif

    OSA_info("Calibrating...\n");
    ret = stereoCalibrator.calibrate();
    if (OSA_isFailed(ret)) {
        OSA_error("Calibrating failed with %d.\n", ret);
        goto _quit;
    }

    OSA_info("Dumping result...\n");
    ret = stereoCalibrator.dumpResult(dumpPath);
    if (OSA_isFailed(ret)) {
        OSA_error("Dumping calibration result to %s failed with %d.\n", dumpPath.c_str(), ret);
        goto _quit;
    }

    /* notify the rectifier to reload the calibration file */
    CVD_faceInvalidateRectifier();
    OSA_info("Done. Calibration file saved to %s.\n", dumpPath.c_str());

_quit:
    CVD_stereoCalibratorDeinit();    
    return ret;
}


static void * CVD_calibratorThreadMain(void *pArg)
{
    int ret;
    bool shouldWork = true;
    int operation = CVD_CMD_NONE;
    cv::Size patternSize(0, 0);


    OSA_info("Created calibrator task. Process ID %d, thread ID %ld.\n", OSA_getpid(), OSA_gettid());

#if 0
    pthread_condattr_t condAttr;
    pthread_cond_t condVar;
    pthread_mutex_t mutex;
    pthread_mutexattr_t mutexAttr;

    pthread_condattr_init(&condAttr);
    pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&condVar, &condAttr);    
    pthread_condattr_destroy(&condAttr);

    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mutex, &mutexAttr);
    pthread_mutexattr_destroy(&mutexAttr);

    for (; ;) {
        pthread_mutex_lock(&mutex);
        for (; ;) {
            OSA_info("CVD waiting, and released lock.\n");
            ret = pthread_cond_wait(&condVar, &mutex);
            OSA_info("CVD wait returned %d (got lock if OK).\n", ret);
        }
        pthread_mutex_unlock(&mutex);
    }
#endif
    
    for (; ;) {
        /* wait for request from client */
        
        OSA_debug("CVD trying to get request lock...\n");
        pthread_mutex_lock(&CVD_gpCvOut->calibratorRequestLock);
        OSA_debug("CVD got request lock.\n");

        for (; ;) {
            shouldWork = CVD_CMD_INIT_CALIBRATION == CVD_gpCvOut->calibratorOperation
                || CVD_CMD_COLLECT_CALIBRATION_IMAGE == CVD_gpCvOut->calibratorOperation
                || CVD_CMD_STEREO_CALIBRATE == CVD_gpCvOut->calibratorOperation;
            if (shouldWork) {
                operation = CVD_gpCvOut->calibratorOperation;
                patternSize.width = CVD_gpCvOut->calibratorPatternSize.w;
                patternSize.height = CVD_gpCvOut->calibratorPatternSize.h;
                break;
            }
            
#if CVD_USE_COND_VAR
            OSA_info("CVD waiting, and released request lock. Cond value %d, mutex value %d.\n", *(int *)&CVD_gpCvOut->calibratorRequestCond, *(int *)&CVD_gpCvOut->calibratorRequestLock);            
            ret = pthread_cond_wait(&CVD_gpCvOut->calibratorRequestCond, &CVD_gpCvOut->calibratorRequestLock);
            OSA_info("CVD wait returned %d.\n", ret);
            OSA_info("CVD wait returned %d. Cond value %d, mutex value %d.\n", ret, *(int *)&CVD_gpCvOut->calibratorRequestCond, *(int *)&CVD_gpCvOut->calibratorRequestLock);
#else
            pthread_mutex_unlock(&CVD_gpCvOut->calibratorRequestLock);
            OSA_debug("CVD will sleep, and released request lock...\n");

            OSA_msleep(1000);

            OSA_debug("CVD woke up, and trying to get request lock...\n");
            pthread_mutex_lock(&CVD_gpCvOut->calibratorRequestLock);
            OSA_debug("CVD woke up, and got request lock.\n");
#endif
        }
        
        OSA_info("Calibrator got an operation command %d.\n", operation);

        switch (operation) {
        case CVD_CMD_INIT_CALIBRATION:
            ret = CVD_stereoCalibratorInit(patternSize);
            break;
        case CVD_CMD_COLLECT_CALIBRATION_IMAGE:
            ret = CVD_stereoCalibratorCollectImage();
            break;
        case CVD_CMD_STEREO_CALIBRATE:
            ret = CVD_stereoCalibratorCalibrate();
            break;
        default:
            OSA_warn("Unsupported calibrator operation %d.\n", operation);
            ret = OSA_STATUS_EBADRQC;
            break;
        }

        CVD_gpCvOut->calibratorOperationStatus = ret;
        CVD_gpCvOut->calibratorOperation = CVD_CMD_NONE;

        pthread_mutex_unlock(&CVD_gpCvOut->calibratorRequestLock);


        /* notify the client that we have done our work */
        pthread_mutex_lock(&CVD_gpCvOut->calibratorResponseLock);
        CVD_gpCvOut->calibratorOperationDone = 1;
#if CVD_USE_COND_VAR
        pthread_cond_signal(&CVD_gpCvOut->calibratorResponseCond);
#endif
        pthread_mutex_unlock(&CVD_gpCvOut->calibratorResponseLock);
    }

    OSA_info("calibration task, process %ld thread %ld quiting...\n", (long)OSA_getpid(), (long)OSA_gettid());
    return NULL;
}


int CVD_calibratorInit()
{
    int ret;

    gCalibratorTaskShouldQuit = false;
    ret = pthread_create(&gCalibratorTask, NULL, CVD_calibratorThreadMain, NULL);
    if (0 != ret) {
        ret = errno;
        OSA_error("Creating face detection task failed with %d.\n", ret);
        return ret;
    } 

    OSA_info("Inited calibrator related functions.\n");
    return OSA_STATUS_OK;
}


int CVD_calibratorDeinit()
{
    int ret;

    gCalibratorTaskShouldQuit = true;
    ret = pthread_join(gCalibratorTask, NULL);

    CVD_stereoCalibratorDeinit();

    OSA_info("Deinited calibrator related functions.\n");
    return ret;
}


int CVD_calibratorWait()
{
    return pthread_join(gCalibratorTask, NULL);
}
