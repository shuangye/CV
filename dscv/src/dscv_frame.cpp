#include <osa/osa.h>
#include <dscv/dscv_frame.hpp>
#include <dscv/dscv_frame.h>



int DSCV_matFromFrame(const DSCV_Frame *pFrame, Mat &dst)
{
    int w, h;
    int frameSize;                         /* in bytes */
    int pixelFormat;
    int cvTypeCode;                          /* type code in OpenCV */
    Mat mat;
    OSA_Size yuvSize;
    /* std::vector<Uchar> const *pInput = NULL; */
    std::vector<Uchar> input;


    if (NULL == pFrame || NULL == pFrame->pData) {
        OSA_error("Invalid parameter.\n");
        return OSA_STATUS_EINVAL;
    }

    yuvSize = pFrame->size;
    switch (pFrame->type) {
    case DSCV_FRAME_TYPE_OPENCV_MAT:
        dst = *((Mat *)pFrame->pData);
        return OSA_STATUS_OK;
        break;
    case DSCV_FRAME_TYPE_JPG:
        /* pInput = static_cast<std::vector<Uchar> const*>(pFrame->pData); */
        input = std::vector<Uchar>((Uchar *)pFrame->pData, (Uchar *)pFrame->pData + pFrame->dataLen);
        mat = cv::imdecode(input, CV_LOAD_IMAGE_COLOR);
        if (mat.empty()) {
            return OSA_STATUS_EINVAL;
        }
        dst = mat;
        return OSA_STATUS_OK;
        break;
    case DSCV_FRAME_TYPE_YUV422:        
    case DSCV_FRAME_TYPE_YUYV:
        cvTypeCode = CV_YUV2BGR_YUYV;
        pixelFormat = CV_8UC2;
        w = yuvSize.w;
        h = yuvSize.h;
        frameSize = yuvSize.w * yuvSize.h * 2;
        break;
    case DSCV_FRAME_TYPE_NV21:  /* YUV420P, CV_YUV2BGR_I420, CV_YUV2BGR_NV21 */
    case DSCV_FRAME_TYPE_NV12:
        cvTypeCode = CV_YUV2BGR_I420;
        pixelFormat = CV_8UC1;
        w = yuvSize.w;
        h = yuvSize.h / 2 * 3;
        frameSize = yuvSize.w * yuvSize.h / 2 * 3;
        break;
    default:
        fprintf(stderr, "Unsupported YUV type %d\n", pFrame->type);
        return OSA_STATUS_EINVAL;
        break;
    }

    /* the following is for YUV */

    if (frameSize != pFrame->dataLen) {
        OSA_error("Expected frame data length %d, actual %u.\n", frameSize, pFrame->dataLen);
        return OSA_STATUS_EINVAL;
    }

    Mat mYUV(h, w, pixelFormat, pFrame->pData);
    mat = Mat(yuvSize.h, yuvSize.w, CV_8UC3);  /* define the dimensions of your RGB Mat before you convert. */
    cvtColor(mYUV, mat, cvTypeCode, 3);    
    dst = mat;

    return OSA_STATUS_OK;
}


int DSCV_matFromFile(const string path, DSCV_Frame *pFrame, Mat &dst)
{
    int ret;
    Mat mat;

    if (NULL == pFrame) {
        return OSA_STATUS_EINVAL;
    }
    
    FILE *fp = fopen(path.c_str(), "rb");
    if (NULL == fp) {
        OSA_error("Failed to open file %s for reading.\n", path.c_str());
        return OSA_STATUS_EINVAL;
    }

    pFrame->dataLen = DSCV_calcFrameLen((DSCV_FrameType)pFrame->type, pFrame->size);
    pFrame->pData = malloc(pFrame->dataLen);
    fread(pFrame->pData, 1, pFrame->dataLen, fp);
    fclose(fp);

    ret = DSCV_matFromFrame(pFrame, mat);
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to convert YUV to mat: %d.\n", ret);
        return ret;
    }

    dst = mat;
    
    free(pFrame->pData);
    pFrame->pData = NULL;
    return ret;
}


/* if you want to dump the encoded file, please write encodedImage.data() with binary mode */
int DSCV_encodeMat(const Mat &mat, const int codec, vector<Uchar> &encodedImage)
{
    bool succeeded;
    const char *pExt = NULL;


    if (mat.empty()) {
        return OSA_STATUS_EINVAL;
    }

    switch (codec) {
    case DSCV_FRAME_TYPE_JPG:
        pExt = ".jpg";
        break;
    case DSCV_FRAME_TYPE_PNG:
        pExt = ".png";
        break;
    default:
        return OSA_STATUS_EINVAL;
        break;
    }

    succeeded = cv::imencode(pExt, mat, encodedImage);
    return succeeded ? OSA_STATUS_OK : OSA_STATUS_EGENERAL;
}
