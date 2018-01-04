#pragma once
#ifndef __DSCV_CALIBRATOR_HPP__
#define __DSCV_CALIBRATOR_HPP__

#include <opencv2/opencv.hpp>
#include "dscv_definitions.h"

using namespace std;
using namespace cv;


class DSCV_API Calibrator {
public:
    static const size_t kHardLeastImagesCount = DSCV_HARD_LEAST_CALIBRATION_IMAGES_COUNT;
    static const size_t kSoftLeastImagesCount = DSCV_SOFT_LEAST_CALIBRATION_IMAGES_COUNT;
	static const string kCameraMatrixKey;
	static const string kDistCoeffsKey;    
    static const string kValidPatternImagesCountKey;
    static const string kReprojectionErrorKey;
	cv::Mat cameraMatrix;
	cv::Mat distCoeffs;
    bool calibrated;
	
	Calibrator(const cv::Size patternSize);
	bool peek(Mat &image, Mat *pDrawnImage = NULL);
	int setPatternImages(const vector<Mat> images);
	int calibrate();
	int dumpResult(const string path);
	

private:	
	const cv::Size _patternSize;
    int _patternImagesCount;
    double _reprojectionError;
	vector<Mat> _chessboardImages;
	std::vector< std::vector<cv::Point3f> > _objectPoints;  /* 3D world coordinates, in length, e.g., millimeters */
	std::vector< std::vector<cv::Point2f> > _imagePoints;   /* 2D image coordinates, in pixels */	

	void calcImagePoints(bool shouldShowChessboards);
};


class DSCV_API StereoCalibrator {
public:	
    static const size_t kHardLeastImagesCount = DSCV_HARD_LEAST_CALIBRATION_IMAGES_COUNT;
    static const size_t kSoftLeastImagesCount = DSCV_SOFT_LEAST_CALIBRATION_IMAGES_COUNT;
	static const string kLCameraMatrixKey;
	static const string kLDistCoeffsKey;
	static const string kRCameraMatrixKey;
	static const string kRDistCoeffsKey;
	static const string kRKey;
	static const string kTKey;
	static const string kEKey;
	static const string kFKey;
    static const string kValidPatternImagesCountKey;
    static const string kReprojectionErrorKey;
    static const string kIsLeftRightReversedKey;
	Mat R, T, E, F; /* stereo calibration information */
    bool calibrated;

	StereoCalibrator(const cv::Size patternSize);
	int setPatternImages(const vector<Mat> lImages, const vector<Mat> rImages);
	int calibrate(const string lCameraCalibration, const string rCameraCalibration);
	int calibrate(const Mat lCameraMatrix, const Mat lDistCoeffs, const Mat rCameraMatrix, const Mat rDistCoeffs);
	int calibrate();
	int dumpResult(const string stereoPath);

private:
	const cv::Size _patternSize;
    int _patternImagesCount;
    double _reprojectionError;
	vector<Mat> _lChessboardImages, _rChessboardImages;
	std::vector< std::vector<cv::Point3f> > _objectPoints;  /* 3D world coordinates, in length, e.g., millimeters */
	std::vector< std::vector<cv::Point2f> > _lImagePoints, _rImagePoints;   /* 2D image coordinates, in pixels */
	cv::Mat _lCameraMatrix, _rCameraMatrix;
	cv::Mat _lDistCoeffs, _rDistCoeffs;
    int _isLeftRightReversed;    /* boolean semantic (use int for storing into XML): to identify which camera is on the left side; defaults = 0 (not reversed) */

	void calcImagePoints(const bool shouldShowChessboards);
	int _calibrate();
};


#endif // !__DSCV_CALIBRATOR_HPP__
