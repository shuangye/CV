/*
* Created by Liu Papillon, on Nov 14, 2017.
*/

#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <osa/osa.h>
#include <assert.h>
#include <comm/comm.h>
#include <cvd/cvd.h>
#include <cvc/cvc.h>
#include "cvc_comm.h"
#include "cvc_config.h"


static Bool gIsInited = OSA_False;
static int gAlgoFd = -1;
static CVD_CvOut *CVC_gpCvOut = NULL;


int CVC_commInit()
{
    int ret;
    

    /* make sure the shared memory is large enough */
    OSA_info("Shared region size %u, header size %u.\n", CVD_ALGO_MEM_LEN, sizeof(*CVC_gpCvOut));
    assert(CVD_ALGO_MEM_LEN >= sizeof(*CVC_gpCvOut));

    gAlgoFd = open(CVD_ALGO_DEV_FILE, O_RDWR);
    if (gAlgoFd < 0) {
        ret = errno;
        OSA_error("Failed to open file %s: %d.\n", CVD_ALGO_DEV_FILE, ret);
        return ret;
    }

    CVC_gpCvOut = (CVD_CvOut *)mmap(NULL, CVD_ALGO_MEM_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, gAlgoFd, 0);
    if (MAP_FAILED == CVC_gpCvOut) {
        ret = errno;
        OSA_error("Failed to map %s file: %d.\n", CVD_ALGO_DEV_FILE, ret);
        goto _failure;
    }

    if (CVD_ALGO_MEM_MAGIC != CVC_gpCvOut->magic || !CVC_gpCvOut->inited) {
        OSA_error("Shared memory is not inited yet. Maybe the server side it not running?\n");
        ret = OSA_STATUS_EAGAIN;
        goto _failure;
    }

    OSA_info("Shared memory is ready at %p.\n", CVC_gpCvOut);       

    gIsInited = OSA_True;
    OSA_info("Inited comm related functions.\n");
    return OSA_STATUS_OK;
    
_failure:    
    CVC_commDeinit();
    return ret;
}


int CVC_commDeinit()
{
    if (MAP_FAILED != CVC_gpCvOut && NULL != CVC_gpCvOut) {
        munmap(CVC_gpCvOut, CVD_ALGO_MEM_LEN);
        CVC_gpCvOut = NULL;
    }

    if (gAlgoFd >= 0) {
        close(gAlgoFd);
        gAlgoFd = -1;
    }

    gIsInited = OSA_False;
    OSA_info("Deinited CVC module.\n");
    return OSA_STATUS_OK;
}


int CVC_commGetDetectedFace(const int id, OSA_Rect *pFace, OSA_Size *pRelativeSize)
{
    int                            ret;

    if (!gIsInited) {
        OSA_error("This module is not inited yet. Please init first.\n");
        return OSA_STATUS_EPERM;
    }

    if (!OSA_isInRange(id, 0, OSA_arraySize(CVC_gpCvOut->faces))) {
        OSA_error("ID %d exceeds valid range.\n", id);
        return OSA_STATUS_EINVAL;
    }
    
    pthread_rwlock_wrlock(&CVC_gpCvOut->faceDetectionResultLock);
    if (CVC_gpCvOut->faceDetectionFresh[id]) {
        *pFace = CVC_gpCvOut->faces[id];
        *pRelativeSize = CVC_gpCvOut->faceDetectionInputImageSize;
        CVC_gpCvOut->faceDetectionFresh[id] = 0;
        ret = OSA_STATUS_OK;
    }
    else {
        ret = OSA_STATUS_EAGAIN;
    }
    pthread_rwlock_unlock(&CVC_gpCvOut->faceDetectionResultLock);

    if (OSA_isFailed(ret)) {
        OSA_debug("Detecting face failed with %d.\n", ret);
    }
    else {
        OSA_debug("Detected face is %dx%d@(%d,%d) in image size %dx%d.\n", pFace->size.w, pFace->size.h, pFace->origin.x, pFace->origin.y, pRelativeSize->w, pRelativeSize->h);
    }

    return ret;
}


int CVC_commSetLivingFaceThreshold(const int threshold)
{
    /* this function is not called frequently, so no need to lock */
    CVC_gpCvOut->livingFaceThreshold = threshold;
    return OSA_STATUS_OK;
}


int CVC_commGetLivingFace(int *pPossibility, void *pFace, size_t *pLen)
{
    int                            ret;

    if (!gIsInited) {
        OSA_error("This module is not inited yet. Please init first.\n");
        return OSA_STATUS_EPERM;
    }
        
    pthread_rwlock_wrlock(&CVC_gpCvOut->livingFaceResultLock);
    if (CVC_gpCvOut->livingFaceDeterminationFresh) {
        if (*pLen < CVC_gpCvOut->encodedFaceAreaLen) {
            OSA_error("Caller provided buffer size %u is smaller than image size %u.\n", *pLen, CVC_gpCvOut->encodedFaceAreaLen);
            ret = OSA_STATUS_ENOMEM;
        }
        else {
            *pPossibility = CVC_gpCvOut->livingPossibility;
            if (*pPossibility >= CVC_gpCvOut->livingFaceThreshold && CVC_gpCvOut->encodedFaceAreaLen > 0) {
                *pLen = CVC_gpCvOut->encodedFaceAreaLen;                                                        /* fill with actual length */
                memcpy(pFace, (void *)((size_t)CVC_gpCvOut + CVC_gpCvOut->encodedFaceAreaOffset), *pLen);       /* make a copy, in case the memory is overwritten */
            }
            else {
                *pLen = 0;
            }
            CVC_gpCvOut->livingFaceDeterminationFresh = 0;
            ret = OSA_STATUS_OK;
        }
    }
    else {
        ret = OSA_STATUS_EAGAIN;
    }
    pthread_rwlock_unlock(&CVC_gpCvOut->livingFaceResultLock);

    if (OSA_isFailed(ret)) {
        OSA_info("Determing living face failed with %d.\n", ret);
    }
    else {
        OSA_info("Determing living face possibility %d; encoded face len %u.\n", *pPossibility, *pLen);
    }

    return ret;
}


int CVC_commRequestCalibratorOperation(const int operation, const void *pArg)
{
    int ret;
    

    if (operation == CVD_CMD_INIT_CALIBRATION && NULL == pArg) {
        OSA_error("Init request shall provide pattern size.\n");
        return OSA_STATUS_EINVAL;
    }

    OSA_info("Will issue a calibrator operation request %d.\n", operation);

    /* send the operation request */
    OSA_debug("CVC waiting for request lock.\n");
    pthread_mutex_lock(&CVC_gpCvOut->calibratorRequestLock);    
    OSA_debug("CVC got request lock.\n");
    CVC_gpCvOut->calibratorOperation = operation;
    if (operation == CVD_CMD_INIT_CALIBRATION) {
        CVC_gpCvOut->calibratorPatternSize = *(OSA_Size *)pArg;
    }
#if CVD_USE_COND_VAR
    pthread_cond_signal(&CVC_gpCvOut->calibratorRequestCond);
    OSA_debug("CVC signaled request lock.\n");
#endif    
    pthread_mutex_unlock(&CVC_gpCvOut->calibratorRequestLock);
    OSA_debug("CVC unlocked request lock.\n");

    /* wait for the operation result */
    pthread_mutex_lock(&CVC_gpCvOut->calibratorResponseLock);
    while (!CVC_gpCvOut->calibratorOperationDone) {
#if CVD_USE_COND_VAR
        ret = pthread_cond_wait(&CVC_gpCvOut->calibratorResponseCond, &CVC_gpCvOut->calibratorResponseLock);
        OSA_info("CVC wait end, returned %d.\n", ret);
#else
        pthread_mutex_unlock(&CVC_gpCvOut->calibratorResponseLock);
        OSA_msleep(1000);
        pthread_mutex_lock(&CVC_gpCvOut->calibratorResponseLock);
#endif
    }
    ret = CVC_gpCvOut->calibratorOperationStatus;
    CVC_gpCvOut->calibratorOperation = CVD_CMD_NONE;
    CVC_gpCvOut->calibratorOperationDone = 0;
    pthread_mutex_unlock(&CVC_gpCvOut->calibratorResponseLock);

    OSA_info("Calibrator operation %d result %d.\n", operation, ret);
    return ret;
}
