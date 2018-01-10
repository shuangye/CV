#pragma once
#ifndef __DS_FACE_H__
#define __DS_FACE_H__

/*
* Created by Liu Papillon, on Nov 11, 2017, the single day.
*
* This header exports C APIs.
*/


#include <osa/osa.h>
#include <dscv/dscv.h>
#include "face_definitions.h"


#ifdef __cplusplus
extern "C" {
#endif       

    FACE_API int FACE_init(FACE_Options *pOptions, FACE_Handle *pHandle);

    FACE_API int FACE_deinit(FACE_Handle handle);

    FACE_API int FACE_detectFace(FACE_Handle handle, DSCV_Frame *pFrame, OSA_Rect *pFace);

    FACE_API int FACE_determineLiving(FACE_Handle handle, DSCV_Frame *pFrame1, DSCV_Frame *pFrame2, OSA_Rect *pFace1, OSA_Rect *pFace2, Int32 *pPossibility);
    
    FACE_API int FACE_detectAndDetermineLiving(FACE_Handle handle, DSCV_Frame *pFrame1, DSCV_Frame *pFrame2, Int32 *pPossibility);

#ifdef __cplusplus
}
#endif


#endif // __DS_FACE_H__
