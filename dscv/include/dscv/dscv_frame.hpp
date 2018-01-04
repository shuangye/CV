#pragma once
#ifndef __DSCV_FRAME_HPP__
#define __DSCV_FRAME_HPP__

/*
* Created by Liu Papillon, on Nov 11, 2017, the single day.
*/


#include <opencv2/opencv.hpp>
#include "dscv_definitions.h"
#include "dscv_frame.h"

using namespace std;
using namespace cv;

DSCV_API int DSCV_matFromFrame(const DSCV_Frame *pFrame, Mat &dst);

/* caller shall fill frame format and width/height */
DSCV_API int DSCV_matFromFile(const string path, DSCV_Frame *pFrame, Mat &dst);

DSCV_API int DSCV_encodeMat(const Mat &mat, const int codec, vector<Uchar> &encodedImage);


#endif // !__DSCV_FRAME_HPP__
