#pragma once
#ifndef __CVC_CONFIG_H__
#define __CVC_CONFIG_H__

/*
* Created by Liu Papillon, on Nov 14, 2017.
*/


#define CVC_MODULE_ABBREVIATION                 "CVC"

#ifdef OSA_OS_GNU_LINUX
#define CVC_UNIX_SOCKET_DIR                     "/tmp/"
#else
#define CVC_UNIX_SOCKET_DIR                     "/data/cv/"    /* Android may have no /tmp/ dir */
#endif


#ifdef __cplusplus
extern "C" {
#endif      



#ifdef __cplusplus
}
#endif

#endif  /* __CVC_CONFIG_H__ */


