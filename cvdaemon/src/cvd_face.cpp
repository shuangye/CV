/*
* Created by Liu Papillon, on Nov 21, 2017.
*
* Desc: living face determination depends on the result of face detection, thus, it will only run when face detection is running.
*/

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <osa/osa.h>
#include <face/face.hpp>
#include <dscv/dscv.hpp>
#include <cvd/cvd.h>                  /* for type definitions */
#include "cvd_face.hpp"
#include "cvd_pri.hpp"
#include "cvd_config.hpp"
#include "cvd_image.hpp"
#include "cvd_comm.hpp"
#include "cvd_debug.hpp"


/*************************** type definitions **********************************/



/*************************** global variables **********************************/

vector<Mat>                             CVD_gSourceImagesForLivingDetermination(CVD_INPUT_IMAGES_COUNT);
vector<cv::Rect>                        CVD_gFacesForLivingDetermination(CVD_INPUT_IMAGES_COUNT);
OSA_Size                                CVD_gFaceDetectionInputImageSize;

static bool                             gIsInited = false;
static StereoRectifier                 *gpStereoRectifier = NULL;
static FACE_Handle                      gFaceHandle = NULL;
static pthread_t                        gFaceDetectionTask;
static pthread_t                        gLivingFaceDeterminationTask;
static bool                             gFaceDetectionTaskShouldQuit = false;                    /* TODO: protect with a lock */
static bool                             gLivingFaceDeterminationTaskShouldQuit = false;


/*************************** static functions **********************************/


static int CVD_setThreadPriproty(int priotity)
{
    int ret;
    int policy;
    struct sched_param param;

    ret = pthread_getschedparam(pthread_self(), &policy, &param);
    if (0 != ret) {
        return ret;
    }

    OSA_info("Thread %ld current policy %d, priority %d -> %d.\n", (long)OSA_gettid(), policy, param.sched_priority, priotity);

    param.sched_priority = priotity;
    ret = pthread_setschedparam(pthread_self(), policy, &param);
    return ret;
}


#pragma region Detect Face


static void CVD_faceSwitchFaceDetection(const int enable)
{
    pthread_rwlock_wrlock(&CVD_gpCvOut->livingFaceControlLock);
    CVD_gpCvOut->faceDetectionEnabled = enable;
    pthread_rwlock_unlock(&CVD_gpCvOut->livingFaceControlLock);
}


static int CVD_faceDetectFace(vector<Mat> &sourceImages, vector<cv::Rect> &outFaces)
{
    int                             ret;
    vector<cv::Rect>                faces(CVD_INPUT_IMAGES_COUNT);
    vector<int>                     cameras(CVD_INPUT_IMAGES_COUNT);
    vector<Mat>                     frames(CVD_INPUT_IMAGES_COUNT);
    vector<Mat>                     rectifiedFrames(CVD_INPUT_IMAGES_COUNT);
    Mat                             combinedFrame;
    Mat                             frameForFaceDetection;
    double                          factor = 1;
    size_t                          i;    


    if (!gIsInited) {
        OSA_error("The face related function is not inited yet.\n");
        return OSA_STATUS_EPERM;
    }

    cameras[0] = 0;
    cameras[0] = 1;
    ret = CVD_imageGet(cameras, frames);
    if (OSA_isFailed(ret)) {
        OSA_debug("Failed to get frames: %d.\n", ret);
        return OSA_STATUS_ENODATA;
    }

#ifdef OSA_DEBUG
    CVD_debugDumpImages(string("DumpImageProvider"), frames);
#endif

#if 1    /* stereo rectification */
    if (!gpStereoRectifier->getValidity()) {
        if (0 == access(CVD_STEREO_CALIBRATION_PATH, R_OK)) {
            gpStereoRectifier->load(string(CVD_STEREO_CALIBRATION_PATH), cv::Size(frames[0].cols, frames[0].rows));
        }
    }
    if (gpStereoRectifier->getValidity()) {
        ret = gpStereoRectifier->rectify(frames[0], frames[1], rectifiedFrames[0], rectifiedFrames[1], &combinedFrame);
        if (OSA_isSucceeded(ret)) {
            if (0 != access(CVD_STEREO_CALIBRATION_PREVIEW_IMAGE_PATH, F_OK)) {
                cv::imwrite(string(CVD_STEREO_CALIBRATION_PREVIEW_IMAGE_PATH), combinedFrame);
                chmod(CVD_STEREO_CALIBRATION_PREVIEW_IMAGE_PATH, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
            }
            #ifdef OSA_DEBUG 
            vector<Mat> dumpImages = frames;
            dumpImages.push_back(combinedFrame);
            CVD_debugDumpImages(string("DumpStereoRectified"), dumpImages);
            #endif
            frames = rectifiedFrames;
        }
    }    
#endif

    for (i = 0; i < CVD_INPUT_IMAGES_COUNT; ++i) {
        if (frames[i].cols > CVD_FACE_DETECTION_W) {
            factor = (double)CVD_FACE_DETECTION_W / frames[i].cols;
            cv::resize(frames[i], frameForFaceDetection, cv::Size(0, 0), factor, factor);
        }
        else {
            frameForFaceDetection = frames[i];
        }

        CVD_gFaceDetectionInputImageSize.w = frameForFaceDetection.cols;
        CVD_gFaceDetectionInputImageSize.h = frameForFaceDetection.rows;
        
        ret = FACE_detectFace(gFaceHandle, frameForFaceDetection, faces[i]);
        if (OSA_isFailed(ret)) {
            OSA_warn("Failed to detect face of image %u: %d.\n", i, ret);            
            faces[i] = cv::Rect(0, 0, 0, 0);
            continue;
        }

        OSA_debug("Face detection in image %u of size %dx%d: %dx%d@(%d,%d).\n", i, CVD_gFaceDetectionInputImageSize.w, CVD_gFaceDetectionInputImageSize.h,
            faces[i].width, faces[i].height, faces[i].x, faces[i].y);
    }
  
    for (i = 0; i < CVD_INPUT_IMAGES_COUNT; ++i) {
        sourceImages[i] = frames[i];
        outFaces[i] = faces[i];
    }        
    
    return ret;
}


static void *CVD_faceDetectionThreadMain(void *pArg)
{
    int                             ret; 
    int                             enabled = 0;
    vector<Mat>                     sourceImages(CVD_INPUT_IMAGES_COUNT);
    vector<cv::Rect>                faces(CVD_INPUT_IMAGES_COUNT);
    unsigned int                    validFaces;
    size_t                          i;
    const int                       kSleepTime = 40;    /* sleep one-frame time (assumes 25fps) */


    OSA_info("Created face detection task. Process ID %ld, thread ID %ld.\n", (long)OSA_getpid(), (long)OSA_gettid());

    CVD_setThreadPriproty(10);

    for (; ;) {
        if (gFaceDetectionTaskShouldQuit) {
            OSA_info("Face detection task is required to quit.\n");
            break;
        }

        pthread_rwlock_rdlock(&CVD_gpCvOut->livingFaceControlLock);
        enabled = CVD_gpCvOut->faceDetectionEnabled;
        pthread_rwlock_unlock(&CVD_gpCvOut->livingFaceControlLock);

        if (!enabled) {
            OSA_msleep(1000);
            continue;
        }

        ret = CVD_faceDetectFace(sourceImages, faces);
        if (OSA_isFailed(ret)) {
            OSA_msleep(OSA_STATUS_ENODATA == ret ? kSleepTime * 50 : kSleepTime);
            continue;
        }

        validFaces = 0;

        for (i = 0; i < faces.size(); ++i) {
            OSA_debug("Face detection in image %u: %dx%d@(%d,%d).\n", i, faces[i].width, faces[i].height, faces[i].x, faces[i].y);
            if (!RectUtils::isZero(faces[i])) {
                validFaces |= 1 << i;    /* at least one valid face */
            }
        }
        
        /* put result to shared memory */
        if (validFaces) {            
            ret = CVD_commPutFaceDetectionResult(sourceImages, faces, validFaces);
            if (OSA_isFailed(ret)) {
                OSA_warn("Failed to put face detection result: %d.\n", ret);
            }
        }

        /* do not make CPU very busy */
        /* OSA_msleep(kSleepTime); */
    }

    OSA_info("Face detection task, process %ld thread %ld quiting...\n", (long)OSA_getpid(), (long)OSA_gettid());
    return NULL;
}


#pragma endregion



#pragma region Determine Living Face


static int CVD_faceDetermineLivingFace(const vector<Mat> frames, const vector<cv::Rect> faces, int &possibility)
{
    int                           ret;
    vector<Mat>                   framesForLiving = frames;
    

    if (!gIsInited) {
        OSA_error("The face related function is not inited yet.\n");
        return OSA_STATUS_EPERM;
    }

    assert(CVD_INPUT_IMAGES_COUNT == frames.size());
    assert(CVD_INPUT_IMAGES_COUNT == faces.size());    

    ret = FACE_determineLiving(gFaceHandle, framesForLiving[0], framesForLiving[1], faces[0], faces[1], possibility);
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to determine living face: %d.\n", ret);
        return ret;
    }

    OSA_info("Determing live face, image size %dx%d, possibility %d.\n", framesForLiving[0].cols, framesForLiving[0].rows, possibility);

#if CVD_OPENCV_SUPPORTS_GUI
    imshow("Living Face 0", framesForLiving[0]);
    imshow("Living Face 1", framesForLiving[1]);
    waitKey(1000);
#endif

    return ret;
}


static void CVD_setLivingFaceDeterminationTaskBusyOrIdle(int busy)
{
    pthread_rwlock_wrlock(&CVD_gpCvOut->livingFaceControlLock);
    CVD_gpCvOut->livingFaceDeterminationBusy = busy;
    pthread_rwlock_unlock(&CVD_gpCvOut->livingFaceControlLock);
}


static void *CVD_faceLivingDeterminationThreadMain(void *pArg)
{
    int                             ret;
    int                             enabled = 0;
    int                             possibility = FACE_LIVING_FACE_IMPOSSIBLE;
    const int                       kSleepTime = 40;


    OSA_info("Created living face determination task. Process ID %ld, thread ID %ld.\n", (long)OSA_getpid(), (long)OSA_gettid());

    for (; ;) {
        if (gLivingFaceDeterminationTaskShouldQuit) {
            OSA_info("Living face determination task is required to quit.\n");
            break;
        }

        pthread_rwlock_rdlock(&CVD_gpCvOut->livingFaceControlLock);
        enabled = CVD_gpCvOut->livingFaceDeterminationEnabled;
        pthread_rwlock_unlock(&CVD_gpCvOut->livingFaceControlLock);

        if (!enabled) {
            OSA_msleep(kSleepTime);
            continue;
        }

        OSA_debug("Will detect living face in %u input images.\n", CVD_INPUT_IMAGES_COUNT);

        CVD_setLivingFaceDeterminationTaskBusyOrIdle(1);        
        ret = CVD_faceDetermineLivingFace(CVD_gSourceImagesForLivingDetermination, CVD_gFacesForLivingDetermination, possibility);
        CVD_setLivingFaceDeterminationTaskBusyOrIdle(0);
        
        if (OSA_isFailed(ret)) {
            OSA_warn("Determining living face failed with %d.\n", ret);
            OSA_msleep(kSleepTime);
            continue;
        }

        ret = CVD_commPutLivingFaceDeterminationResult(CVD_gSourceImagesForLivingDetermination, CVD_gFacesForLivingDetermination, possibility);
        if (OSA_isFailed(ret)) {
            OSA_warn("Failed to put face detection result: %d.\n", ret);
        }

        /* I have done my work, so I can have a nap */
        pthread_rwlock_wrlock(&CVD_gpCvOut->livingFaceControlLock);
        CVD_gpCvOut->livingFaceDeterminationEnabled = 0;
        pthread_rwlock_unlock(&CVD_gpCvOut->livingFaceControlLock);

        /* do not make CPU very busy */
        OSA_msleep(kSleepTime);
    }

    OSA_info("Living face determination task, process %ld thread %ld quiting...\n", (long)OSA_getpid(), (long)OSA_gettid());
    return NULL;
}


#pragma endregion


/*************************** global functions **********************************/


int CVD_faceInit()
{
    int ret;    
    FACE_Options options;


    gpStereoRectifier = new StereoRectifier();
    if (NULL == gpStereoRectifier) {
        return OSA_STATUS_ENOMEM;
    }

    OSA_clear(&options);
    OSA_strncpy(options.faceDetectionModelPath, CVD_FACE_DETECTION_MODEL_PATH);
    ret = FACE_init(options, gFaceHandle);
    if (OSA_isFailed(ret)) {
        return ret;
    }
    
    /* face detection should be enabled/disabled by end user */
    CVD_faceSwitchFaceDetection(0);

    gFaceDetectionTaskShouldQuit = false;
    ret = pthread_create(&gFaceDetectionTask, NULL, CVD_faceDetectionThreadMain, NULL);
    if (0 != ret) {
        ret = errno;
        OSA_error("Creating face detection task failed with %d.\n", ret);
        return ret;
    }    
    
    gLivingFaceDeterminationTaskShouldQuit = false;
    ret = pthread_create(&gLivingFaceDeterminationTask, NULL, CVD_faceLivingDeterminationThreadMain, NULL);
    if (0 != ret) {
        ret = errno;
        OSA_error("Creating living face determination task failed with %d.\n", ret);
        return ret;
    }    
    
    gIsInited = true;
    OSA_info("Inited face related functions.\n");
    CVD_faceSwitchFaceDetection(1);    /* for test only */
    return OSA_STATUS_OK;
}


int CVD_faceDeinit()
{
    if (!gIsInited) {
        return OSA_STATUS_OK;
    }

    gFaceDetectionTaskShouldQuit = true;
    gLivingFaceDeterminationTaskShouldQuit = true;
    pthread_join(gFaceDetectionTask, NULL);
    pthread_join(gLivingFaceDeterminationTask, NULL);

    if (NULL != gpStereoRectifier) {
        delete gpStereoRectifier;
        gpStereoRectifier = NULL;
    }

    FACE_deinitialize(gFaceHandle);        

    gIsInited = false;
    OSA_info("Deinited face related functions.\n");
    return OSA_STATUS_OK;
}


void CVD_faceInvalidateRectifier()
{
    if (NULL != gpStereoRectifier) {
        gpStereoRectifier->setValidity(false);
    }

    unlink(CVD_STEREO_CALIBRATION_PREVIEW_IMAGE_PATH);
}



/* to pend the caller, until the thread quits */
int CVD_faceWait()
{
    int ret = 0;
    ret |= pthread_join(gFaceDetectionTask, NULL);
    ret |= pthread_join(gLivingFaceDeterminationTask, NULL);
    return ret;
}
