/*
* Created by Liu Papillon, on Nov 11, 2017, the single day.
*
* This a C++ file, the implementation of <face/face.h>
*/



#include <opencv2/opencv.hpp>
#include <dscv/dscv.hpp>
#include <face/face.h>                /* prototypes of the functions defined in this file */
#include <face/face.hpp>              /* C++ APIs which will be used by the implementation of this file */

using namespace std;
using namespace cv;



int FACE_init(FACE_Options *pOptions, FACE_Handle *pHandle)
{
    if (NULL == pOptions || NULL == pHandle) {
        OSA_error("Invalid parameters.\n");
        return OSA_STATUS_EINVAL;
    }

    return FACE_init(*pOptions, *pHandle);
}


int FACE_deinit(FACE_Handle handle)
{
    return FACE_deinitialize(handle);
}


int FACE_detectFace(FACE_Handle handle, DSCV_Frame *pFrame, OSA_Rect *pFace)
{
    int ret;
    Mat mat;
    cv::Rect face;


    if (NULL == pFrame || NULL == pFace) {
        OSA_error("Invalid parameters.\n");
        return OSA_STATUS_EINVAL;
    }

    ret = DSCV_matFromFrame(pFrame, mat);
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to parse frame.\n");
        return ret;
    }

    ret = FACE_detectFace(handle, mat, face);
    if (OSA_isFailed(ret)) {
        return ret;
    }

    pFace->origin.x = face.x;
    pFace->origin.y = face.y;
    pFace->size.w = face.width;
    pFace->size.h = face.height;
    return ret;
}


int FACE_determineLiving(FACE_Handle handle, DSCV_Frame *pFrame1, DSCV_Frame *pFrame2, OSA_Rect *pFace1, OSA_Rect *pFace2, Int32 *pPossibility)
{
    int ret;
    Mat mat1, mat2;
    cv::Rect face1, face2;


    if (NULL == pFrame1 || NULL == pFrame2 || NULL == pFace1 || NULL == pFace2 || NULL == pPossibility) {
        OSA_error("Invalid parameters.\n");
        return OSA_STATUS_EINVAL;
    }

    *pPossibility = FACE_LIVING_FACE_IMPOSSIBLE;

    ret = DSCV_matFromFrame(pFrame1, mat1);
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to parse frame 1.\n");
        return ret;
    }

    ret = DSCV_matFromFrame(pFrame2, mat2);
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to parse frame 2.\n");
        return ret;
    }

    face1.x      = pFace1->origin.x;
    face1.y      = pFace1->origin.y;
    face1.width  = pFace1->size.w;
    face1.height = pFace1->size.h;
    face2.x      = pFace2->origin.x;
    face2.y      = pFace2->origin.y;
    face2.width  = pFace2->size.w;
    face2.height = pFace2->size.h;
    Int32 &possibility = *pPossibility;
    ret = FACE_determineLiving(handle, mat1, mat2, face1, face2, possibility);
    if (OSA_isSucceeded(ret)) {
        *pPossibility = possibility;
    }
    
    return ret;
}


int FACE_detectAndDetermineLiving(FACE_Handle handle, DSCV_Frame *pFrame1, DSCV_Frame *pFrame2, Int32 *pPossibility)
{
    int ret;
    Mat mat1, mat2;
    

    if (NULL == pFrame1 || NULL == pFrame2 || NULL == pPossibility) {
        OSA_error("Invalid parameters.\n");
        return OSA_STATUS_EINVAL;
    }

    *pPossibility = FACE_LIVING_FACE_IMPOSSIBLE;

    ret = DSCV_matFromFrame(pFrame1, mat1);
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to parse frame 1.\n");
        return ret;
    }

    ret = DSCV_matFromFrame(pFrame2, mat2);
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to parse frame 2.\n");
        return ret;
    }       

    Int32 &possibility = *pPossibility;
    ret = FACE_detectAndDetermineLiving(handle, mat1, mat2, possibility);
    if (OSA_isSucceeded(ret)) {
        *pPossibility = possibility;
    }

    return ret;
}
