#include <osa/osa.h>
#include <face/face.hpp>
#include "../src/face_config.h"
#include "../../mio/include/mio/mio.hpp"
#include "face_tests.hpp"

using namespace mio;


/**************************************************/


static int detectFace(Mat &frame, int key)
{
    static bool inited = false;
    static FACE_Handle handle = NULL;
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
        cv::Rect face;
        return FACE_detectFace(handle, frame, face);
    }

    return OSA_STATUS_OK;
}


static int FACE_detection_Test()
{
    vector<int> cameras;
    cameras.push_back(0);
    Camera camera(cameras);	
    camera.preview(25, detectFace);
    return OSA_STATUS_OK;
}


int FACE_detection_Tests()
{
    return FACE_detection_Test();
}
