/*
* Created by Liu Papillon, on Nov 14, 2017.
*/

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <osa/osa.h>
#include <cvc/cvc.h>


int main(int argc, char *argv[])
{
    int ret;
    char c;
    Bool shouldQuit = 0;
    OSA_Rect face;
    Int32 possibility;
    void *pEncodedFace = NULL;
    const size_t encodedFaceBufferLen = 1 * OSA_MB;
    size_t len;


    OSA_info("CV client initializing...\n");

    ret = CVC_init();
    if (OSA_isFailed(ret)) {
        OSA_error("Failed to init: %d.\n", ret);
        return ret;
    }

    pEncodedFace = malloc(encodedFaceBufferLen);
    assert(NULL != pEncodedFace);        

    OSA_info("Input a command:\n");

    for (; ;) {
        if (shouldQuit) {
            OSA_info("Will quit...\n");
            break;
        }

        c = getchar();
        switch (c) {
        case 'i':
            ret = CVC_calibratorInit(9, 6);
            break;
        case 'c':
            ret = CVC_calibratorCollectImage();
            break;
        case 'C':
            ret = CVC_calibratorStereoCalibrate();
            break;
        case 'd':
            ret = CVC_detectFace(0, &face);
            break;
        case 'l':
            len = encodedFaceBufferLen;
            ret = CVC_determineLivingFace(&possibility, pEncodedFace, &len);
            break;
        case 'q':
            shouldQuit = 1;
            break;
        default:
            break;
        }    
    }

    free(pEncodedFace);
    return CVC_deinit();
}
