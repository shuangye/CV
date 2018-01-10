#include <time.h>
#include <opencv2/opencv.hpp>
#include <osa/osa.h>
#include <mio/camera.hpp>
#include "../mio_config.h"

using namespace std;
using namespace cv;


namespace mio {

Char * OSA_datetimeGetCurrent(const Char *pFormat, Char *pBuffer, size_t length)
{
    time_t t = time(NULL);
    struct tm *pTimeInfo = localtime(&t);
    strftime(pBuffer, length, pFormat, pTimeInfo);
    return pBuffer;
}



Camera::Camera(const vector<int> cameras) : _cameras(cameras)
{
	int cameraId;

	for (size_t i = 0; i < _cameras.size(); ++i) {
		cameraId = _cameras[i];
		_captures[cameraId] = VideoCapture(cameraId);
	}
}

Camera::~Camera()
{
	for (map<int, VideoCapture>::iterator i = _captures.begin(); i != _captures.end(); ++i) {		
		i->second.release();
	}
}


bool Camera::checkCamera(const int cameraId)
{
	VideoCapture capture;

	if (find(_cameras.begin(), _cameras.end(), cameraId) == _cameras.end()) {
		cerr << "Camera with ID of " << cameraId << " is not initialized." << endl;
		return false;
	}

	capture = _captures[cameraId];
	if (!capture.isOpened()) {
		cerr << "Camera with ID of " << cameraId << " is not opened." << endl;
		return false;
	}

	return true;
}


int Camera::set(const vector<int> cameras)
{
	int cameraId;
	VideoCapture capture;

	for (size_t i = 0; i < cameras.size(); ++i) {
		cameraId = cameras[i];
		if (!checkCamera(cameraId)) {
			return -1;
		}

		capture = _captures[cameraId];
#if 1
		// capture.set(CV_CAP_PROP_SETTINGS, 1);
#else
		double v;
		v = capture.get(CV_CAP_PROP_AUTO_EXPOSURE);
		v = capture.get(CV_CAP_PROP_EXPOSURE);
		v = capture.get(CV_CAP_PROP_BRIGHTNESS);
		
		bool succeeded;
		succeeded = capture.set(CV_CAP_PROP_FRAME_WIDTH, 1920);
		succeeded = capture.set(CV_CAP_PROP_FRAME_HEIGHT, 1080);
		succeeded = capture.set(CV_CAP_PROP_FPS, 25);
		succeeded = capture.set(CV_CAP_PROP_FORMAT, CV_8UC3);
		succeeded = capture.set(CV_CAP_PROP_AUTO_EXPOSURE, 1);
		succeeded = capture.set(CV_CAP_PROP_EXPOSURE, -10);
		succeeded = capture.set(CV_CAP_PROP_BRIGHTNESS, 50);		
#endif
	}

	return 0;
}


int Camera::getFrames(const vector<int> cameras, vector<Mat> &frames)
{
	int ret = 0;
	int cameraId;
	VideoCapture capture;
    Char datetime[64];
    cv::Point textOrigin(0, 20);


	/* grab() + retrieve(), to sync between multiple cameras */

	for (size_t i = 0; i < cameras.size(); ++i) {
		cameraId = cameras[i];
		if (!checkCamera(cameraId)) {
			return -1;
		}

		capture = _captures[cameraId];
		capture.grab();
	}

	for (size_t i = 0; i < cameras.size(); ++i) {
		cameraId = cameras[i];		
		capture = _captures[cameraId];
		if (!capture.retrieve(frames[i])) {
			return -1;
		}

        if (overlayDatetime) {
            OSA_datetimeGetCurrent(OSA_DATETIME_FORMAT_ISO8601, datetime, sizeof(datetime));
            cv::putText(frames[i], string(datetime), textOrigin, FONT_HERSHEY_PLAIN, 2, cv::Scalar(0, 0, 255), 1);
        }
	}

	return ret;
}


int Camera::preview(int fps, PerFrameHandler perFrameHandler, AllFramesHandler allFramesHandler)
{
	int ret;
	const size_t kCameraCount = _cameras.size();
	vector<Mat> frames = vector<Mat>(kCameraCount);
    OSA_lowerLimit(fps, 1);
    int key;
	const int kInterval = 1000 / fps;  /* in ms */
    const int kIdleInterval = 1000;
	const int kEscKey = 27;
    VideoWriter *pWriter = NULL;  
    bool shouldRecord = false;
    bool shouldExit = false;

        

#if CVIO_OPENCV_SUPPORTS_GUI
	stringstream name;
	vector<string> windows(kCameraCount);
	for (size_t i = 0; i < kCameraCount; ++i) {
		name.clear();
		name.str("");
		name << "Camera " << i;
		windows[i] = name.str();
		namedWindow(windows[i], WINDOW_NORMAL);
	}
#endif

	OSA_info("There is/are %u camera(s).\n", kCameraCount);
		
	for (; ;) {
#if CVIO_OPENCV_SUPPORTS_GUI
        key = waitKey(kInterval);
        switch (key) {
        case 'r':
            shouldRecord = true;
            break;
        case 'q':
            shouldRecord = false;
            break;
        case kEscKey:
            shouldExit = true;
            break;
        default:
            break;
        }
		
        if (shouldExit) {
            break;
        }
#else
		for (size_t i = 0; i < kCameraCount; ++i) {
			printf("Camera %d frame type %d, channel count %d, resolution %dx%d\n", i, frames[i].type(), frames[i].channels(), frames[i].cols, frames[i].rows);
		}		
#endif	

        ret = getFrames(_cameras, frames);
        if (0 != ret) {
            waitKey(kIdleInterval);
            continue;
        }

#if 0
        if (frames[0].cols > 320) {
            Mat image;
            double factor = 320.0 / frames[0].cols;
            cv::resize(frames[0], image, cv::Size(0, 0), factor, factor);
            OSA_info("%dx%d vs. %dx%d\n", image.cols, image.rows, frames[0].cols, frames[0].rows);
        }
#endif

        if (NULL != allFramesHandler) {
            allFramesHandler(frames, key);
        }
        
        for (size_t i = 0; i < kCameraCount; ++i) {
            if (NULL != perFrameHandler) {
                perFrameHandler(frames[i], key);
            }

#if CVIO_OPENCV_SUPPORTS_GUI
            imshow(windows[i], frames[i]);
#endif
        }

        if (shouldRecord) {
            if (NULL == pWriter) {
                pWriter = new VideoWriter("recording.avi", CV_FOURCC('M', 'J', 'P', 'G'), 10, cv::Size(frames[0].cols, frames[0].rows), true);
            }
            pWriter->write(frames[0]);
        }
	}

#if SUPPORT_IMAGE_PREVIEW
	for (size_t i = 0; i < kCameraCount; ++i) {
		destroyWindow(windows[i]);	
	}
#endif

    if (NULL != pWriter) {
        pWriter->release();
    }

	return OSA_STATUS_OK;
}



int Camera::_saveFrame(Mat &frame, int key)
{
	if ('c' == key) {
		imwrite(string(CVIO_CAPTURED_FRAME_PATH), frame);
	}
	return OSA_STATUS_OK;
}

}