#include <cvc/cvc.h>
#include "cvc_CvcHelper.h"


JNIEXPORT jint JNICALL Java_cvc_CvcHelper_CVC_1init(JNIEnv *env, jclass _class)
{
    (void)env;
    (void)_class;
    return CVC_init();
}


JNIEXPORT jint JNICALL Java_cvc_CvcHelper_CVC_1deinit(JNIEnv *env, jclass _class)
{
    (void)env;
    (void)_class;
    return CVC_deinit();
}


JNIEXPORT jint JNICALL Java_cvc_CvcHelper_CVC_1calibratorInit(JNIEnv *env, jclass _class, jint jW, jint jH)
{
    (void)env;
    (void)_class;
    return CVC_calibratorInit(jW, jH);
}


JNIEXPORT jint JNICALL Java_cvc_CvcHelper_CVC_1calibratorCollectImage(JNIEnv *env, jclass _class)
{
    (void)env;
    (void)_class;
    return CVC_calibratorCollectImage();
}


JNIEXPORT jint JNICALL Java_cvc_CvcHelper_CVC_1calibratorStereoCalibrate(JNIEnv *env, jclass _class)
{
    (void)env;
    (void)_class;
    return CVC_calibratorStereoCalibrate();
}


JNIEXPORT jint JNICALL Java_cvc_CvcHelper_CVC_1detectFace(JNIEnv *env, jclass _class, jint jId, jobject jFace, jintArray jRelativeW, jintArray jRelativeH)
{
    int ret;
    jclass clazz;
    jfieldID fid;
    OSA_Rect face;
    OSA_Size relativeSize;

    
    clazz = (*env)->GetObjectClass(env, jFace);
    if (0 == clazz) {
        printf("GetObjectClass returned 0\n");
        return (-1);
    }
    
    ret = CVC_detectFace(jId, &face, &relativeSize);
    if (0 == ret) {
        fid = (*env)->GetFieldID(env, clazz, "w", "I");
        (*env)->SetIntField(env, jFace, fid, face.size.w);
        fid = (*env)->GetFieldID(env, clazz, "h", "I");
        (*env)->SetIntField(env, jFace, fid, face.size.h);
        fid = (*env)->GetFieldID(env, clazz, "x", "I");
        (*env)->SetIntField(env, jFace, fid, face.origin.x);
        fid = (*env)->GetFieldID(env, clazz, "y", "I");
        (*env)->SetIntField(env, jFace, fid, face.origin.y);
        (*env)->SetIntArrayRegion(env, jRelativeW, 0, 1, &relativeSize.w);
        (*env)->SetIntArrayRegion(env, jRelativeH, 0, 1, &relativeSize.h);
    }

    return ret;
}


JNIEXPORT jint JNICALL Java_cvc_CvcHelper_CVC_1setLivingFaceThreshold(JNIEnv *env, jclass _class, jint jThreshold)
{
    return CVC_setLivingFaceThreshold(jThreshold);
}


JNIEXPORT jint JNICALL Java_cvc_CvcHelper_CVC_1determineLivingFace(JNIEnv *env, jclass _class, jintArray jPossibility, jbyteArray jFace, jintArray jLen)
{
    (void)env;
    (void)_class;
    
    int ret;
    Int32 possibility = 0;
    void *pFace = NULL;
    size_t len = 0;


    pFace = (*env)->GetByteArrayElements(env, jFace, 0);
    (*env)->GetIntArrayRegion(env, jLen, 0, 1, &len);
    
    ret = CVC_determineLivingFace(&possibility, pFace, &len);
    if (OSA_isSucceeded(ret)) {
        (*env)->SetIntArrayRegion(env, jPossibility, 0, 1, &possibility);
        (*env)->SetIntArrayRegion(env, jLen, 0, 1, &len);
    }

    return ret;
}

