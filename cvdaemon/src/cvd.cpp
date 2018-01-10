/*
* Created by Liu Papillon, on Nov 14, 2017.
*/

#include <vector>
#include <osa/osa.h>
#include <comm/comm.h>
#include <cvd/cvd.h>
#include <cvd/cvd.hpp>
#include "cvd_comm.hpp"
#include "cvd_face.hpp"
#include "cvd_calibrator.hpp"
#include "cvd_image.hpp"


int CVD_init()
{
    int ret;


    OSA_info("Module %s, built on %s, %s.\n", OSA_MODULE_NAME, __DATE__, __TIME__);

    ret = CVD_imageInit();
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to init image related functions: %d.\n", ret);
        goto _failure;
    }

    ret = CVD_commInit();
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to init COMM related functions: %d.\n", ret);
        goto _failure;
    }
    
    ret = CVD_faceInit();
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to init face related functions: %d.\n", ret);
        goto _failure;
    }

    ret = CVD_calibratorInit();
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to init calibrator related functions: %d.\n", ret);
        goto _failure;
    }
         
    OSA_info("Module %s is inited.\n", OSA_MODULE_NAME);
    return ret;

_failure:
    CVD_calibratorDeinit();
    CVD_faceDeinit();
    CVD_commDeinit();
    CVD_imageDeinit();
    return ret;
}


int CVD_wait()
{
    int ret = 0;

    ret |= CVD_calibratorWait();
    ret |= CVD_faceWait();

    return ret;
}

