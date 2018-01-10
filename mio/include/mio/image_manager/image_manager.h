#pragma once
#ifndef __MIO_IMAGE_MANAGER_H__
#define __MIO_IMAGE_MANAGER_H__

/*
* Created by Liu Papillon, on Dec 19, 2017.
*
* This header exports C APIs.

This image manger manages a memory region, which contains images from 'producers', 
and can be read by one or more 'consumers'.
When designing, the following ideas are considered:
 - To efficiently manage the memory region, every frame of image is considered to be in same format, and of the same size.
 - Currently, for each image provider, the memory region is used as a circular buffer. The oldest memory region is overwritten
   by the producer, regardless of whether being used by a consumer or not. However, this is not a good design, since a
   consumer (e.g., a computer vision algorithm) may use a frame for a long period.
 - For each producer, there may be more than one corresponding consumers. That is, one frame from one producer may be
   distributed to multiple places, each place is called a distribution.
*/


#include <osa/osa.h>
#include <dscv/dscv.h>
#include "config.h"
#include "memory.h"
#include "producer.h"
#include "consumer.h"


#define MIO_IMAGE_MANAGER_MAX_DISTRIBUTIONS_COUNT         MIO_IMAGE_MANAGER_CONSUMER_ROLES_COUNT


typedef struct MIO_ImageManager_Config {
    Uint32                     isProducer : 1;
    Uint32                     padding1 : 31;
    Int32                      frameFormat;    /* value of DSCV_FrameType */
    OSA_Size                   frameSize;
} MIO_ImageManager_Config;

typedef Handle MIO_ImageManager_Handle;


#ifdef __cplusplus
extern "C" {
#endif    

    int MIO_imageManager_init(const MIO_ImageManager_Config *pConfig, MIO_ImageManager_Handle *pHandle);

    int MIO_imageManager_deinit(MIO_ImageManager_Handle handle);

    int MIO_imageManager_reset(MIO_ImageManager_Handle handle, const int producerId);

    int MIO_imageManager_writeFrame(MIO_ImageManager_Handle handle, const int producerId, const DSCV_Frame *pFrame);

    int MIO_imageManager_readFrame(MIO_ImageManager_Handle handle, const int producerId, const int consumerRole, DSCV_Frame *pFrame);

    int MIO_imageManager_releaseFrame(MIO_ImageManager_Handle handle, const int producerId, const int consumerRole, const DSCV_Frame *pFrame);


#ifdef __cplusplus
}
#endif

#endif  /* __MIO_IMAGE_MANAGER_H__ */
