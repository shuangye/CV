/*
* Created by Liu Papillon, on Jan 4, 2018.
*/


#include <osa/osa.h>
#include <dscv/dscv.h>
#include <mediac/mediac.h>
#include "mediac_MediacHelper.h"


JNIEXPORT jint JNICALL Java_mediac_MediacHelper_MEDIAC_1init(JNIEnv *env, jclass clazz, jint jClientId, jlongArray jHandle)
{
    int ret;
    MEDIAC_Handle handle;

    (void)env;
    (void)clazz;

#if 1
    ret = MEDIAC_init(jClientId, &handle);
#else
    handle = (MEDIAC_Handle)0x12345678;
#endif
    if (OSA_isSucceeded(ret)) {
        (*env)->SetLongArrayRegion(env, jHandle, 0, 1, (jlong *)&handle);
    }
    return ret;
}


JNIEXPORT jint JNICALL Java_mediac_MediacHelper_MEDIAC_1deinit(JNIEnv *env, jclass clazz, jlong jHandle)
{
    (void)env;
    (void)clazz;
    return MEDIAC_deinit((MEDIAC_Handle)jHandle);
}


JNIEXPORT jint JNICALL Java_mediac_MediacHelper_MEDIAC_1open(JNIEnv *env, jclass clazz, jlong jHandle)
{
    (void)env;
    (void)clazz;
    return MEDIAC_open((MEDIAC_Handle)jHandle);
}


JNIEXPORT jint JNICALL Java_mediac_MediacHelper_MEDIAC_1close(JNIEnv *env, jclass clazz, jlong jHandle)
{
    (void)env;
    (void)clazz;
    return MEDIAC_close((MEDIAC_Handle)jHandle);
}


JNIEXPORT jint JNICALL Java_mediac_MediacHelper_MEDIAC_1setFormat(JNIEnv *env, jclass clazz, jlong jHandle, jint jW, jint jH)
{
    (void)env;
    (void)clazz;
    OSA_Size frameSize = { .w = jW, .h = jH };
    return MEDIAC_setFormat((MEDIAC_Handle)jHandle, DSCV_FRAME_TYPE_YUYV, frameSize);
}


JNIEXPORT jint JNICALL Java_mediac_MediacHelper_MEDIAC_1setFrameRate(JNIEnv *env, jclass clazz, jlong jHandle, jint jFrameRate)
{
    (void)env;
    (void)clazz;
    return MEDIAC_setFrameRate((MEDIAC_Handle)jHandle, jFrameRate);
}


JNIEXPORT jint JNICALL Java_mediac_MediacHelper_MEDIAC_1startStreaming(JNIEnv *env, jclass clazz, jlong jHandle)
{
    (void)env;
    (void)clazz;
    return MEDIAC_startStreaming((MEDIAC_Handle)jHandle);
}


JNIEXPORT jint JNICALL Java_mediac_MediacHelper_MEDIAC_1stopStreaming(JNIEnv *env, jclass clazz, jlong jHandle)
{
    (void)env;
    (void)clazz;
    return MEDIAC_stopStreaming((MEDIAC_Handle)jHandle);
}
