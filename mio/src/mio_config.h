#pragma once
#ifndef __MIO_CONFIG_H__
#define __MIO_CONFIG_H__

/*
* Created by Liu Papillon, on Nov 11, 2017, the single day.
*/

#include <osa/osa.h>


#ifdef OSA_OS_ANDROID
#define CVIO_OPENCV_SUPPORTS_GUI              0
#else
#define CVIO_OPENCV_SUPPORTS_GUI              1
#endif


#define CVIO_CAPTURED_FRAME_PATH              "frame.jpg"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#ifdef __cplusplus
}
#endif // __cplusplus

#endif  /*  */

