#pragma once
#ifndef __CVD_H__
#define __CVD_H__

/*
* Created by Liu Papillon, on Nov 14, 2017.
*
* This header exports C APIs.
*/

#include <pthread.h>
#include <osa/osa.h>

#define CVD_ALGO_DEV_FILE                    "/dev/algo"
#define CVD_ALGO_MEM_LEN                     ((Uint32)(1 << 20))                     /* 1MB */
#define CVD_ALGO_MEM_MAGIC                   ((Uint32)'C' << (8 * 3) | (Uint32)'V' << (8 * 2) | (Uint32)'D' << (8 * 1) | (Uint32)'M')
#define CVD_FACE_DETECTION_COUNT             2
#define CVD_ENCODED_FACE_AREA_MAX_LEN        (CVD_ALGO_MEM_LEN - sizeof(CVD_CvOut))
#define CVD_USE_COND_VAR                     0


#ifdef __cplusplus
extern "C" {
#endif      

    typedef enum CVD_Command {
        CVD_CMD_NONE                             = 0,
        CVD_CMD_DETERMINE_LIVING_FACE            = 1,
        CVD_CMD_DETECT_FACE                      = 2,
        CVD_CMD_INIT_CALIBRATION                 = 3,    /* chessboard pattern size */
        CVD_CMD_COLLECT_CALIBRATION_IMAGE        = 4,    /* no message body */
        CVD_CMD_STEREO_CALIBRATE                 = 5,    /* no message body */
        CVD_CMD_DETERMINE_COUNT,
    } CVD_Command;


    typedef struct CVD_Message {
        Int32                                    status;
        union {
            OSA_Rect                             face;
            Int32                                possibility;
            OSA_Size                             calibrationPatternSize;
        } body;
    } CVD_Message;


    typedef struct CVD_CvOut {
        Uint32                     magic;

        Uint32                     inited                                   : 1;
        Uint32                     faceDetectionEnabled                     : 1;    /* maybe cond var is better? */
        Uint32                     livingFaceDeterminationEnabled           : 1;
        Uint32                     livingFaceDeterminationBusy              : 1;        
        Uint32                     livingFaceDeterminationFresh             : 1;
        Uint32                     calibratorOperationDone                  : 1;
        Uint32                     padding1                                 : 26;

        pthread_rwlock_t           faceDetectionControltLock;        
        pthread_rwlock_t           faceDetectionResultLock;
        pthread_rwlock_t           livingFaceControlLock;
        pthread_rwlock_t           livingFaceResultLock;
        OSA_Rect                   faces[CVD_FACE_DETECTION_COUNT];                /* face detection result */
        Uint8                      faceDetectionFresh[CVD_FACE_DETECTION_COUNT];   /* face detection result */
        Uint8                      padding2[2];
        OSA_Size                   faceDetectionInputImageSize;
        Int32                      livingFaceThreshold;                            /* living face determination configuration */
        Int32                      livingPossibility;                              /* living face determination result */
        Uint32                     encodedFaceAreaOffset;                          /* living face determination result: memory offset relative to the beginning of this memory region */
        Uint32                     encodedFaceAreaLen;                             /* living face determination result */

        pthread_mutex_t            calibratorRequestLock;
        pthread_cond_t             calibratorRequestCond;
        pthread_mutex_t            calibratorResponseLock;
        pthread_cond_t             calibratorResponseCond;
        Int32                      calibratorOperation;                            /* of type CVD_Command */
        OSA_Size                   calibratorPatternSize;
        Int32                      calibratorOperationStatus;
    } CVD_CvOut;

        
#ifdef __cplusplus
}
#endif


#endif  /* __CVD_H__ */
