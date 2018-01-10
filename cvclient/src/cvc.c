/*
* Created by Liu Papillon, on Nov 14, 2017.
*/

#include <osa/osa.h>
#include <cvd/cvd.h>
#include <cvc/cvc.h>
#include "cvc_config.h"
#include "cvc_comm.h"


int CVC_init()
{
    int ret;

    OSA_info("Module %s, built on %s, %s.\n", OSA_MODULE_NAME, __DATE__, __TIME__);

    ret = CVC_commInit();
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to init comm related functions: %d.\n", ret);
        goto _failure;
    }

    OSA_info("Inited CVC module.\n");
    return OSA_STATUS_OK;

_failure:
    CVC_deinit();
    return ret;
}


int CVC_deinit()
{
    OSA_info("Deinited CVC module.\n");
    return OSA_STATUS_OK;
}


int CVC_calibratorInit(const int patternW, const int patternH)
{
    OSA_Size size = { .w = patternW, .h = patternH };
    return CVC_commRequestCalibratorOperation(CVD_CMD_INIT_CALIBRATION, &size);
}


int CVC_calibratorCollectImage()
{
    return CVC_commRequestCalibratorOperation(CVD_CMD_COLLECT_CALIBRATION_IMAGE, NULL);
}


int CVC_calibratorStereoCalibrate()
{
    return CVC_commRequestCalibratorOperation(CVD_CMD_STEREO_CALIBRATE, NULL);
}


int CVC_detectFace(const int id, OSA_Rect *pFace, OSA_Size *pRelativeSize)
{
    if (NULL == pFace || NULL == pRelativeSize) {
        OSA_error("Invalid parameter.\n");
        return OSA_STATUS_EINVAL;
    }

    return CVC_commGetDetectedFace(id, pFace, pRelativeSize);
}


int CVC_setLivingFaceThreshold(const int threshold)
{
    return CVC_commSetLivingFaceThreshold(threshold);
}


int CVC_determineLivingFace(int *pPossibility, void *pFace, size_t *pLen)
{
    if (NULL == pPossibility || NULL == pFace || NULL == pLen) {
        OSA_error("Invalid parameter.\n");
        return OSA_STATUS_EINVAL;
    }
    
    return CVC_commGetLivingFace(pPossibility, pFace, pLen);
}
