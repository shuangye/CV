/*
* Created by Liu Papillon, on Nov 14, 2017.
* 
* The CV Daemon works as a server in respect to communication.
* It receives request from clients and perform the respect oprtation, and then responses the operation result.
*/

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <osa/osa.h>
#include <dscv/dscv.hpp>
#include <cvd/cvd.h>
#include "cvd_comm.hpp"
#include "cvd_config.hpp"
#include "cvd_pri.hpp"
#include "cvd_debug.hpp"


static bool gInited = false;
static int gAlgoFd = -1;
CVD_CvOut *CVD_gpCvOut = NULL;



void CVD_printMemory(const void *pAddr, const size_t len)
{
    size_t i = 0;
    const size_t lineLen = 32;
    Char buffer[8];
    Char line[sizeof(buffer) * lineLen];
    Char *p;
    Char *pLineAddr;


    if (NULL == pAddr || 0 == len) {
        return;
    }

    for (p = (Char *)pAddr, pLineAddr = (Char *)pAddr, i = 0; i < len; ++i, ++p) {
        snprintf(buffer, sizeof(buffer), "%02X ", *p);
        strncat(line, buffer, sizeof(line) - 1);
        if (i > 0 && (i % lineLen == 0)) {
            OSA_info("Memory content at %p: %s\n", pLineAddr, line);
            OSA_clear(line);
            pLineAddr += lineLen;
        }
    }
}



int CVD_commInit()
{
    int ret;
    pthread_condattr_t condAttr;
    pthread_mutexattr_t mutexAttr;
    

    /* make sure the shared memory is large enough */
    OSA_info("Shared region size %u, header size %u.\n", CVD_ALGO_MEM_LEN, sizeof(*CVD_gpCvOut));
    assert(CVD_ALGO_MEM_LEN >= sizeof(*CVD_gpCvOut));

    gAlgoFd = open(CVD_ALGO_DEV_FILE, O_RDWR);
    if (gAlgoFd < 0) {
        ret = errno;
        OSA_error("Failed to open file %s: %d.\n", CVD_ALGO_DEV_FILE, ret);
        return ret;
    }

#if 1
    CVD_gpCvOut = (CVD_CvOut *)mmap(NULL, CVD_ALGO_MEM_LEN, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_LOCKED, gAlgoFd, 0);
    if (MAP_FAILED == CVD_gpCvOut) {
        ret = errno;
        OSA_error("Failed to map %s file: %d.\n", CVD_ALGO_DEV_FILE, ret);
        goto _failure;
    }
#else
    CVD_gpCvOut = (CVD_CvOut *)malloc(CVD_ALGO_MEM_LEN);
    assert(NULL != CVD_gpCvOut);
#endif

    memset(CVD_gpCvOut, 0, CVD_ALGO_MEM_LEN);
    
    CVD_gpCvOut->magic = CVD_ALGO_MEM_MAGIC;
    
    ret = 0;
    ret |= pthread_rwlock_init(&CVD_gpCvOut->faceDetectionControltLock, NULL);
    ret |= pthread_rwlock_init(&CVD_gpCvOut->faceDetectionResultLock, NULL);
    ret |= pthread_rwlock_init(&CVD_gpCvOut->livingFaceControlLock, NULL);
    ret |= pthread_rwlock_init(&CVD_gpCvOut->livingFaceResultLock, NULL);
    if (0 != ret) {
        OSA_error("Failed to init thread sync objects.\n");
    }
    
    ret = 0;
    ret |= pthread_mutexattr_init(&mutexAttr);
    ret |= pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    ret |= pthread_mutex_init(&CVD_gpCvOut->calibratorRequestLock, &mutexAttr);
    ret |= pthread_mutexattr_destroy(&mutexAttr);
    if (0 != ret) {
        OSA_error("Failed to init thread sync objects.\n");
    }
    
    ret = 0;
    ret |= pthread_mutexattr_init(&mutexAttr);
    ret |= pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    ret |= pthread_mutex_init(&CVD_gpCvOut->calibratorResponseLock, &mutexAttr);
    ret |= pthread_mutexattr_destroy(&mutexAttr);
    if (0 != ret) {
        OSA_error("Failed to init thread sync objects.\n");
    }
    
    ret = 0;
    ret |= pthread_condattr_init(&condAttr);
    ret |= pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
    ret |= pthread_cond_init(&CVD_gpCvOut->calibratorRequestCond, &condAttr);    
    ret |= pthread_condattr_destroy(&condAttr);
    if (0 != ret) {
        OSA_error("Failed to init thread sync objects.\n");
    }
    
    ret = 0;
    ret |= pthread_condattr_init(&condAttr);
    ret |= pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
    ret |= pthread_cond_init(&CVD_gpCvOut->calibratorResponseCond, &condAttr);
    ret |= pthread_condattr_destroy(&condAttr);
    if (0 != ret) {
        OSA_error("Failed to init thread sync objects.\n");
    }
        
    CVD_gpCvOut->encodedFaceAreaOffset = sizeof(*CVD_gpCvOut);    /* at the end of the CVD_gpCvOut */
    CVD_gpCvOut->calibratorOperation = CVD_CMD_NONE;
    CVD_gpCvOut->inited = 1;

    msync(CVD_gpCvOut, CVD_ALGO_MEM_LEN, MS_SYNC | MS_INVALIDATE);

    gInited = true;
    OSA_info("Inited comm related functions. Mapped %s to %p. Magic = %08X.\n", CVD_ALGO_DEV_FILE, CVD_gpCvOut, CVD_gpCvOut->magic);
    // OSA_info("On init, cond value %d, mutex value %d.\n", *(int *)&CVD_gpCvOut->calibratorRequestCond, *(int *)&CVD_gpCvOut->calibratorRequestLock);
    return OSA_STATUS_OK;

_failure:
    CVD_commDeinit();
    return ret;
}


int CVD_commDeinit()
{
    if (!gInited) {
        return OSA_STATUS_OK;
    }

    if (MAP_FAILED != CVD_gpCvOut && NULL == CVD_gpCvOut) {
        /* TODO: check all attached processes have detached */
        pthread_rwlock_destroy(&CVD_gpCvOut->faceDetectionControltLock);
        pthread_rwlock_destroy(&CVD_gpCvOut->faceDetectionResultLock);
        pthread_rwlock_destroy(&CVD_gpCvOut->livingFaceControlLock);        
        pthread_rwlock_destroy(&CVD_gpCvOut->livingFaceResultLock);
        pthread_mutex_destroy(&CVD_gpCvOut->calibratorRequestLock);        
        pthread_cond_destroy(&CVD_gpCvOut->calibratorRequestCond);
        pthread_mutex_destroy(&CVD_gpCvOut->calibratorResponseLock);
        pthread_cond_destroy(&CVD_gpCvOut->calibratorResponseCond);
        OSA_clear(CVD_gpCvOut);
        munmap(CVD_gpCvOut, CVD_ALGO_MEM_LEN);
        CVD_gpCvOut = NULL;
    }

    if (gAlgoFd >= 0) {
        close(gAlgoFd);
        gAlgoFd = -1;
    }

    gInited = false;
    OSA_info("Deinited comm related functions.\n");
    return OSA_STATUS_OK;
}


int CVD_commPutFaceDetectionResult(const vector<cv::Mat> sourceImages, const vector<cv::Rect> faces, const unsigned int validFaces)
{
    size_t i;

    if (!gInited) {
        OSA_error("Comm related functions are not inited yet.\n");
        return OSA_STATUS_EPERM;
    }

    assert(sourceImages.size() == CVD_gSourceImagesForLivingDetermination.size());
    assert(faces.size() == OSA_arraySize(CVD_gpCvOut->faces));

    /* 1. Put results */
    pthread_rwlock_wrlock(&CVD_gpCvOut->faceDetectionResultLock);
    for (i = 0; i < OSA_arraySize(CVD_gpCvOut->faces); ++i) {
        if (!(validFaces & (1 << i))) {
            continue;
        }
        CVD_gpCvOut->faces[i].origin.x     = faces[i].x;
        CVD_gpCvOut->faces[i].origin.y     = faces[i].y;
        CVD_gpCvOut->faces[i].size.w       = faces[i].width;
        CVD_gpCvOut->faces[i].size.h       = faces[i].height;
        CVD_gpCvOut->faceDetectionFresh[i] = 1;
    }
    CVD_gpCvOut->faceDetectionInputImageSize = CVD_gFaceDetectionInputImageSize;    
    pthread_rwlock_unlock(&CVD_gpCvOut->faceDetectionResultLock);

    /* living face determination requires both faces are valid (1 << 0 | 1 << 1 = 3) */
    if (validFaces & 0x3) {
        /* 2. notify the living face determination task if it is not busy */
        pthread_rwlock_wrlock(&CVD_gpCvOut->livingFaceControlLock);
        if (!CVD_gpCvOut->livingFaceDeterminationBusy) {
            for (i = 0; i < sourceImages.size(); ++i) {
                /* make a deep copy */
                CVD_gSourceImagesForLivingDetermination[i] = sourceImages[i].clone();

                /* face detection and living face determination use different image sizes, so scaling is needed */
                CVD_gFacesForLivingDetermination[i].width  = faces[i].width * sourceImages[i].cols / CVD_gFaceDetectionInputImageSize.w;
                CVD_gFacesForLivingDetermination[i].height = faces[i].height * sourceImages[i].rows / CVD_gFaceDetectionInputImageSize.h;
                CVD_gFacesForLivingDetermination[i].x      = faces[i].x * sourceImages[i].cols / CVD_gFaceDetectionInputImageSize.w;
                CVD_gFacesForLivingDetermination[i].y      = faces[i].y * sourceImages[i].rows / CVD_gFaceDetectionInputImageSize.h;
#if 1
                CVD_debugDumpImages(string("DumpLivingSource"), CVD_gSourceImagesForLivingDetermination);
#endif
            }
        }
        CVD_gpCvOut->livingFaceDeterminationEnabled = 1;  /* hey guy, wake up and begin working */
        pthread_rwlock_unlock(&CVD_gpCvOut->livingFaceControlLock);
    }

    return OSA_STATUS_OK;
}


int CVD_commPutLivingFaceDeterminationResult(const vector<cv::Mat> inputImages, const vector<cv::Rect> faces, const int possibility)
{
    int                        ret;
    Mat                        faceRefImage = inputImages[0];    /* Take the first image as the reference. This assumes the cameras have been calibrated. */
    Mat                        face;
    cv::Rect                   faceRefRegion = faces[0];    
    cv::Rect                   faceRegion;
    vector<Uchar>              encodedFace;
    size_t                     len;
    bool                       shouldPutImage = possibility >= CVD_gpCvOut->livingFaceThreshold;

    if (!gInited) {
        OSA_error("Comm related functions are not inited yet.\n");
        return OSA_STATUS_EPERM;
    }

    /* if the face is empty, nothing needs to be done */
    if (RectUtils::isZero(faceRefRegion)) {
        return OSA_STATUS_OK;
    }

    if (shouldPutImage) {        
        faceRegion = faceRefRegion;
        face = Mat(faceRefImage, faceRegion);

        ret = DSCV_encodeMat(face, DSCV_FRAME_TYPE_JPG, encodedFace);
        if (OSA_isFailed(ret)) {
            OSA_warn("Failed to encode face area to jpg: %d.\n", ret);
            return ret;
        }

        len = encodedFace.size();
        if (len > CVD_ENCODED_FACE_AREA_MAX_LEN) {
            OSA_warn("Encoded face area length %u is larger than the buffer available %u.\n", len, CVD_ENCODED_FACE_AREA_MAX_LEN);
            return OSA_STATUS_ENOMEM;
        }
    }

    pthread_rwlock_wrlock(&CVD_gpCvOut->livingFaceResultLock);
    if (shouldPutImage) {
        memcpy((void *)((size_t)CVD_gpCvOut + CVD_gpCvOut->encodedFaceAreaOffset), encodedFace.data(), len);
        CVD_gpCvOut->encodedFaceAreaLen = len;
        OSA_info("Possibility = %d, threshold %d. Put encoded JPEG face image with length %d to shared memory region.",
            possibility, CVD_gpCvOut->livingFaceThreshold, len);
#if 1
        vector<Mat> dumpImages = inputImages;
        dumpImages.push_back(face);
        CVD_debugDumpImages(string("DumpEncodedFace"), dumpImages);
#endif
    }
    else {
        CVD_gpCvOut->encodedFaceAreaLen = 0;
    }
    CVD_gpCvOut->livingPossibility = possibility;
    CVD_gpCvOut->livingFaceDeterminationFresh = 1;
    pthread_rwlock_unlock(&CVD_gpCvOut->livingFaceResultLock);

    return OSA_STATUS_OK;
}
