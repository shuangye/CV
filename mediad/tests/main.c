/*
* Created by Liu Papillon, on Dec 19, 2017.
*/


#include <unistd.h>
#include <osa/osa.h>
#include "../src/mediad_pri.h"
#include "../src/image_consumer.h"
#include "../src/mediad_signal.h"


int main(int argc, char *argv[])
{
    int ret = OSA_STATUS_OK;

    OSA_info("Module %s main(), built on %s, %s.\n", OSA_MODULE_NAME, __DATE__, __TIME__);

    ret = daemon(0, 0);
    if (0 != ret) {
        OSA_warn("Failed to turn process %s to a daemon: %d.\n", OSA_MODULE_NAME, errno);
    }

    MEDIAD_sigInit();
    
    ret = MEDIAD_init();
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to init: %d.\n", ret);
    }

    OSA_info("Module %s process %ld started.\n", OSA_MODULE_NAME, (long)OSA_getpid());

#if 0
    ret = MEDIAD_imageConsumerInit();
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to init consumer: %d.\n", ret);
    }
#endif
    
    ret = MEDIAD_wait();
    
    // MEDIAD_imageConsumerDeinit();
   
    ret = MEDIAD_deinit();

    MEDIAD_sigDeinit();

    OSA_info("Module %s process %ld will exit...\n", OSA_MODULE_NAME, (long)OSA_getpid());
    return ret;
}
