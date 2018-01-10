#pragma once
#ifndef __MIO_IMAGE_MANAGER_MEMORY_H__
#define __MIO_IMAGE_MANAGER_MEMORY_H__

/*
* Created by Liu Papillon, on Dec 19, 2017.
*
* This header exports C APIs.
*/


#include <osa/osa.h>


#define MIO_IMAGE_MANAGER_MEM_LEN                          ((size_t)(64 * OSA_MB))


#ifdef __cplusplus
extern "C" {
#endif    


    int MIO_imageManager_memInit(void **ppMem);

    int MIO_imageManager_memDeinit(const void *pMemRegion);


#ifdef __cplusplus
}
#endif

#endif  /* __MIO_IMAGE_MANAGER_MEMORY_H__ */
