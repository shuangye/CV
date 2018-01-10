#pragma once
#ifndef __MIO_DEFINITIONS_H__
#define __MIO_DEFINITIONS_H__

/*
* Created by Liu Papillon, on Nov 11, 2017, the single day.
*/


#include <osa/osa.h>

#ifdef OSA_OS_WINDOWS
#ifdef CVIO_EXPORTS  
#define MIO_API __declspec(dllexport)   
#else  
#define MIO_API __declspec(dllimport)   
#endif 
#else  /* OSA_OS_WINDOWS */
#define MIO_API
#endif  /* OSA_OS_WINDOWS */


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#ifdef __cplusplus
}
#endif // __cplusplus

#endif  /* __MIO_DEFINITIONS_H__ */
