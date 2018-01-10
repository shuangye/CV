#pragma once
#ifndef __CVD_IMAGE_HPP__
#define __CVD_IMAGE_HPP__

/*
* Created by Liu Papillon, on Nov 14, 2017.
*/


#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;


int CVD_imageInit();

int CVD_imageDeinit();

int CVD_imageGet(const vector<int> cameras, vector<Mat> &frames);


#ifdef __cplusplus
extern "C" {
#endif     


#ifdef __cplusplus
}
#endif

#endif  /* __CVD_IMAGE_HPP__ */
