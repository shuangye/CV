/*
* Created by Liu Papillon, on Jan 4, 2018.
*/

#include <stdlib.h>
#include <string.h>
#include <osa/osa.h>
#include <dscv/dscv.h>
#include <mediac/mediac.h>


/*
 exe clientId operation [parameters]
 - exe clientId open
 - exe clientId format 640x480 
 - exe clientId fps 30
 - exe clientId start 
 - exe clientId stop
 - exe clientId close
 */


static int gClientId = 0;
static MEDIAC_Handle gHandle = NULL;



static int operate(const int argc, char *argv[])
{
    int ret;
    const char *pOperation = argv[2];
    char *pParameter = NULL;

    if (0 == strcmp(pOperation, "open")) {
        ret = MEDIAC_open(gHandle);
    }
    else if (0 == strcmp(pOperation, "format")) {
        if (argc < 4) {
            OSA_error("Missing parameter for operation %s\n", pOperation);
            ret = OSA_STATUS_EINVAL;
        }
        else {
            OSA_Size frameSize;
            pParameter = argv[3];
            sscanf(pParameter, " %dx%d ", &frameSize.w, &frameSize.h);
            ret = MEDIAC_setFormat(gHandle, DSCV_FRAME_TYPE_YUYV, frameSize);
        }
    }
    else if (0 == strcmp(pOperation, "fps")) {
        if (argc < 4) {
            OSA_error("Missing parameter for operation %s\n", pOperation);
            ret = OSA_STATUS_EINVAL;
        }
        else {
            pParameter = argv[3];
            ret = MEDIAC_setFrameRate(gHandle, strtol(pParameter, NULL, 0));
        }
    }
    else if (0 == strcmp(pOperation, "start")) {
        ret = MEDIAC_startStreaming(gHandle);
    }
    else if (0 == strcmp(pOperation, "stop")) {
        ret = MEDIAC_stopStreaming(gHandle);
    }
    else if (0 == strcmp(pOperation, "close")) {
        ret = MEDIAC_close(gHandle);
    }
    else {
        OSA_error("Unsupported operation %s\n", pOperation);
        ret = OSA_STATUS_EINVAL;
    }

    if (OSA_isFailed(ret)) {
        OSA_error("Operation %s failed with %d.\n", pOperation, ret);
        return ret;
    }
    return ret;
}



int main(int argc, char *argv[])
{
    int ret;
    

    OSA_info("Module %s main(), built on %s, %s.\n", OSA_MODULE_NAME, __DATE__, __TIME__);

    if (argc < 3) {
        OSA_error("Usage: %s client_ID operation [parameters]\n", argv[0]);
        return OSA_STATUS_EINVAL;
    }

    gClientId = strtol(argv[1], NULL, 0);
    ret = MEDIAC_init(gClientId, &gHandle);
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to init client %d: %d.\n", gClientId, ret);
        return ret;
    }

    ret = operate(argc, argv);
    MEDIAC_deinit(gHandle);
    return ret;
}
