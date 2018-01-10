#pragma once
#ifndef __CVD_PRI_HPP__
#define __CVD_PRI_HPP__

/*
* Created by Liu Papillon, on Nov 23, 2017.
*/

#include <cvd/cvd.h>

#define CVD_INPUT_IMAGES_COUNT   CVD_FACE_DETECTION_COUNT


extern CVD_CvOut                      *CVD_gpCvOut;    /* defined in cvd_comm.cpp */

extern vector<Mat>                     CVD_gSourceImagesForLivingDetermination;
extern vector<cv::Rect>                CVD_gFacesForLivingDetermination;
extern OSA_Size                        CVD_gFaceDetectionInputImageSize;

#ifdef __cplusplus
extern "C" {
#endif     


#ifdef __cplusplus
}
#endif

#endif  /* __CVD_PRI_HPP__ */
