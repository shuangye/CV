#pragma once
#ifndef __DS_FACE_STEREO_HPP__
#define __DS_FACE_STEREO_HPP__


#include <opencv2/opencv.hpp>
// #include "Rectifier.hpp"

using namespace std;
using namespace cv;

class Stereo
{
public:
	Stereo();
	~Stereo();
	/*
	* @brief: calculate the disparity map, given two images. The two images must be of the same size.
	*/
	int calcDisparity(Mat lImage, Mat rImage, Mat& disparity);

	// int calcDepth(const string stereoCalibrationPath, cv::Rect region);

private:
	// StereoRectifier *_pStereoRectifier;
	Mat _disparity;
	//Mat _Q;
};


#endif  /* __DS_FACE_STEREO_HPP__ */
