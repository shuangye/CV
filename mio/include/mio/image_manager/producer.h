#pragma once
#ifndef __MIO_IMAGE_MANAGER_IMAGE_PRODUCER_H__
#define __MIO_IMAGE_MANAGER_IMAGE_PRODUCER_H__

/*
* Created by Liu Papillon, on Dec 19, 2017.
*/

#include <dscv/dscv.h>


#ifdef __cplusplus
extern "C" {
#endif    

    typedef Handle MIO_ImageManager_ProducerHandle;

    typedef struct MIO_ImageManager_ProducerFormat {
        OSA_Size              frameSize;
        Int32                 frameType;  /* of type `DSCV_FrameType` */
    } MIO_ImageManager_ProducerFormat;


    int MIO_imageManager_producerInit(const int producerId, MIO_ImageManager_ProducerHandle *pHandle);

    int MIO_imageManager_producerDeinit(MIO_ImageManager_ProducerHandle handle);

    int MIO_imageManager_producerOpen(MIO_ImageManager_ProducerHandle handle);

    int MIO_imageManager_producerClose(MIO_ImageManager_ProducerHandle handle);

    int MIO_imageManager_producerSetFormat(MIO_ImageManager_ProducerHandle handle, const MIO_ImageManager_ProducerFormat *pFormat);

    int MIO_imageManager_producerSetFrameRate(MIO_ImageManager_ProducerHandle handle, const unsigned int frameRate);
        
    int MIO_imageManager_producerStartStreaming(MIO_ImageManager_ProducerHandle handle);

    int MIO_imageManager_producerStopStreaming(MIO_ImageManager_ProducerHandle handle);

    int MIO_imageManager_producerIsStreaming(const MIO_ImageManager_ProducerHandle handle);

    int MIO_imageManager_producerGetFrame(MIO_ImageManager_ProducerHandle handle, DSCV_Frame *pFrame);

    int MIO_imageManager_producerPutFrame(MIO_ImageManager_ProducerHandle handle, const DSCV_Frame *pFrame);

#ifdef __cplusplus
}
#endif


#endif  /* __MIO_IMAGE_MANAGER_IMAGE_PRODUCER_H__ */
