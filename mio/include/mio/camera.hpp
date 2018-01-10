#pragma once
#ifndef __MIO_CAMERA_HPP__
#define __MIO_CAMERA_HPP__

#include <opencv2/opencv.hpp>
#include "mio_definitions.h"

using namespace cv;
using namespace std;


namespace mio
{

    class MIO_API Camera
    {
    public:
        typedef int(*PerFrameHandler)(Mat &frame, int key);
        typedef int(*AllFramesHandler)(vector<Mat> &frames, int key);

        int fps = 25;
        bool overlayDatetime = false;
        PerFrameHandler saveHandler = _saveFrame;

        Camera(const vector<int> cameras);
        ~Camera();
        bool checkCamera(const int cameraId);
        int set(const vector<int> cameras);
        int getFrames(const vector<int> cameras, vector<Mat> &frames);
        int preview(int fps = 25, PerFrameHandler perFrameHandler = NULL, AllFramesHandler allFramesHandler = NULL);

    private:
        vector<int> _cameras;
        map<int, VideoCapture> _captures;  /* <cameraId, VideoCapture> */

        static int _saveFrame(Mat &frame, int key);
    };

}

#endif  /* __MIO_CAMERA_HPP__ */
