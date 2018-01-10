#pragma once
#ifndef __DS_FACE_HPP__
#define __DS_FACE_HPP__

/*
* Created by Liu Papillon, on Nov 11, 2017, the single day.
*
* This header exports C++ APIs.
*
* These APIs obey a rule: do not modify user provided data. Thus, the input images are passed by values, not by references.
*/


#include <opencv2/opencv.hpp>
#include <osa/osa.h>
#include "face_definitions.h"

using namespace std;
using namespace cv;


FACE_API int FACE_init(FACE_Options &options, FACE_Handle &handle);

FACE_API int FACE_deinitialize(FACE_Handle &handle);

FACE_API int FACE_detectFace(FACE_Handle &handle, Mat image, cv::Rect &face);

FACE_API int FACE_determineLiving(FACE_Handle &handle, Mat frame1, Mat frame2, cv::Rect face1, cv::Rect face2, Int32 &possibility);

FACE_API int FACE_detectAndDetermineLiving(FACE_Handle &handle, Mat frame1, Mat frame2, Int32 &possibility);


#endif //! __DS_FACE_HPP__
