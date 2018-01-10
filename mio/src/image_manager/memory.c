/*
* Created by Liu Papillon, on Dec 19, 2017.
*/

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <mio/image_manager/memory.h>

#define MIO_IMAGE_MANAGER_MEM_DEVICE_PATH                  "/dev/mediamemdev"


static int                    gFd = -1;
static int                    gRefCount = 0;


/* allow mapping multiple times */
int MIO_imageManager_memInit(void **ppMemRegion)
{
    int                    ret;    
    void                  *pMappedMem = NULL;
    

    if (NULL == ppMemRegion) {
        return OSA_STATUS_EINVAL;
    }

    if (NULL != pMappedMem) {
        *ppMemRegion = pMappedMem;
        ++gRefCount;
        return OSA_STATUS_OK;
    }

    if (gFd < 0) {
        gFd = open(MIO_IMAGE_MANAGER_MEM_DEVICE_PATH, O_RDWR | O_NONBLOCK);
        if (gFd < 0) {
            ret = errno;
            OSA_error("Failed to open video device %s: %d.\n", MIO_IMAGE_MANAGER_MEM_DEVICE_PATH, ret);
            return ret;
        }
    }

    pMappedMem = mmap(NULL, MIO_IMAGE_MANAGER_MEM_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, gFd, 0);
    if (MAP_FAILED == pMappedMem) {
        ret = errno;
        OSA_error("Failed to map video device: %d.\n", ret);
        goto _failure;
    }
    
    *ppMemRegion = pMappedMem;
    gRefCount = 1;
    OSA_info("Mapped media memory to %p with len %u by pid %ld tid %ld.\n", pMappedMem, MIO_IMAGE_MANAGER_MEM_LEN, (long)OSA_getpid(), (long)OSA_gettid());
    return OSA_STATUS_OK;

_failure:
    MIO_imageManager_memDeinit(pMappedMem);
    return ret;
}


int MIO_imageManager_memDeinit(const void *pMemRegion)
{
    if (NULL == pMemRegion) {
        return OSA_STATUS_OK;
    }

#if 0
    --gRefCount;
    if (gRefCount > 0) {
        return OSA_STATUS_OK;
    }
#endif

    munmap((void *)pMemRegion, MIO_IMAGE_MANAGER_MEM_LEN);                  

    if (gFd >= 0) {
        close(gFd);
        gFd = -1;
    }

    OSA_info("Deinited media memory at %p by pid %ld tid %ld.\n", pMemRegion, (long)OSA_getpid(), (long)OSA_gettid());    
    return OSA_STATUS_OK;
}


