#pragma once
#ifndef __DSCV_RECTUTILS_HPP__
#define __DSCV_RECTUTILS_HPP__

#include <opencv2/opencv.hpp>
#include "dscv_definitions.h"

using namespace std;
using namespace cv;

class DSCV_API RectUtils
{
private:
public:
	unsigned int CalcDiagonal2(const cv::Rect &rect) {
		return rect.size().width * rect.size().width + rect.size().height * rect.size().height;
	};

	static int CompareRect(const cv::Rect &rect1, const cv::Rect &rect2);

    static bool isZero(const cv::Rect &rect) { return 0 == rect.width || 0 == rect.height; }

	static cv::Rect CoveredRect(const vector<cv::Rect> rects);

	static cv::Point bottomRight(const cv::Rect &rect);
};

#endif // !__DSCV_RECTUTILS_HPP__

