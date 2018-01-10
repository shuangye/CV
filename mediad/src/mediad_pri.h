#pragma once
#ifndef __MEDIAD_PRI_H__
#define __MEDIAD_PRI_H__

/*
* Created by Liu Papillon, on Dec 21, 2017.
*/


#include <mio/image_manager/image_manager.h>
#include <mediad/mediad_config.h>


#ifdef __cplusplus
extern "C" {
#endif    

    extern MIO_ImageManager_ProducerHandle                        MEDIAD_gImageProducerHandles[MEDIAD_IMAGE_PRODUCERS_COUNT];
    extern MIO_ImageManager_Handle                                MEDIAD_gImageManagerHandle;


    int MEDIAD_init();

    int MEDIAD_deinit();

    int MEDIAD_wait();


#ifdef __cplusplus
}
#endif

#endif  /* __MEDIAD_PRI_H__ */
