#pragma once

#ifndef __MIO_MJPEG_DECODER_H__
#define __MIO_MJPEG_DECODER_H__

/*
* Created by Liu Papillon, on Mar 28, 2018.
*/

#include <osa/osa.h>


typedef struct MIO_MJPEGDECODER_Options {
    OSA_Size                         frameSize;
} MIO_MJPEGDECODER_Options;


typedef Handle                          MIO_MJPEGDECODER_Handle;


int MIO_MJPEGDECODER_init(const MIO_MJPEGDECODER_Options *pOptions, MIO_MJPEGDECODER_Handle *pHandle);

int MIO_MJPEGDECODER_deinit(const MIO_MJPEGDECODER_Handle handle);

int MIO_MJPEGDECODER_decodedFrameDataLen(const MIO_MJPEGDECODER_Handle handle, size_t *pDataLen);

int MIO_MJPEGDECODER_decodeToNV12(const MIO_MJPEGDECODER_Handle handle, const DSCV_Frame *pSrcFrame, DSCV_Frame *pDstFrame);

#endif // !__MIO_MJPEG_DECODER_H__

