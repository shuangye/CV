/*
* Created by Liu Papillon, on Dec 22, 2017.
*/

#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <osa/osa.h>
#include "debug.h"
#include "config_pri.h"


int MEDIAD_debugDumpData(const Char *pLabel,  const void *pData, const size_t len)
{
    long timestamp;
    Char dumpFilePath[MEDIAD_PATH_MAX_LEN]; 
    FILE *fp = NULL;
    static unsigned int   frameIndex = 0;


    if (NULL == pLabel || NULL == pData) {
        return OSA_STATUS_EINVAL;
    }

    snprintf(dumpFilePath, sizeof(dumpFilePath), "%s/%s", MEDIAD_DUMP_DIR, pLabel);
    if (0 != access(dumpFilePath, F_OK)) {
        return OSA_STATUS_EPERM;
    }
    
    timestamp = (long)time(NULL);
    snprintf(dumpFilePath, sizeof(dumpFilePath), "%s/%s_index%u_%ld", MEDIAD_DUMP_DIR, pLabel, frameIndex, timestamp);
    fp = fopen(dumpFilePath, "w");
    if (NULL == fp) {
        return OSA_STATUS_EINVAL;
    }

    fwrite(pData, 1, len, fp);
    fclose(fp);

    ++frameIndex;

    return OSA_STATUS_OK;
}
