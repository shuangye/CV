LOCAL_PATH              := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE            := face
# LOCAL_MODULE_FILENAME := face
LOCAL_CFLAGS            += -DOSA_MODULE_NAME='"FACE"'
LOCAL_CPPFLAGS          += -DOSA_MODULE_NAME='"FACE"'
LOCAL_CPPFLAGS          += -std=c++11
LOCAL_C_INCLUDES        := ../../include ../../../dscv/include D:/CV/opencv-2.4.13.4-android-sdk/sdk/native/jni/include
LOCAL_SRC_FILES         := ../../src/face.cpp ../../src/face_c.cpp ../../src/face_detect.cpp ../../src/face_living.cpp ../../src/face_stereo.cpp
# LOCAL_SRC_FILES         += ../../src/jni/sv_StereoVisionHelper.c
# LOCAL_SRC_FILES       += ../../tests/main.cpp
LOCAL_LDLIBS            += -L../../../dscv/build/libs/armeabi-v7a -ldscv
LOCAL_LDLIBS            += -LD:/CV/opencv-2.4.13.4-android-sdk/sdk/native/libs/armeabi-v7a
LOCAL_LDLIBS            += -lz -lm -lstdc++
LOCAL_LDLIBS            += -lopencv_core -lopencv_java
LOCAL_LDLIBS            += -llog    # enable Android local log system
# LOCAL_SHARED_LIBRARIES := libxtract


# do NOT strip
# cmd-strip := 

# for shared lib:
include $(BUILD_SHARED_LIBRARY)

# for static lib:
# include $(BUILD_STATIC_LIBRARY)

# for executable:
# include $(BUILD_EXECUTABLE)
