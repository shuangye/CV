#include <dscv/dscv_frame.h>



size_t DSCV_calcFrameLen(const DSCV_FrameType type, const OSA_Size size)
{
    size_t len = 0;

    switch (type) {
    case DSCV_FRAME_TYPE_NV21:
    case DSCV_FRAME_TYPE_NV12:
        len = size.w * size.h / 2 * 3;
        break;
    case DSCV_FRAME_TYPE_YUV422:
    case DSCV_FRAME_TYPE_YUYV:
        len = size.w * size.h * 2;
        break;
    default:
        OSA_error("The frame format %d is not implemented yet.\n", type);
        len = 0;
        break;
    }

    return len;
}
