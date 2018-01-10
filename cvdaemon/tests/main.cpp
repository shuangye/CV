/*
* Created by Liu Papillon, on Nov 14, 2017.
*/

#include <unistd.h>
#include <osa/osa.h>
#include <cvd/cvd.h>
#include <cvd/cvd.hpp>
#include <comm/comm.h>
#include "../src/cvd_calibrator.hpp"
#include "../src/cvd_face.hpp"



int main(int argc, char *argv[])
{
    int ret = OSA_STATUS_OK;

    OSA_info("Module %s main(), built on %s, %s.\n", OSA_MODULE_NAME, __DATE__, __TIME__);

    ret = daemon(0, 0);
    if (0 != ret) {
        OSA_warn("Failed to turn process %s to a daemon: %d.\n", OSA_MODULE_NAME, errno);
    }

    ret = CVD_init();    
    if (OSA_isFailed(ret)) {
        OSA_error("Init failed with %d.\n", ret);
        return ret;
    }

    /* for Android, an application's stdin, stdout, and stderr may be not opened by default. */
#if 1
    ret = CVD_wait();
#else   
    int c;
    bool shouldQuit = false;
    for (; ;) {
        if (shouldQuit) {
            OSA_info("Will quit...\n");
            break;
        }

        OSA_info("Input a command:\n");
        c = getchar();
        switch (c) {        
        default:
            OSA_warn("Unrecognized command %c.\n", c);
            break;
        }    
    }
#endif

    OSA_info("Module %s process will exit...\n", OSA_MODULE_NAME);
    return ret;
}

