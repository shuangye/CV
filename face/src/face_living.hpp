#pragma once
#ifndef __DS_FACE_LIVING_HPP__
#define __DS_FACE_LIVING_HPP__

#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

class Living
{
public:
	Living();
	~Living();

	int determineLivingFace(Mat &lImage, Mat &rImage, cv::Rect lFaceRegion, cv::Rect rFaceRegion, int &possibility);		

private:
    static const int _kMinFaceRatio = 32;
	bool _showDisparity;
	cv::Rect _detectedFaceInDisparity;
	Mat _disparity;
	Mat _lFaceGray, _dFaceGray;
    
	int determineLivingFaceTopHalf(Mat &lImage, Mat &rImage, cv::Rect lFaceRegion, cv::Rect rFaceRegion, int &possibility);
	int determineLivingFaceBottomHalf(Mat &lImage, Mat &rImage, cv::Rect lFaceRegion, cv::Rect rFaceRegion, int &possibility);
    void livingFacePenalty(Mat &lImage, Mat &rImage, cv::Rect lFaceRegion, cv::Rect rFaceRegion, int &possibility);
	int determineLivingFace1(Mat &lImage, Mat &rImage, cv::Rect lFaceRegion, cv::Rect rFaceRegion, int &possibility);
	int determineLivingFace2(Mat &lImage, Mat &rImage, cv::Rect lFaceRegion, cv::Rect rFaceRegion, int &possibility);
	// int determineLivingFace3(Mat &lImage, Mat &rImage, cv::Rect lFaceRegion, cv::Rect rFaceRegion, int &possibility);
	int determineLivingFace4(Mat &lImage, Mat &rImage, cv::Rect lFaceRegion, cv::Rect rFaceRegion, int &possibility);
	int determineLivingFace5(Mat &lImage, Mat &rImage, cv::Rect lFaceRegion, cv::Rect rFaceRegion, int &possibility);
};


#endif  /* __DS_FACE_LIVING_HPP__ */
