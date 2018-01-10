#pragma once
#ifndef __CVD_COMM_HPP__
#define __CVD_COMM_HPP__

/*
* Created by Liu Papillon, on Nov 14, 2017.
*/

#include <vector>
#include <opencv2/opencv.hpp>
#include <osa/osa.h>

using namespace std;
using namespace cv;


int CVD_commInit();

int CVD_commDeinit();

int CVD_commPutFaceDetectionResult(const vector<cv::Mat> sourceImages, const vector<cv::Rect> faces, const unsigned int validFaces);

int CVD_commPutLivingFaceDeterminationResult(const vector<cv::Mat> inputImages, const vector<cv::Rect> faces, const int possibility);

void CVD_printMemory(const void *pAddr, const size_t len);


#ifdef __cplusplus
extern "C" {
#endif     


#ifdef __cplusplus
}
#endif

#endif  /* __CVD_COMM_HPP__ */
