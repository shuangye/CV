#pragma once
#ifndef __MEDIAD_DEBUG_PRIVATE_H__
#define __MEDIAD_DEBUG_PRIVATE_H__

/*
* Created by Liu Papillon, on Dec 22, 2017.
*/

#include <osa/osa.h>

#define MEDIAD_DUMP_DIR      "/data/rk_backup/"


#ifdef __cplusplus
extern "C" {
#endif    

    int MEDIAD_debugDumpData(const Char *pLabel, const void *pData, const size_t len);
   
#ifdef __cplusplus
}
#endif

#endif  /* __MEDIAD_DEBUG_PRIVATE_H__ */
