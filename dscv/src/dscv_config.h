#pragma once
#ifndef __DSCV_CONFIG_H__
#define __DSCV_CONFIG_H__

/*
* Created by Liu Papillon, on Nov 11, 2017, the single day.
*/

#include <osa/osa.h>


#ifdef OSA_OS_ANDROID
#define DSCV_OPENCV_SUPPORTS_GUI              0
#else
#define DSCV_OPENCV_SUPPORTS_GUI              1
#endif

#define DSCV_LEFT_CALIBRATION_PATH            "LeftCalibration.xml"
#define DSCV_STEREO_CALIBRATION_PATH          "StereoCalibration.xml"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#ifdef __cplusplus
}
#endif // __cplusplus

#endif  /* __DSCV_CONFIG_H__ */
