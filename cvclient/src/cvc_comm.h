#pragma once
#ifndef __CVC_COMM_H__
#define __CVC_COMM_H__

/*
* Created by Liu Papillon, on Nov 23, 2017.
*/

#include <osa/osa.h>


#ifdef __cplusplus
extern "C" {
#endif      

    int CVC_commInit();

    int CVC_commDeinit();

    int CVC_commGetDetectedFace(const int id, OSA_Rect *pFace, OSA_Size *pRelativeSize);

    int CVC_commSetLivingFaceThreshold(const int threshold);

    int CVC_commGetLivingFace(int *pPossibility, void *pFace, size_t *pLen);

    int CVC_commRequestCalibratorOperation(const int operation, const void *pArg);

#ifdef __cplusplus
}
#endif

#endif  /* __CVC_COMM_H__ */


