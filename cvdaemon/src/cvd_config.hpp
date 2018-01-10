#pragma once
#ifndef __CVD_CONFIG_HPP__
#define __CVD_CONFIG_HPP__

/*
* Created by Liu Papillon, on Nov 14, 2017.
*/



#ifdef OSA_OS_WINDOWS
#define CVD_OPENCV_SUPPORTS_GUI                                             1
#define CVD_STEREO_CALIBRATION_PATH                                        "stereo_calibration.xml"
#define CVD_STEREO_CALIBRATION_PREVIEW_IMAGE_PATH                          "stereo_calibration.jpg"
#define CVD_FACE_DETECTION_MODEL_PATH                                       "D:/CV/OpenCV2.4.13/sources/data/lbpcascades/lbpcascade_frontalface.xml"
#endif                                                                     
                                                                           
#ifdef OSA_OS_ANDROID                                                      
#define CVD_OPENCV_SUPPORTS_GUI                                             0
#define CVD_STEREO_CALIBRATION_PATH                                        "/data/rk_backup/stereo_calibration.xml"
#define CVD_STEREO_CALIBRATION_PREVIEW_IMAGE_PATH                          "/data/rk_backup/stereo_calibration.jpg"
#define CVD_FACE_DETECTION_MODEL_PATH                                      "/system/usr/share/dusun/lbpcascade_frontalface.xml"
#endif // OSA_OS_ANDROID                                                   
                                                                           
                                                                           
#define CVD_TEST_IMAGE_PATH                                                "face.jpg"
#define CVD_FACE_DETECTION_W                                               320


#ifdef __cplusplus
extern "C" {
#endif     


#ifdef __cplusplus
}
#endif

#endif  /* __CVD_CONFIG_HPP__ */
