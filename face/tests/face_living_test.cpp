#include <osa/osa.h>
#include <face/face.hpp>
#include <dscv/dscv.hpp>
#include "../src/face_config.h"
#include "../../cvio/include/cvio/cvio.hpp"
#include "face_tests.hpp"


static StereoRectifier                 *gpStereoRectifier = NULL;

/**************************************************/

static int determineLivingFace(vector<Mat> &frames, int key)
{
    static bool inited = false;
    static FACE_Handle handle = NULL;
    vector<Mat> rectifiedFrames(2);
    Mat combinedImage;
    Char buffer[32];
    int possibility = 0;
    int ret;


    (void)key;

    if (!inited) {
        FACE_Options options;
        OSA_clear(&options);
        OSA_strncpy(options.faceDetectionModelPath, FACE_DETECTION_MODEL_PATH);
        ret = FACE_init(options, handle);
        if (OSA_isSucceeded(ret)) {
            inited = true;
        }
    }
    else {
        assert(frames.size() == 2);

        if (NULL == gpStereoRectifier) {
            gpStereoRectifier = new StereoRectifier();            
        }

        assert(NULL != gpStereoRectifier);

        if (!gpStereoRectifier->getValidity()) {
            gpStereoRectifier->load(string(FACE_STEREO_CALIBRATION_PATH), cv::Size(frames[0].cols, frames[0].rows));
        }

        if (gpStereoRectifier->getValidity()) {
            ret = gpStereoRectifier->rectify(frames[0], frames[1], rectifiedFrames[0], rectifiedFrames[1], &combinedImage);
            if (OSA_isSucceeded(ret)) {
                frames = rectifiedFrames;
                // imshow("Combined", combinedImage);
            }
        }
        
        // cv::resize(frames[0], frames[0], cv::Size(320, 240));
        // cv::resize(frames[1], frames[1], cv::Size(320, 240));
        ret = FACE_detectAndDetermineLiving(handle, frames[0], frames[1], possibility);
        OSA_debug("Determining living face result %d, possibility %d.\n", ret, possibility);
        if (OSA_isSucceeded(ret)) {
            snprintf(buffer, sizeof(buffer), "Possibility = %d%", possibility);
            cv::putText(frames[0], string(buffer), cv::Point(0, frames[0].size().height - 20), FONT_HERSHEY_PLAIN, 2, cv::Scalar(0, 0, 255), 1);
        }
    }

    return OSA_STATUS_OK;
}


static int FACE_determineLiving_Test()
{
    vector<int> cameras;
    cameras.push_back(0);
    cameras.push_back(1);
    Camera camera(cameras);	
    camera.preview(25, NULL, determineLivingFace);
    return OSA_STATUS_OK;
}


/**************************************************/


int FACE_living_Tests()
{
    return FACE_determineLiving_Test();
}
