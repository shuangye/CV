#pragma once
#ifndef __CVC_H__
#define __CVC_H__

/*
* Created by Liu Papillon, on Nov 14, 2017.
*/


#include <osa/osa.h>


#ifdef __cplusplus
extern "C" {
#endif      
   
    int CVC_init();

    int CVC_deinit();

    int CVC_calibratorInit(const int patternW, const int patternH);

    int CVC_calibratorCollectImage();

    int CVC_calibratorStereoCalibrate();

    int CVC_detectFace(const int id, OSA_Rect *pFace, OSA_Size *pRelativeSize);

    /* threshold 0~100% */
    int CVC_setLivingFaceThreshold(const int threshold);

    int CVC_determineLivingFace(int *pPossibility, void *pFace, size_t *pLen);
    
#ifdef __cplusplus
}
#endif

#endif  /* __CVC_H__ */
