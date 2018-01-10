#pragma once
#ifndef __MEDIAC_H__
#define __MEDIAC_H__

/*
* Created by Liu Papillon, on Jan 2, 2018.
*/

#include <osa/osa.h>


typedef Handle         MEDIAC_Handle;


#ifdef __cplusplus
extern "C" {
#endif    

    int MEDIAC_init(const int clientId, MEDIAC_Handle *pHandle);

    int MEDIAC_deinit(MEDIAC_Handle handle);

    int MEDIAC_open(MEDIAC_Handle handle);

    int MEDIAC_close(MEDIAC_Handle handle);

    int MEDIAC_setFormat(MEDIAC_Handle handle, const int frameType, const OSA_Size frameSize);

    int MEDIAC_setFrameRate(MEDIAC_Handle handle, const int frameRate);

    int MEDIAC_startStreaming(MEDIAC_Handle handle);

    int MEDIAC_stopStreaming(MEDIAC_Handle handle);

    
#ifdef __cplusplus
}
#endif

#endif  /* __MEDIAC_H__ */
