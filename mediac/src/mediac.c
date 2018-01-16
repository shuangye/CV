/*
* Created by Liu Papillon, on Jan 2, 2018.
*/

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <osa/osa.h>
#include <mediad/mediad.h>
#include <mediac/mediac.h>

#define MEDIAC_COMMAND_MESSAGE_LEN                 (sizeof(MEDIAD_CommandMessage))
#define SOCKET_DIR                                 "/data/rk_backup/"


/************************************ type definitions ************************************/


typedef struct MEDIAC_Mediac {
    int                         id;
    int                         sd;
    Char                        localSocketPath[PATH_MAX];
} MEDIAC_Mediac;


/************************************ global variables ************************************/



/************************************ local functions ************************************/


static int sendMessage(MEDIAC_Mediac *pMediac, MEDIAD_CommandMessage *pMessage)
{
    int                           ret;
    MEDIAD_CommandMessage         response;
    ssize_t                       transmittedLen;

    OSA_info("Will send command %d.\n", pMessage->command);
    transmittedLen = send(pMediac->sd, pMessage, MEDIAC_COMMAND_MESSAGE_LEN, 0);
    if (transmittedLen != MEDIAC_COMMAND_MESSAGE_LEN) {
        return errno;
    }

    if (!pMessage->needsAck) {
        return OSA_STATUS_OK;
    }

    transmittedLen = recv(pMediac->sd, &response, MEDIAC_COMMAND_MESSAGE_LEN, 0);
    if (transmittedLen != MEDIAC_COMMAND_MESSAGE_LEN) {
        return errno;
    }
    return response.status;
}


/************************************ public functions ************************************/


int MEDIAC_init(const int clientId, MEDIAC_Handle *pHandle)
{
    int                           ret;
    struct sockaddr_un            localAddr, remoteAddr;
    MEDIAC_Mediac                *pMediac = NULL;


    if (NULL == pHandle) {
        return OSA_STATUS_EINVAL;
    }

    pMediac = calloc(1, sizeof(*pMediac));
    if (NULL == pMediac) {
        return OSA_STATUS_ENOMEM;
    }

    pMediac->sd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (pMediac->sd < 0) {
        ret = errno;
        OSA_error("Failed to create socket: %d.\n", ret);
        goto _failure;
    }

    snprintf(pMediac->localSocketPath, sizeof(pMediac->localSocketPath), "%s/%s_%ld_%d.sock", SOCKET_DIR, OSA_MODULE_NAME, (long)OSA_gettid(), clientId);
    unlink(pMediac->localSocketPath);
    OSA_clear(&localAddr);
    localAddr.sun_family = AF_UNIX;
    OSA_strncpy(localAddr.sun_path, pMediac->localSocketPath);
    ret = bind(pMediac->sd, (struct sockaddr *)&localAddr, SUN_LEN(&localAddr));
    if (0 != ret) {
        ret = errno;
        OSA_error("Failed to bind socket to address %s: %d.\n", localAddr.sun_path, ret);
        goto _failure;
    }

    OSA_clear(&remoteAddr);
    remoteAddr.sun_family = AF_UNIX;
    OSA_strncpy(remoteAddr.sun_path, MEDIAD_COMMAND_SERVER_ADDRESS);
    ret = connect(pMediac->sd, (struct sockaddr *)&remoteAddr, SUN_LEN(&remoteAddr));
    if (0 != ret) {
        ret = errno;
        OSA_error("Connecting to remote %s failed with %d.\n", remoteAddr.sun_path, ret);
        goto _failure;
    }

    pMediac->id = clientId;
    *pHandle = pMediac;

    OSA_info("Pid %ld tid %ld, media client %d inited.\n", (long)OSA_getpid(), (long)OSA_gettid(), clientId);
    return OSA_STATUS_OK;

_failure:
    MEDIAC_deinit(pMediac);
    return ret;
}


int MEDIAC_deinit(MEDIAC_Handle handle)
{
    MEDIAC_Mediac                *pMediac = (MEDIAC_Mediac *)handle;
    MEDIAD_CommandMessage         message;


    if (NULL == pMediac) {
        return OSA_STATUS_OK;
    }

    if (pMediac->sd >= 0) {
        OSA_clear(&message);
        message.producerId = pMediac->id;
        message.command = MEDIAD_COMMAND_DISCONNECT;
        sendMessage(pMediac, &message);
        close(pMediac->sd);
        pMediac->sd = -1;
    }

    unlink(pMediac->localSocketPath);
    
    OSA_info("Pid %ld tid %ld, media client %d deinited.\n", (long)OSA_getpid(), (long)OSA_gettid(), pMediac->id);

    free(pMediac);    

    return OSA_STATUS_OK;
}


int MEDIAC_open(MEDIAC_Handle handle)
{
    MEDIAC_Mediac                *pMediac = (MEDIAC_Mediac *)handle;
    MEDIAD_CommandMessage         message;
    
    if (NULL == pMediac) {
        return OSA_STATUS_EINVAL;
    }

    OSA_clear(&message);
    message.producerId = pMediac->id;
    message.command = MEDIAD_COMMAND_OPEN;
    message.needsAck = 1;
    return sendMessage(pMediac, &message);
}


int MEDIAC_close(MEDIAC_Handle handle)
{
    MEDIAC_Mediac                *pMediac = (MEDIAC_Mediac *)handle;
    MEDIAD_CommandMessage         message;
    
    if (NULL == pMediac) {
        return OSA_STATUS_EINVAL;
    }

    OSA_clear(&message);
    message.producerId = pMediac->id;
    message.command = MEDIAD_COMMAND_CLOSE;
    return sendMessage(pMediac, &message);
}


int MEDIAC_setFormat(MEDIAC_Handle handle, const int frameType, const OSA_Size frameSize)
{
    MEDIAC_Mediac                *pMediac = (MEDIAC_Mediac *)handle;
    MEDIAD_CommandMessage         message;

    if (NULL == pMediac) {
        return OSA_STATUS_EINVAL;
    }

    OSA_clear(&message);
    message.producerId = pMediac->id;
    message.command = MEDIAD_COMMAND_SET_FORMAT;
    message.needsAck = 1;
    message.args[0] = frameType;
    message.args[1] = frameSize.w;
    message.args[2] = frameSize.h;
    return sendMessage(pMediac, &message);
}


int MEDIAC_setFrameRate(MEDIAC_Handle handle, const int frameRate)
{
    MEDIAC_Mediac                *pMediac = (MEDIAC_Mediac *)handle;
    MEDIAD_CommandMessage         message;

    if (NULL == pMediac) {
        return OSA_STATUS_EINVAL;
    }

    OSA_clear(&message);
    message.producerId = pMediac->id;
    message.command = MEDIAD_COMMAND_SET_FRAME_RATE;
    message.needsAck = 1;
    message.args[0] = frameRate;
    return sendMessage(pMediac, &message);
}


int MEDIAC_startStreaming(MEDIAC_Handle handle)
{
    MEDIAC_Mediac                *pMediac = (MEDIAC_Mediac *)handle;
    MEDIAD_CommandMessage         message;

    if (NULL == pMediac) {
        return OSA_STATUS_EINVAL;
    }

    OSA_clear(&message);
    message.producerId = pMediac->id;
    message.command = MEDIAD_COMMAND_START_IMAGE_STREAMING;
    message.needsAck = 1;
    return sendMessage(pMediac, &message);
}


int MEDIAC_stopStreaming(MEDIAC_Handle handle)
{
    MEDIAC_Mediac                *pMediac = (MEDIAC_Mediac *)handle;
    MEDIAD_CommandMessage         message;

    if (NULL == pMediac) {
        return OSA_STATUS_EINVAL;
    }

    OSA_clear(&message);
    message.producerId = pMediac->id;
    message.command = MEDIAD_COMMAND_STOP_IMAGE_STREAMING;
    message.needsAck = 1;
    return sendMessage(pMediac, &message);
}

