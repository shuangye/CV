#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <osa/osa.h>
#include <mio/image_manager/image_manager.h>
#include <mediad/mediad_command.h>
#include "command.h"
#include "mediad_pri.h"
#include "mediad_signal.h"

#define MEDIAD_COMMAND_MAX_CONNECTIONS_COUNT              8


static pthread_t                                  gListeningThread = 0;
static int                                        gListeningSd = -1;
static Bool                                       gListeningTasksShouldQuit;


static void *connectionMain(void *arg)
{
    int                                 ret;
    int                                 producerId;
    const int                           sd = (int)arg;
    MEDIAD_CommandMessage               message;
    ssize_t                             transmittedLen;
    const ssize_t                       kMessageLen = sizeof(message);
    MIO_ImageManager_ProducerHandle     producerHandle;
    Bool                                shouldQuit = OSA_False;


    OSA_info("Created task for a connection. Process %ld, thread %ld.\n", (long)OSA_getpid(), (long)OSA_gettid());

    for (; ;) {
        if (gListeningTasksShouldQuit || shouldQuit) {
            close(sd);
            break;
        }

        OSA_clear(&message);
        transmittedLen = recv(sd, &message, kMessageLen, 0);
        if (0 == transmittedLen) {
            close(sd);    /* connection closed by the other end */
            break;
        }

        if (transmittedLen != kMessageLen) {
            OSA_warn("Receiving message from client failed: %d.\n", errno);
            continue;
        }

        producerId = message.producerId;
        if (!OSA_isInRange(producerId, 0, OSA_arraySize(MEDIAD_gImageProducerHandles))) {
            OSA_warn("Producer ID %d exceeds valid range.\n", producerId);
            continue;
        }

        producerHandle = MEDIAD_gImageProducerHandles[producerId];
        
        switch (message.command) {
        case MEDIAD_COMMAND_DISCONNECT:
            shouldQuit = OSA_True;
            ret = OSA_STATUS_OK;
            break;
        case MEDIAD_COMMAND_OPEN:
            ret = MIO_imageManager_producerOpen(producerHandle);
            MIO_imageManager_reset(MEDIAD_gImageManagerHandle, producerId);
            break;
        case MEDIAD_COMMAND_CLOSE:
            ret = MIO_imageManager_producerClose(producerHandle);
            break;
        case MEDIAD_COMMAND_SET_FORMAT:
        {
            MIO_ImageManager_ProducerFormat config;
            OSA_clear(&config);
            config.frameType = message.args[0];
            config.frameSize.w = message.args[1];
            config.frameSize.h = message.args[2];
            ret = MIO_imageManager_producerSetFormat(producerHandle, &config);
            break;
        }        
        case MEDIAD_COMMAND_SET_FRAME_RATE:
            ret = MIO_imageManager_producerSetFrameRate(producerHandle, message.args[0]);
            break;
        case MEDIAD_COMMAND_START_IMAGE_STREAMING:
            ret = MIO_imageManager_producerStartStreaming(producerHandle);
            break;
        case MEDIAD_COMMAND_STOP_IMAGE_STREAMING:
            ret = MIO_imageManager_producerStopStreaming(producerHandle);
            break;
        default:
            OSA_warn("Unsupported command %d from client.\n", message.command);
            ret = OSA_STATUS_EINVAL;
            break;
        }

        OSA_info("Producer %d got a command %d; processing result is %d.\n", producerId, message.command, ret);

        message.status = ret;

        /* in case the sonnection is closed by the other end */
        if (0 == sigsetjmp(MEDIAD_gStackEnv, 1)) {
            transmittedLen = send(sd, &message, kMessageLen, 0);
        }
        else {
            if (MEDIAD_sigCheck(SIGPIPE)) {
                OSA_info("Connection closed by the other end.\n");
                shouldQuit = OSA_True;
            }
            else {
                OSA_info("Caught signal %d.\n", MEDIAD_getSig());
            }
        }
    }

    OSA_info("Task for a connection will quit. Process %ld, thread %ld.\n", (long)OSA_getpid(), (long)OSA_gettid());
    return NULL;
}


static void * cmdMain(void *arg)
{
    int                          ret;
    int                          acceptedSd;
    struct sockaddr_un           acceptedAddr;
    socklen_t                    acceptedAddrLen = SUN_LEN(&acceptedAddr);
    pthread_t                    connectionThread;    /* shared among every creation */

    OSA_info("Created command task. Process %ld, thread %ld.\n", (long)OSA_getpid(), (long)OSA_gettid());

    for (; ;) {        
        if (gListeningTasksShouldQuit) {
            break;
        }

        acceptedSd = accept(gListeningSd, (struct sockaddr *)&acceptedAddr, &acceptedAddrLen);
        if (acceptedSd < 0) {
            OSA_error("accept failed with %d.\n", errno);
            continue;
        }

        /* create a new thread for each connection */
        ret = pthread_create(&connectionThread, NULL, connectionMain, (void *)acceptedSd);
        if (0 != ret) {
            OSA_error("Failed to create a thread for the new connection: %d.\n", errno);
            continue;
        }
    }

    OSA_info("Command task. Process %ld thread %ld quiting...\n", (long)OSA_getpid(), (long)OSA_gettid());
    return NULL;
}


int MEDIAD_cmdInit()
{
    int ret;
    struct sockaddr_un      addr;

    gListeningSd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (gListeningSd < 0) {
        return errno;
    }
        
    unlink(MEDIAD_COMMAND_SERVER_ADDRESS);
    OSA_clear(&addr);
    addr.sun_family = AF_UNIX;
    OSA_strncpy(addr.sun_path, MEDIAD_COMMAND_SERVER_ADDRESS);
    ret = bind(gListeningSd, (struct sockaddr *)&addr, SUN_LEN(&addr));
    if (0 != ret) {
        ret = errno;
        OSA_error("Failed to bind socket to address %s: %d.\n", addr.sun_path, ret);
        goto _failure;
    }

    // ret = fchmod(gListeningSd, S_ISUID | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
    ret = chmod(addr.sun_path, S_ISUID | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
    if (0 != ret) {
        OSA_warn("Failed to change mode of server socket file %s: %d.\n", addr.sun_path, errno);
    }

    ret = listen(gListeningSd, MEDIAD_COMMAND_MAX_CONNECTIONS_COUNT);
    if (0 != ret) {
        ret = errno;
        OSA_error("Failed to listen on socket with address %s: %d.\n", addr.sun_path, ret);
        goto _failure;
    }

    gListeningTasksShouldQuit = OSA_False;
    ret = pthread_create(&gListeningThread, NULL, cmdMain, NULL);
    if (0 != ret) {
        ret = errno;
        OSA_error("Failed to listen on socket with address %s: %d.\n", addr.sun_path, ret);
        goto _failure;
    }

    return OSA_STATUS_OK;

_failure:
    return ret;
}


int MEDIAD_cmdDeinit()
{
    gListeningTasksShouldQuit = OSA_True;
    MEDIAD_cmdWait();

    if (gListeningSd >= 0) {
        close(gListeningSd);
        gListeningSd = -1;
    }

    return OSA_STATUS_OK;
}


int MEDIAD_cmdWait()
{
    pthread_join(gListeningThread, NULL);
    return OSA_STATUS_OK;
}
