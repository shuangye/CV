#pragma once
#ifndef __EXCEPTION_HANDLER_H__
#define __EXCEPTION_HANDLER_H__

/*
* Created by Liu Papillon, on Mar 31, 2018.
*/

#include "mediad/mediad_config.h"



typedef enum MEDIAD_ExceptionType {
    MEDIAD_EXCEPTION_TYPE_NONE = 0,
    MEDIAD_EXCEPTION_TYPE_CAMERA_NO_IMAGE,
    MEDIAD_EXCEPTION_TYPE_IMAGE_MANAGER_WRITING_FAILED,
    MEDIAD_EXCEPTION_TYPE_COUNT,
} MEDIAD_ExceptionType;


typedef struct MEDIAD_Exception {
    Int32                                type;       /* of type MEDIAD_ExceptionType */
    Int32                                errorCode;
    Int32                                successiveErrorsCount;
    Int32                                producerId;
    MIO_ImageManager_ProducerHandle      producerHandles[MEDIAD_IMAGE_PRODUCERS_COUNT];
    MIO_ImageManager_Handle              imageManagerHandle;
} MEDIAD_Exception;



int MEDIAD_handleException(MEDIAD_Exception *pException);

#endif // !__EXCEPTION_HANDLER_H__

