#include <osa/osa.h>
#include "mediad_signal.h"



sigjmp_buf MEDIAD_gStackEnv;


static int gCaughtSignum = 0;


static void sigHandler(int signum)
{
    /* don't call any std functions here */
    gCaughtSignum = signum;
    siglongjmp(MEDIAD_gStackEnv, 1);
}


int MEDIAD_sigInit()
{
    signal(SIGPIPE, sigHandler);
    return OSA_STATUS_OK;
}


int MEDIAD_sigDeinit()
{
    /* TODO: restore original signal handler */
    return OSA_STATUS_OK;
}


int MEDIAD_getSig()
{
    return gCaughtSignum;
}


int MEDIAD_sigCheck(const int signum)
{
    return signum == gCaughtSignum;
}


void MEDIAD_sigClear(const int signum)
{
    if (signum == gCaughtSignum) {
        gCaughtSignum = 0;
    }
}
