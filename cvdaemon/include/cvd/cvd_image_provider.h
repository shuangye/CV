#pragma once
#ifndef __CVD_IMAGE_PROVIDER_H__
#define __CVD_IMAGE_PROVIDER_H__

/*
* Created by Liu Papillon, on Nov 15, 2017.
*
* This header exports C APIs.
*/


#include <osa/osa.h>
#include "dscv/dscv.h"


#define CVD_IMAGE_PROVIDER_COUNT                        2
#define CVD_IMAGE_PROVIDER_DEVICE_PATH                  "/dev/mediamemdev"
#define CVD_IMAGE_PROVIDER_MEMORY_LEN                   (64 * (1 << 20))        /* 64MB, exclusively for CV algorithms */
#define CVD_IMAGE_PROVIDER_MEMORY_MAGIC                 (((Uint64)'C') << (7 * 8) | ((Uint64)'V') << (6 * 8) | ((Uint64)'D') << (5 * 8) | ((Uint64)'_') << (4 * 8) | ((Uint64)'I') << (3 * 8) | ((Uint64)'M') << (2 * 8) | ((Uint64)'G') << (1 * 8) | ((Uint64)'.'))    /* "CVD_IMG." */


#ifdef __cplusplus
extern "C" {
#endif      

    typedef struct CVD_ImageProviderConfig {
        Uint32                     isProducer : 1;
        Uint32                     padding1 : 31;
        Int32                      frameFormat;    /* value of DSCV_FrameType */
        OSA_Size                   frameSize;
    } CVD_ImageProviderConfig;

    typedef void* CVD_ImageProviderHandle;

    int CVD_imageProviderInit(const CVD_ImageProviderConfig *pConfig, CVD_ImageProviderHandle *pHandle);

    int CVD_imageProviderDeinit(CVD_ImageProviderHandle handle);

    /* a copy of the frame is made */
    int CVD_imageProviderWriteFrame(CVD_ImageProviderHandle handle, const int id, const DSCV_Frame *pFrame);

    /* the caller is not needed to provide a piece of memory, only a pointer is needed. The pointer to the memory should NOT be freed after using. */
    int CVD_imageProviderReadFrame(CVD_ImageProviderHandle handle, const int id, DSCV_Frame *pFrame);
        
#ifdef __cplusplus
}
#endif


#endif  /* __CVD_IMAGE_PROVIDER_H__ */
