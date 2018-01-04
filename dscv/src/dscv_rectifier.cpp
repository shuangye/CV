#include <iosfwd>
#include <stdio.h>
#include <osa/osa.h>
#include <dscv/dscv_rectifier.hpp>
#include <dscv/dscv_calibrator.hpp>



/************************************** Rectifier ********************************************/

#pragma region Rectifier

Rectifier::Rectifier(const string calibrationPath, const cv::Size imageSize) : _isValid(false), _imageSize(imageSize)
{
    Mat cameraMatrix;
    Mat distCoeffs;


    _isValid = false;

    FileStorage fs(calibrationPath, FileStorage::READ);
    if (!fs.isOpened()) {
        OSA_error("Failed to open calibration file %s.\n", calibrationPath.c_str());
        fs.release();
        return;
    }

    fs[Calibrator::kCameraMatrixKey] >> cameraMatrix;
    fs[Calibrator::kDistCoeffsKey] >> distCoeffs;
    fs.release();

    if (cameraMatrix.empty() || distCoeffs.empty()) {
        OSA_error("Calibration info is incomplete.\n");
        return;
    }

    cv::initUndistortRectifyMap(
        cameraMatrix,   /* 校准时得到的相机内参数矩阵   */
        distCoeffs,     /* 校准时得到的畸变系数矩阵     */
        cv::Mat(),      /* 可选矫正项（无）             */
        cv::Mat(),      /* 生成无畸变的相机矩阵         */
        imageSize,      /* 无畸变图像的尺寸             */
        CV_32FC1,       /* 输出图片的类型               */
        map1, map2);    /* x和y映射功能                 */

    _isValid = true;
}


int Rectifier::rectify(const Mat image, Mat &rectifiedImage)
{
    if (!_isValid) {
        OSA_error("This rectifier is not initialized properly.\n");
        return OSA_STATUS_EPERM;
    }

    if (image.cols != _imageSize.width || image.rows != _imageSize.height) {
        OSA_error("This rectifier is initialized for image size %dx%d, but actual image size is %dx%d.\n",
            _imageSize.width, _imageSize.height, image.cols, image.rows);
        return OSA_STATUS_EINVAL;
    }

    cv::remap(image, rectifiedImage, map1, map2, cv::INTER_LINEAR);
    return OSA_STATUS_OK;
}

#pragma endregion

/************************************** StereoRectifier ********************************************/

#pragma region StereoRectifier

StereoRectifier::StereoRectifier() : _isLeftRightReversed(0)
{
    setValidity(false);
}


int StereoRectifier::load(const string stereoCalibrationPath, const cv::Size imageSize)
{
    Mat lCameraMatrix, rCameraMatrix;
    Mat lDistCoeffs, rDistCoeffs;
    Mat R, T;
    

    _imageSize = imageSize;
    _isLeftRightReversed = 0;

    /* Read individal camera calibration information from saved XML file */
    FileStorage fs(stereoCalibrationPath, FileStorage::READ);
    if (!fs.isOpened()) {
        OSA_error("Failed to open stereo calibration file %s.\n", stereoCalibrationPath.c_str());
        fs.release();
        return OSA_STATUS_EINVAL;
    }

    fs[StereoCalibrator::kLCameraMatrixKey] >> lCameraMatrix;
    fs[StereoCalibrator::kLDistCoeffsKey] >> lDistCoeffs;
    fs[StereoCalibrator::kRCameraMatrixKey] >> rCameraMatrix;
    fs[StereoCalibrator::kRDistCoeffsKey] >> rDistCoeffs;
    fs[StereoCalibrator::kRKey] >> R;
    fs[StereoCalibrator::kTKey] >> T;
    fs[StereoCalibrator::kIsLeftRightReversedKey] >> _isLeftRightReversed;
    fs.release();

    if (lCameraMatrix.empty() || rCameraMatrix.empty() || lDistCoeffs.empty() || rDistCoeffs.empty() || R.empty() || T.empty()) {
        OSA_error("Stereo calibration info is incomplete.\n");
        return OSA_STATUS_EINVAL;
    }

    /* Calculate transforms for rectifying images */
    Mat Rl, Rr, Pl, Pr;
    stereoRectify(lCameraMatrix, lDistCoeffs, rCameraMatrix, rDistCoeffs, imageSize, R, T, Rl, Rr, Pl, Pr, _Q);

    /* Calculate pixel maps for efficient rectification of images via lookup tables */
    initUndistortRectifyMap(lCameraMatrix, lDistCoeffs, Rl, Pl, imageSize, CV_16SC2, _mapL1, _mapL2);
    initUndistortRectifyMap(rCameraMatrix, rDistCoeffs, Rr, Pr, imageSize, CV_16SC2, _mapR1, _mapR2);

    setValidity(true);
    return OSA_STATUS_OK;
}


int StereoRectifier::getQ(Mat &Q)
{
    if (!getValidity()) {
        return OSA_STATUS_EPERM;
    }

    Q = _Q;
    return OSA_STATUS_OK;
}


int StereoRectifier::rectify(const Mat lImage, const Mat rImage, Mat &lRectifiedImage, Mat &rRectifiedImage, Mat *pCombined)
{
    if (!getValidity()) {
        OSA_error("This stereo rectifier is not yet initialized properly.\n");
        return OSA_STATUS_EPERM;
    }

    if (lImage.cols != _imageSize.width || lImage.rows != _imageSize.height || rImage.size() != lImage.size()) {
        OSA_error("This stereo rectifier is initialized for image size %dx%d, but actual left image size %dx%d, right %dx%d.\n",
            _imageSize.width, _imageSize.height, lImage.cols, lImage.rows, rImage.cols, rImage.rows);
        return OSA_STATUS_EINVAL;
    }

#if 1
    /* Remap images by pixel maps to rectify */
    remap(lImage, lRectifiedImage, _mapL1, _mapL2, INTER_LINEAR);
    remap(rImage, rRectifiedImage, _mapR1, _mapR2, INTER_LINEAR);
#else
    lRectifiedImage = lImage;
    rRectifiedImage = rImage;
#endif


#if 1
    if (_isLeftRightReversed) {
        cv::swap(lRectifiedImage, rRectifiedImage);
    }
#endif

    if (NULL != pCombined) {
        /* Make a larger image containing the left and right rectified images side-by-side */
        Mat combo(lImage.rows, 2 * lImage.cols, CV_8UC3);
        lRectifiedImage.copyTo(combo(Range::all(), Range(0, lImage.cols)));
        rRectifiedImage.copyTo(combo(Range::all(), Range(lImage.cols, 2 * lImage.cols)));

        /* Draw horizontal red lines in the combo image to make comparison easier */
        for (int y = 0; y < combo.rows; y += 20) {
            line(combo, Point(0, y), Point(combo.cols, y), Scalar(0, 0, 255));
        }

        *pCombined = combo;
    }

    return OSA_STATUS_OK;
}

#pragma endregion
