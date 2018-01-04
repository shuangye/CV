#pragma once
#ifndef __DSCV_DEFINITIONS_H__
#define __DSCV_DEFINITIONS_H__

/*
* Created by Liu Papillon, on Nov 11, 2017, the single day.
*/


#include <osa/osa.h>

#ifdef OSA_OS_WINDOWS
#ifdef DSCV_EXPORTS  
#define DSCV_API __declspec(dllexport)   
#else  
#define DSCV_API __declspec(dllimport)   
#endif 
#else  /* OSA_OS_WINDOWS */
#define DSCV_API
#endif  /* OSA_OS_WINDOWS */


#define DSCV_HARD_LEAST_CALIBRATION_IMAGES_COUNT                 1
#define DSCV_SOFT_LEAST_CALIBRATION_IMAGES_COUNT                 20


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#ifdef __cplusplus
}
#endif // __cplusplus

#endif  /* __DSCV_DEFINITIONS_H__ */
