#pragma once
#ifndef __MEDIAD_CONFIG_PRI_H__
#define __MEDIAD_CONFIG_PRI_H__

/*
* Created by Liu Papillon, on Dec 22, 2017.
*/

#include <limits.h>
#include <osa/osa.h>

#ifdef OSA_OS_WINDOWS
#define MEDIAD_PATH_MAX_LEN              MAX_PATH
#else
#define MEDIAD_PATH_MAX_LEN              PATH_MAX
#endif // !OSA_OS_WINDOWS


#define MEDIAD_MAX_SUCCESSIVE_CAMERA_FAILURES_COUNT    500


#ifdef __cplusplus
extern "C" {
#endif    

#ifdef __cplusplus
}
#endif

#endif  /* __MEDIAD_CONFIG_PRI_H__ */
