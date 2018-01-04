#pragma once
#ifndef __DSCV_FRAME_H__
#define __DSCV_FRAME_H__

/*
* Created by Liu Papillon, on Dec 19, 2017.
*/


#include "dscv_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif    

    typedef enum DSCV_FrameType {
        DSCV_FRAME_TYPE_NV21                   = 0,
        DSCV_FRAME_TYPE_NV12                   = 1,
        DSCV_FRAME_TYPE_YUV422                 = 2,
        DSCV_FRAME_TYPE_YUYV                   = 3,
        DSCV_FRAME_TYPE_OPENCV_MAT             = 4,
        DSCV_FRAME_TYPE_JPG                    = 5,
        DSCV_FRAME_TYPE_PNG                    = 6,
        DSCV_FRAME_TYPE_COUNT                     ,
    } DSCV_FrameType;

    typedef struct DSCV_Frame {
        Int32                      type;    /* of type `DSCV_FrameType` */
        OSA_Size                   size;
        void                      *pData;
        Uint32                     dataLen;
        Uint32                     index;
        Uint64                     timestamp;
    } DSCV_Frame;


    DSCV_API size_t DSCV_calcFrameLen(const DSCV_FrameType type, const OSA_Size size);


#ifdef __cplusplus
}
#endif

#endif // !__DSCV_FRAME_H__
