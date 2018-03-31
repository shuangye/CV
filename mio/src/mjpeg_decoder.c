/*
* Created by Liu Papillon, on Mar 28, 2018.
*/

#include <stdlib.h>
#include <osa/osa.h>
#include <dscv/dscv.h>
#include <librkvpu/vpu_api.h>  /* for hardware JPEG -> NV12 decoder */
#include <mio/mio_utils.h>
#include <mio/mjpeg_decoder.h>


typedef struct MIO_MJPEGDECODER_Object {
    Uint32                                  isInited   : 1;
    Uint32                                  padding1   : 31;
    DS_MjpegDecoder                        *pDecoder;
    MIO_MJPEGDECODER_Options                options;
} MIO_MJPEGDECODER_Object;


int MIO_MJPEGDECODER_init(const MIO_MJPEGDECODER_Options *pOptions, MIO_MJPEGDECODER_Handle *pHandle)
{
    int ret = OSA_STATUS_OK;

    if (NULL == pOptions || NULL == pHandle) {
        return OSA_STATUS_EINVAL;
    }

    DS_MjpegDecoder *pDecoder = (DS_MjpegDecoder*)DS_GetMJPEGDecoder();
    if (NULL == pDecoder) {
        OSA_error("Failed to get MJPEG decoder.\n");
        return OSA_STATUS_EGENERAL;
    }

    ret = pDecoder->init(pDecoder, pOptions->frameSize.w, pOptions->frameSize.h);
    if (ret != 0){
        OSA_error("Failed to init MJPEG decoder.\n");
        return OSA_STATUS_EGENERAL;
    }
    
    MIO_MJPEGDECODER_Object *pObject = (MIO_MJPEGDECODER_Object *)calloc(1, sizeof(*pObject));
    if (NULL == pObject) {
        ret = OSA_STATUS_ENOMEM;
        goto _failure;
    }

    pObject->pDecoder = pDecoder;
    pObject->options = *pOptions;
    pObject->isInited = 1;
    *pHandle = (MIO_MJPEGDECODER_Handle)pObject;
    OSA_info("Inited MJPEG decoder.\n");
    return OSA_STATUS_OK;

_failure:
    if (NULL != pDecoder) {
        pDecoder->dinit(pDecoder);
    }
    if (NULL != pObject) {
        free(pObject);
    }
    return ret;
}


int MIO_MJPEGDECODER_deinit(const MIO_MJPEGDECODER_Handle handle)
{
    MIO_MJPEGDECODER_Object *pObject = (MIO_MJPEGDECODER_Object *)handle;
    if (NULL == pObject) {
        return OSA_STATUS_EINVAL;
    }

    if (!pObject->isInited) {
        return OSA_STATUS_OK;
    }

    if (NULL != pObject->pDecoder) {
        pObject->pDecoder->dinit(pObject->pDecoder);
        pObject->pDecoder = NULL;
    }
    
    pObject->isInited = 0;
    free(pObject);
    OSA_info("Deinited MJPEG decoder.\n");
    return OSA_STATUS_OK;
}


int MIO_MJPEGDECODER_decodedFrameDataLen(const MIO_MJPEGDECODER_Handle handle, size_t *pDataLen)
{
    MIO_MJPEGDECODER_Object *pObject = (MIO_MJPEGDECODER_Object *)handle;

    if (NULL == pObject || NULL == pDataLen) {
        return OSA_STATUS_EINVAL;
    }

    if (!pObject->isInited) {
        OSA_error("This module is not inited yet.\n");
        return OSA_STATUS_EPERM;
    }

    *pDataLen = pObject->options.frameSize.w * pObject->options.frameSize.h * 2;  // RK VPU MJPEG decoder always use this length
    return OSA_STATUS_OK;
}


int MIO_MJPEGDECODER_decodeToNV12(const MIO_MJPEGDECODER_Handle handle, const DSCV_Frame *pSrcFrame, DSCV_Frame *pDstFrame)
{
    MIO_MJPEGDECODER_Object *pObject = (MIO_MJPEGDECODER_Object *)handle;
    size_t dstFrameDataLen;
    int decodedFrameDataLen;
    

    if (NULL == pObject || NULL == pSrcFrame || NULL == pDstFrame) {
        return OSA_STATUS_EINVAL;
    }

    if (!pObject->isInited) {
        OSA_error("This module is not inited yet.\n");
        return OSA_STATUS_EPERM;
    }

    if (pSrcFrame->size.w != pObject->options.frameSize.w || pSrcFrame->size.h != pObject->options.frameSize.h) {
        OSA_error("Invalid frame size %dx%d; expected %dx%d.\n", pSrcFrame->size.w, pSrcFrame->size.h, pObject->options.frameSize.w, pObject->options.frameSize.h);
        return OSA_STATUS_EINVAL;
    }

    if (pSrcFrame->type != DSCV_FRAME_TYPE_JPG) {
        OSA_error("Source frame format %d is not JPEG.\n", pSrcFrame->type);
        return OSA_STATUS_EPERM;
    }

    dstFrameDataLen = MIO_UTL_calcFrameDataLen(DSCV_FRAME_TYPE_NV12, pSrcFrame->size);
    if (pDstFrame->dataLen < dstFrameDataLen) {
        OSA_error("Dest memory length %u is insufficient to store decoded frame of length %u.\n", pDstFrame->dataLen, dstFrameDataLen);
        return OSA_STATUS_EINVAL;
    }

    decodedFrameDataLen = pObject->pDecoder->decode(pObject->pDecoder, pSrcFrame->pData, pSrcFrame->dataLen, pDstFrame->pData);
    if (decodedFrameDataLen < dstFrameDataLen) {
        OSA_error("Decoding failed.\n");
        return OSA_STATUS_EGENERAL;
    }

    pDstFrame->type = DSCV_FRAME_TYPE_NV12;
    pDstFrame->size = pSrcFrame->size;
    pDstFrame->dataLen = dstFrameDataLen;
    pDstFrame->index = pSrcFrame->index;
    pDstFrame->timestamp = pSrcFrame->timestamp;
    return OSA_STATUS_OK;
}
