/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class mediac_MediacHelper */

#ifndef _Included_mediac_MediacHelper
#define _Included_mediac_MediacHelper
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     mediac_MediacHelper
 * Method:    MEDIAC_init
 * Signature: (I[J)I
 */
JNIEXPORT jint JNICALL Java_mediac_MediacHelper_MEDIAC_1init
  (JNIEnv *, jclass, jint, jlongArray);

/*
 * Class:     mediac_MediacHelper
 * Method:    MEDIAC_deinit
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_mediac_MediacHelper_MEDIAC_1deinit
  (JNIEnv *, jclass, jlong);

/*
 * Class:     mediac_MediacHelper
 * Method:    MEDIAC_open
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_mediac_MediacHelper_MEDIAC_1open
  (JNIEnv *, jclass, jlong);

/*
 * Class:     mediac_MediacHelper
 * Method:    MEDIAC_close
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_mediac_MediacHelper_MEDIAC_1close
  (JNIEnv *, jclass, jlong);

/*
 * Class:     mediac_MediacHelper
 * Method:    MEDIAC_setFormat
 * Signature: (JII)I
 */
JNIEXPORT jint JNICALL Java_mediac_MediacHelper_MEDIAC_1setFormat
  (JNIEnv *, jclass, jlong, jint, jint);

/*
 * Class:     mediac_MediacHelper
 * Method:    MEDIAC_setFrameRate
 * Signature: (JI)I
 */
JNIEXPORT jint JNICALL Java_mediac_MediacHelper_MEDIAC_1setFrameRate
  (JNIEnv *, jclass, jlong, jint);

/*
 * Class:     mediac_MediacHelper
 * Method:    MEDIAC_startStreaming
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_mediac_MediacHelper_MEDIAC_1startStreaming
  (JNIEnv *, jclass, jlong);

/*
 * Class:     mediac_MediacHelper
 * Method:    MEDIAC_stopStreaming
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_mediac_MediacHelper_MEDIAC_1stopStreaming
  (JNIEnv *, jclass, jlong);

#ifdef __cplusplus
}
#endif
#endif
