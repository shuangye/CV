#pragma once
#ifndef __CVD_DEBUG_HPP__
#define __CVD_DEBUG_HPP__

/*
* Created by Liu Papillon, on Dec 21, 2017.
*/


#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

#define CVD_DEBUG_DUMP_AUTO_LABEL (__FILE__ __func__)


void CVD_debugDumpImages(const string label, const vector<Mat> images);


#ifdef __cplusplus
extern "C" {
#endif     


#ifdef __cplusplus
}
#endif

#endif  /* __CVD_DEBUG_HPP__ */
