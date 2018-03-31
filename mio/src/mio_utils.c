/*
* Created by Liu Papillon, on Mar 28, 2018.
*/


#include <mio/mio_utils.h>

/* DSCV_calcFrameLen */
size_t MIO_UTL_calcFrameDataLen(const DSCV_FrameType type, const OSA_Size size)
{
    size_t len = 0;

    switch (type) {
    case DSCV_FRAME_TYPE_NV21:
    case DSCV_FRAME_TYPE_NV12:
        len = size.w * size.h / 2 * 3;
        break;
    case DSCV_FRAME_TYPE_YUV422:
    case DSCV_FRAME_TYPE_YUYV:
    case DSCV_FRAME_TYPE_JPG:  /* give max since the length of every JPG frame varies */
        len = size.w * size.h * 2;
        break;
    default:
        OSA_error("The frame format %d is not implemented yet.\n", type);
        len = 0;
        break;
    }

    return len;
}
