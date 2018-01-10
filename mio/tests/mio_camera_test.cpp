#include <osa/osa.h>
#include <mio/mio.hpp>
#include "../src/mio_config.h"
#include "../../face/include/face/face.hpp"
#include "mio_tests.hpp"

using namespace mio;


static int MIO_cameraPreview_Test()
{
    vector<int> cameras;
    cameras.push_back(0);
    Camera camera(cameras);	
    camera.overlayDatetime = true;
    camera.preview(25, NULL);
    return OSA_STATUS_OK;
}


int MIO_camera_Tests()
{
    return MIO_cameraPreview_Test();
}
