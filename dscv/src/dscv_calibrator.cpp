#include <iosfwd>
#include <stdio.h>
#include <osa/osa.h>
#include <dscv/dscv_calibrator.hpp>
#include "dscv_config.h"



/************************************** Calibrator ********************************************/


#pragma region Calibrator

const string Calibrator::kCameraMatrixKey = "cameraMatrix";
const string Calibrator::kDistCoeffsKey = "distCoeffs";
const string Calibrator::kValidPatternImagesCountKey = "ValidPatternImagesCount";
const string Calibrator::kReprojectionErrorKey = "RepeojectionError";


Calibrator::Calibrator(const cv::Size patternSize) : _patternSize(patternSize)
{
    _patternImagesCount = 0;
    calibrated = false;
}


void Calibrator::calcImagePoints(const bool shouldShowChessboards)
{
    Char windowName[64];
    const int kInterval = 1000;

	// Calculate the object points in the object co-ordinate system (origin at top left corner)
	vector<Point3f> patternPoints(_patternSize.area());  /* width * height */
	for (int i = 0; i < _patternSize.height; i++) {
		for (int j = 0; j < _patternSize.width; j++) {
			patternPoints[_patternSize.width * i + j] = Point3f(j, i, 0.f);
		}
	}

	for (size_t i = 0; i < _chessboardImages.size(); ++i) {
        snprintf(windowName, sizeof(windowName), "Chessboard Corners %u", i);

		Mat image = _chessboardImages[i];
		vector<Point2f> corners;
		bool pattern_found = findChessboardCorners(image, _patternSize, corners);
		if (pattern_found) {			
			Mat gray;
			cvtColor(image, gray, CV_BGR2GRAY);
			cornerSubPix(gray, corners, Size(5, 5), Size(-1, -1), TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
			_objectPoints.push_back(patternPoints);
			_imagePoints.push_back(corners);

#if DSCV_OPENCV_SUPPORTS_GUI
            if (shouldShowChessboards) {
                Mat drawnImage = image.clone();    /* do not modify the original image */
                drawChessboardCorners(drawnImage, _patternSize, corners, true);                
                imshow(windowName, drawnImage);
                waitKey(kInterval);
                destroyWindow(windowName);
            }
#endif
		}
		//if a valid pattern was not found, delete the entry from vector of images
		else {
#if DSCV_OPENCV_SUPPORTS_GUI
            if (shouldShowChessboards) {
                strncat(windowName, " (no chessboard found)", sizeof(windowName));
                imshow(windowName, image);
                waitKey(kInterval);
                destroyWindow(windowName);
            }
#endif

            OSA_info("Will remove invalid chessboard image %u; image data @ %p\n", i, image.data);
			_chessboardImages.erase(_chessboardImages.begin() + i); 
		}
	}
}


bool Calibrator::peek(Mat &image, Mat *pDrawnImage)
{
	vector<Point2f> corners;
    bool found;

	found = findChessboardCorners(image, _patternSize, corners);    
    if (found) {
        /* do not modify this image, since it may be used by the user for subsequent input for `calcImagePoints()` method, where the chessboard pattern won't be found again */
        Mat drawnImage = image.clone();
        drawChessboardCorners(drawnImage, _patternSize, corners, true);
        if (NULL != pDrawnImage) {
            *pDrawnImage = drawnImage;
        }
    }
    return found;
}


int Calibrator::setPatternImages(const vector<Mat> images)
{
	_chessboardImages = images;
	return OSA_STATUS_OK;
}


int Calibrator::calibrate()
{
	vector<Mat> rvecs, tvecs;
    size_t chessboardImagesCount;
	double rms;


	calcImagePoints(true);

    /*
      Even the pattern is found when calling `Calibrator::peek()`, it may not be found when calling `calcImagePoints`.
      So, use `_objectPoints.size()` to determine valid chessboard image count, not `_chessboardImages.size()`.
     */
    /* chessboardImagesCount = _chessboardImages.size(); */
    chessboardImagesCount = _objectPoints.size();

    if (chessboardImagesCount < kHardLeastImagesCount) {
        OSA_error("Insufficient valid chessboard images: expected at least %u, actual %u.\n", kHardLeastImagesCount, chessboardImagesCount);		
        return OSA_STATUS_EPERM;
    }

	if (chessboardImagesCount < kSoftLeastImagesCount) {
		OSA_warn("Recommended chessboard images count %u, actual %u.\n", kSoftLeastImagesCount, chessboardImagesCount);		
	}

	rms = calibrateCamera(_objectPoints, _imagePoints, _chessboardImages[0].size(), cameraMatrix, distCoeffs, rvecs, tvecs);
	OSA_info("Calibration done. RMS reprojection error = %f.\n", rms);

    _patternImagesCount = chessboardImagesCount;
    _reprojectionError = rms;
    calibrated = true;
	return OSA_STATUS_OK;
}


int Calibrator::dumpResult(const string path)
{
    if (!calibrated) {
        OSA_error("Not calibrated yet.\n");
        return OSA_STATUS_EPERM;
    }

	FileStorage fs(path, FileStorage::WRITE);
	if (!fs.isOpened()) {
        OSA_error("Failed to open file %s for dumping.\n", path.c_str());
        return OSA_STATUS_EGENERAL;
	}

	fs << kCameraMatrixKey << cameraMatrix;
	fs << kDistCoeffsKey << distCoeffs;
    fs << kValidPatternImagesCountKey << _patternImagesCount;
    fs << kReprojectionErrorKey << _reprojectionError;
	fs.release();

	return OSA_STATUS_OK;
}

#pragma endregion


/************************************** StereoCalibrator ********************************************/


#pragma region StereoCalibrator

const string StereoCalibrator::kLCameraMatrixKey = "lCameraMatrix";
const string StereoCalibrator::kRCameraMatrixKey = "rCameraMatrix";
const string StereoCalibrator::kLDistCoeffsKey = "lDistCoeffs";
const string StereoCalibrator::kRDistCoeffsKey = "rDistCoeffs";
const string StereoCalibrator::kRKey = "R";
const string StereoCalibrator::kTKey = "T";
const string StereoCalibrator::kEKey = "E";
const string StereoCalibrator::kFKey = "F";
const string StereoCalibrator::kValidPatternImagesCountKey = "ValidPatternImagesCount";
const string StereoCalibrator::kReprojectionErrorKey = "RepeojectionError";
const string StereoCalibrator::kIsLeftRightReversedKey = "isLeftRightReversed";

StereoCalibrator::StereoCalibrator(const cv::Size patternSize) : _patternSize(patternSize)
{
    calibrated = false;
    _patternImagesCount = 0;
    _reprojectionError = 0;
    _isLeftRightReversed = 0;
}


void StereoCalibrator::calcImagePoints(const bool shouldShowChessboards)
{
    Char lWindowName[64];
    Char rWindowName[64];
    const int kInterval = 1000;


	// Calculate the object points in the object co-ordinate system (origin at top left corner)
	vector<Point3f> patternPoints(_patternSize.area());  /* width * height */
	for (int i = 0; i < _patternSize.height; i++) {
		for (int j = 0; j < _patternSize.width; j++) {
			patternPoints[_patternSize.width * i + j] = Point3f(j, i, 0.f);
		}
	}

	for (size_t i = 0; i < _lChessboardImages.size(); i++) {
        snprintf(lWindowName, sizeof(lWindowName), "L Chessboard Corners %u", i);
        snprintf(rWindowName, sizeof(rWindowName), "R Chessboard Corners %u", i);

		Mat lImage = _lChessboardImages[i];
		Mat rImage = _rChessboardImages[i];
		vector<Point2f> lCorners, rCorners;
		bool lPatternFound = findChessboardCorners(lImage, _patternSize, lCorners);
		bool rPatternFound = findChessboardCorners(rImage, _patternSize, rCorners);
		if (lPatternFound && rPatternFound) {			
			Mat gray;
			cvtColor(lImage, gray, CV_BGR2GRAY);
			cornerSubPix(gray, lCorners, Size(5, 5), Size(-1, -1), TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
			cvtColor(rImage, gray, CV_BGR2GRAY);
			cornerSubPix(gray, rCorners, Size(5, 5), Size(-1, -1), TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
			_objectPoints.push_back(patternPoints);
			_lImagePoints.push_back(lCorners);
			_rImagePoints.push_back(rCorners);

#if DSCV_OPENCV_SUPPORTS_GUI
            if (shouldShowChessboards) {
                Mat lDrawnImage = lImage.clone(), rDrawnImage = rImage.clone();    /* do not modify the original image */                
                drawChessboardCorners(lDrawnImage, _patternSize, lCorners, true);
                drawChessboardCorners(rDrawnImage, _patternSize, rCorners, true);
                imshow(lWindowName, lDrawnImage);                
                imshow(rWindowName, rDrawnImage);
                waitKey(kInterval);
                destroyWindow(lWindowName);
                destroyWindow(rWindowName);
            }
#endif
		}
		else {
#if DSCV_OPENCV_SUPPORTS_GUI
            if (shouldShowChessboards) {
                strncat(lWindowName, " (no chessboard found)", sizeof(lWindowName));
                strncat(rWindowName, " (no chessboard found)", sizeof(rWindowName));
                imshow(lWindowName, lImage);
                imshow(rWindowName, rImage);
                waitKey(kInterval);
                destroyWindow(lWindowName);
                destroyWindow(rWindowName);
            }
#endif

            OSA_info("Will remove invalid chessboard images pair %u; L @ %p, R @ %p.\n", i, lImage.data, rImage.data);
			_lChessboardImages.erase(_lChessboardImages.begin() + i);
			_rChessboardImages.erase(_rChessboardImages.begin() + i);
		}
	}
}


int StereoCalibrator::setPatternImages(const vector<Mat> lImages, const vector<Mat> rImages)
{
	_lChessboardImages = lImages;
	_rChessboardImages = rImages;
	return OSA_STATUS_OK;
}


int StereoCalibrator::_calibrate()
{
	size_t chessboardImagesCount;
    double rms;


	if (_lCameraMatrix.empty() || _lDistCoeffs.empty() || _rCameraMatrix.empty() || _rDistCoeffs.empty()) {
		OSA_warn("It is strongly recommended to include left and right cameras calibration.\n");
	}

	calcImagePoints(true);

    /*
      Even the pattern is found when calling `Calibrator::peek()`, it may not be found when calling `calcImagePoints`.
      So, use `_objectPoints.size()` to determine valid chessboard image count, not `_chessboardImages.size()`.
    */
    /* chessboardImagesCount = _chessboardImages.size(); */
	chessboardImagesCount = _objectPoints.size();

    if (chessboardImagesCount < kHardLeastImagesCount) {
        OSA_error("Insufficient valid chessboard images: expected at least %u, actual %u.\n", kHardLeastImagesCount, chessboardImagesCount);		
        return OSA_STATUS_EPERM;
    }

    if (chessboardImagesCount < kSoftLeastImagesCount) {
        OSA_warn("Recommended chessboard images count %u, actual %u.\n", kSoftLeastImagesCount, chessboardImagesCount);		
    }

    rms = stereoCalibrate(_objectPoints, _lImagePoints, _rImagePoints, _lCameraMatrix, _lDistCoeffs, _rCameraMatrix, _rDistCoeffs, _lChessboardImages[0].size(), R, T, E, F);
        /*
        TermCriteria(TermCriteria::COUNT+TermCriteria::EPS, 30, 1e-6),
        CV_CALIB_FIX_INTRINSIC | CV_CALIB_USE_INTRINSIC_GUESS);
        */
	OSA_info("Calibrated stereo camera with an RMS error of %f.\n ", rms);

    if (_lImagePoints.size() > 0 && _rImagePoints.size() > 0) {
        cv::Point2f lPoint, rPoint;
        lPoint = _lImagePoints[0][0];
        rPoint = _rImagePoints[0][0];
        _isLeftRightReversed = lPoint.x < rPoint.x;    /* don't know why */
    }

    _patternImagesCount = chessboardImagesCount;
    _reprojectionError = rms;
    calibrated = true;
	return OSA_STATUS_OK;
}


int StereoCalibrator::calibrate(const string lCameraCalibration, const string rCameraCalibration)
{
	FileStorage lfs(lCameraCalibration, FileStorage::READ);
	if (lfs.isOpened()) {
		lfs[Calibrator::kCameraMatrixKey] >> _lCameraMatrix;
		lfs[Calibrator::kDistCoeffsKey] >> _lDistCoeffs;
	}
	lfs.release();	

	FileStorage rfs(rCameraCalibration, FileStorage::READ);
	if (rfs.isOpened()) {
		rfs[Calibrator::kCameraMatrixKey] >> _rCameraMatrix;
		rfs[Calibrator::kDistCoeffsKey] >> _rDistCoeffs;
	}
	rfs.release();	

	return _calibrate();
}


int StereoCalibrator::calibrate(const Mat lCameraMatrix, const Mat lDistCoeffs, const Mat rCameraMatrix, const Mat rDistCoeffs)
{
	_lCameraMatrix = lCameraMatrix;
	_lDistCoeffs = lDistCoeffs;
	_rCameraMatrix = rCameraMatrix;
	_rDistCoeffs = rDistCoeffs;

	return _calibrate();
}


int StereoCalibrator::calibrate()
{
	int ret;

	Calibrator lCalibrator(_patternSize);
	lCalibrator.setPatternImages(_lChessboardImages);
	ret = lCalibrator.calibrate();
	if (OSA_isFailed(ret)) {
		OSA_error("Calibrating left camera failed with %d.\n", ret);
		return ret;
	}

	Calibrator rCalibrator(_patternSize);
	rCalibrator.setPatternImages(_rChessboardImages);
	ret = rCalibrator.calibrate();
	if (OSA_isFailed(ret)) {
		OSA_error("Calibrating right camera failed with %d.\n", ret);
		return ret;
	}

	return calibrate(lCalibrator.cameraMatrix, lCalibrator.distCoeffs, rCalibrator.cameraMatrix, rCalibrator.distCoeffs);
}


int StereoCalibrator::dumpResult(const string stereoPath)
{
    if (!calibrated) {
        OSA_error("Not calibrated yet.\n");
        return OSA_STATUS_EPERM;
    }

	FileStorage fs(stereoPath, FileStorage::WRITE);
	if (!fs.isOpened()) {
		OSA_error("Failed to open file %s for dumping.\n", stereoPath.c_str());
		return OSA_STATUS_EGENERAL;
	}
	
	fs << kLCameraMatrixKey << _lCameraMatrix;
	fs << kRCameraMatrixKey << _rCameraMatrix;
	fs << kLDistCoeffsKey << _lDistCoeffs;
	fs << kRDistCoeffsKey << _rDistCoeffs;
	fs << kRKey << R;
	fs << kTKey << T;
	fs << kEKey << E;
	fs << kFKey << F;
    fs << kValidPatternImagesCountKey << _patternImagesCount;
    fs << kReprojectionErrorKey << _reprojectionError;
    fs << kIsLeftRightReversedKey << _isLeftRightReversed;
    fs.release();

	return OSA_STATUS_OK;
}

#pragma endregion