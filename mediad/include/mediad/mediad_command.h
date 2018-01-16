#pragma once
#pragma once
#ifndef __MEDIAD_COMMAND_H__
#define __MEDIAD_COMMAND_H__

/*
* Created by Liu Papillon, on Dec 26, 2017.
*/

#include <osa/osa.h>

#ifdef OSA_OS_ANDROID
#define MEDIAD_COMMAND_SERVER_ADDRESS                     "/data/rk_backup/mediad.sock"
#else
#define MEDIAD_COMMAND_SERVER_ADDRESS                     "mediad.sock"
#endif

/* `SUN_LEN` may not be defined on Android */
#ifndef SUN_LEN
/* Evaluate to actual length of the `sockaddr_un' structure.  */
#include <string.h>
#define SUN_LEN(ptr) ((size_t) (((struct sockaddr_un *) 0)->sun_path) + strlen ((ptr)->sun_path))
#endif


#ifdef __cplusplus
extern "C" {
#endif 

    typedef enum MEDIAD_Command {
        MEDIAD_COMMAND_NONE                             = 0,
        MEDIAD_COMMAND_DISCONNECT                       = 1,
        MEDIAD_COMMAND_OPEN                             = 2,
        MEDIAD_COMMAND_CLOSE                            = 3,
        MEDIAD_COMMAND_SET_FORMAT                       = 4,
        MEDIAD_COMMAND_SET_FRAME_RATE                   = 5,
        MEDIAD_COMMAND_START_IMAGE_STREAMING            = 6,
        MEDIAD_COMMAND_STOP_IMAGE_STREAMING             = 7,
        MEDIAD_COMMAND_COUNT,
    } MEDIAD_Command;


    typedef struct MEDIAD_CommandMessage {
        Int32                           producerId;
        Int32                           command;    /* MEDIAD_Command */
        Int32                           status;
        Uint32                          needsAck : 1;
        Uint32                          padding1 : 31;
        Int32                           args[8];
    } MEDIAD_CommandMessage;


#ifdef __cplusplus
}
#endif

#endif  /* __MEDIAD_COMMAND_H__ */
