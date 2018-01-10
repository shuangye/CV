/*
* Created by Liu Papillon, on Nov 11, 2017, the single day.
*
* This a C++ file, the implementation of <face/face.hpp>
*/


#include <opencv2/opencv.hpp>
#include <dscv/dscv.hpp>
#include <face/face.hpp>
#include "face_pri.hpp"
#include "face_detect.hpp"
#include "face_living.hpp"

using namespace std;
using namespace cv;



int FACE_init(FACE_Options &options, FACE_Handle &handle)
{
    int                   ret;
    FACE_Module          *pObj = NULL;


    OSA_info("Module %s, built on %s, %s.\n", OSA_MODULE_NAME, __DATE__, __TIME__);
    
    pObj = (FACE_Module *)calloc(1, sizeof(*pObj));
    if (NULL == pObj) {
        return OSA_STATUS_ENOMEM;
    }

    pObj->options = options;

    pObj->pDetector = new Detector(options.faceDetectionModelPath);
    if (!pObj->pDetector->isValid()) {
        ret = OSA_STATUS_EINVAL;
        goto _failure;
    }

    pObj->pLiving = new Living();
    if (NULL == pObj->pLiving) {
        ret = OSA_STATUS_ENOMEM;
        goto _failure;
    }

    handle = pObj;
    OSA_info("Inited face module.\n");
    return OSA_STATUS_OK;

_failure:
    FACE_Handle _handle = (FACE_Handle)pObj;
    FACE_deinitialize(_handle);
    return ret;
}


int FACE_deinitialize(FACE_Handle &handle)
{
    FACE_Module          *pObj = NULL;


    pObj = (FACE_Module *)handle;
    if (NULL != pObj) {
        if (NULL != pObj->pDetector) {
            delete pObj->pDetector;
            pObj->pDetector = NULL;
        }
        if (NULL != pObj->pLiving) {
            delete pObj->pLiving;
            pObj->pLiving = NULL;
        }
        free(pObj);
    }

    pObj = NULL;
    handle = NULL;

    OSA_info("Deinited face module.\n");
    return OSA_STATUS_OK;
}


int FACE_detectFace(FACE_Handle &handle, Mat image, cv::Rect &face)
{
    FACE_Module               *pModule;
    int64 t1, t2;


    pModule = (FACE_Module *)handle;
    if (NULL == pModule || NULL == pModule->pDetector) {
        OSA_error("Invalid parameter.\n");
        return OSA_STATUS_EINVAL;
    }

    t1 = getTickCount();
    face = pModule->pDetector->largestObject(image);
    t2 = getTickCount();

    OSA_debug("Consumed %.1fms.\n", (t2 - t1) * 1000 / getTickFrequency());
    OSA_debug("face %dx%d@(%d,%d).\n", face.width, face.height, face.x, face.y);

#if 0
    cv::rectangle(image, face, Scalar(255, 0, 0));
#endif

    return OSA_STATUS_OK;
}


int FACE_determineLiving(FACE_Handle &handle, Mat frame1, Mat frame2, cv::Rect face1, cv::Rect face2, Int32 &possibility)
{
    FACE_Module               *pModule;


    possibility = FACE_LIVING_FACE_IMPOSSIBLE;

    pModule = (FACE_Module *)handle;
    if (NULL == pModule || NULL == pModule->pDetector) {
        OSA_error("Invalid parameter.\n");
        return OSA_STATUS_EINVAL;
    }
        
    if (RectUtils::isZero(face1) || RectUtils::isZero(face2)) {
        return OSA_STATUS_OK;
    }

    return pModule->pLiving->determineLivingFace(frame1, frame2, face1, face2, possibility);    
}


int FACE_detectAndDetermineLiving(FACE_Handle &handle, Mat frame1, Mat frame2, Int32 &possibility)
{
    int                        ret;
    FACE_Module               *pModule;
    cv::Rect                   face1, face2;


    possibility = FACE_LIVING_FACE_IMPOSSIBLE;

    pModule = (FACE_Module *)handle;
    if (NULL == pModule || NULL == pModule->pDetector) {
        OSA_error("Invalid parameter.\n");
        return OSA_STATUS_EINVAL;
    }

    ret = FACE_detectFace(handle, frame1, face1);
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to detect face in frame 1.\n");
        return ret;
    }

    if (RectUtils::isZero(face1)) {
        return OSA_STATUS_OK;
    }

    ret = FACE_detectFace(handle, frame2, face2);
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to detect face in frame 2.\n");
        return ret;
    }

    if (RectUtils::isZero(face2)) {
        return OSA_STATUS_OK;
    }

    return FACE_determineLiving(handle, frame1, frame2, face1, face2, possibility);
}

