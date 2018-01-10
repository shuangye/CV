#pragma once
#ifndef __CVD_CALIBRATOR_HPP__
#define __CVD_CALIBRATOR_HPP__

/*
* Created by Liu Papillon, on Nov 16, 2017.
*/


#include <opencv2/opencv.hpp>
#include <cvd/cvd.h>

using namespace std;
using namespace cv;


int CVD_calibratorInit();

int CVD_calibratorDeinit();

int CVD_calibratorWait();


#ifdef __cplusplus
extern "C" {
#endif     


#ifdef __cplusplus
}
#endif

#endif  /* __CVD_CALIBRATOR_HPP__ */
