#pragma once
#ifndef __DS_FACE_DEFINITIONS_H__
#define __DS_FACE_DEFINITIONS_H__

/*
* Created by Liu Papillon, on Nov 11, 2017, the single day.
*/


#include <osa/osa.h>
#include <dscv/dscv.h>

#ifdef OSA_OS_WINDOWS
#ifdef FACE_EXPORTS  
#define FACE_API __declspec(dllexport)   
#else  
#define FACE_API __declspec(dllimport)   
#endif 
#else  /* OSA_OS_WINDOWS */
#define FACE_API
#endif  /* OSA_OS_WINDOWS */


#define FACE_LIVING_FACE_IMPOSSIBLE              0


typedef void *  FACE_Handle;

typedef struct FACE_Options {
    Char                       faceDetectionModelPath[128];
    Uint32                     reserved[16];
} FACE_Options;


#ifdef __cplusplus
extern "C" {
#endif
    

#ifdef __cplusplus
}
#endif


#endif // __DS_FACE_DEFINITIONS_H__
