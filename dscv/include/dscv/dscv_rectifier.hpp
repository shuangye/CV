#pragma once
#ifndef __DSCV_RECTIFIER_HPP__
#define __DSCV_RECTIFIER_HPP__

#include <opencv2/opencv.hpp>
#include "dscv_definitions.h"

using namespace std;
using namespace cv;


class DSCV_API Rectifier {
public:
    Rectifier(const string calibrationPath, const cv::Size imageSize);
    int rectify(const Mat image, Mat &rectifiedImage);

private:
    bool _isValid;
    const cv::Size _imageSize;  /* original image size */	
    Mat map1, map2;
};


class DSCV_API StereoRectifier {
public:
    int getQ(Mat &Q);
    StereoRectifier();
    int load(const string stereoCalibrationPath, const cv::Size imageSize);
    int rectify(const Mat lImage, const Mat rImage, Mat &lRectifiedImage, Mat &rRectifiedImage, Mat *pCombined);
    void setValidity(bool valid) { _validity = valid; }
    bool getValidity() const { return _validity; }
    
private:    
    bool _validity;
    Mat _Q;
    cv::Size _imageSize;
    Mat _mapL1, _mapL2, _mapR1, _mapR2; /* pixel maps for rectification */
    int _isLeftRightReversed;    /* boolean semantic (use int for storing into XML): to identify which camera is on the left side; defaults = 0 (not reversed) */
};


#endif // !__DSCV_RECTIFIER_HPP__
