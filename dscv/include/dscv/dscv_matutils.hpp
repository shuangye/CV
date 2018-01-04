#pragma once
#ifndef __DSCV_MATUTILS_HPP__
#define __DSCV_MATUTILS_HPP__

#include <opencv2/opencv.hpp>
#include "dscv_definitions.h"

using namespace cv;
using namespace std;


class DSCV_API MatUtils 
{
public:
	cv::Point findLightestRegion(const Mat &image, const cv::Size windowSize);

	cv::Point findDarkestRegion(const Mat &image, const cv::Size windowSize);
		
	unsigned long long sum(const Mat &image);
	
	unsigned int average(const Mat &image);

	unsigned int sumOfAbsDiff(const Mat &image);

	int levelDiff(const Mat &image0, const Mat &image1, Mat &diff);

	void neutralizeIfNeeded(Mat &image);

	int neutralizeGray(Mat &image);

	void rotate(Mat &src, Mat &dst, int degree);

	void calcLbp(const Mat srcImage, Mat &lbp);


	/* static functions */

	static Mat toGray(Mat image);

	static double calcSimilarity(const Mat A, const Mat B);


private:
};

#endif // !__DSCV_MATUTILS_HPP__


