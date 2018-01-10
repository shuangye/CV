#pragma once
#ifndef __DS_CV_FACE_CONFIG_H__
#define __DS_CV_FACE_CONFIG_H__

/*
* Created by Liu Papillon, on Nov 13, 2017.
*/


#include <osa/osa.h>


#if defined(OSA_OS_ANDROID)
#define FACE_OPENCV_SUPPORTS_GUI              0
#define FACE_OPENCV_SUPPORT_NON_FREE          0
#define FACE_DETECTION_MODEL_PATH             "lbpcascade_frontalface.xml"
#elif defined(OSA_OS_GNU_LINUX)
#define FACE_OPENCV_SUPPORTS_GUI              0
#define FACE_OPENCV_SUPPORT_NON_FREE          1
#define FACE_DETECTION_MODEL_PATH             "lbpcascade_frontalface.xml"
#elif defined(OSA_OS_WINDOWS)
#define FACE_OPENCV_SUPPORTS_GUI              1
#define FACE_OPENCV_SUPPORT_NON_FREE          1
#define FACE_DETECTION_MODEL_PATH             "D:/CV/OpenCV2.4.13/sources/data/lbpcascades/lbpcascade_frontalface.xml"
#define FACE_STEREO_CALIBRATION_PATH          "StereoCalibration.xml"
#endif


#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif


#endif // __DS_CV_FACE_CONFIG_H__
