LOCAL_PATH              := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE            := cvd
# LOCAL_MODULE_FILENAME := cvd
LOCAL_CFLAGS            += -DOSA_DEBUG
LOCAL_CPPFLAGS          += -DOSA_DEBUG
LOCAL_CFLAGS            += -DOSA_MODULE_NAME='"CVD"'
LOCAL_CPPFLAGS          += -DOSA_MODULE_NAME='"CVD"'
LOCAL_CFLAGS            += -DHAVE_PTHREADS
LOCAL_CFLAGS            += -pie -fPIE
LOCAL_CPPFLAGS          += -DHAVE_PTHREADS
LOCAL_CPPFLAGS          += -std=c++11
LOCAL_CPPFLAGS          += -pie -fPIE
LOCAL_C_INCLUDES        := ../../include ../../../dscv/include D:/CV/opencv-2.4.13.4-android-sdk/sdk/native/jni/include
LOCAL_SRC_FILES         := ../../src/cvd.cpp ../../src/cvd_comm.cpp ../../src/cvd_calibrator.cpp  ../../src/cvd_face.cpp ../../src/cvd_image.cpp 
# LOCAL_SRC_FILES         += ../../src/cvd_image_provider.c
LOCAL_SRC_FILES         += ../../src/cvd_debug.cpp
LOCAL_SRC_FILES         += ../../tests/main.cpp
LOCAL_LDLIBS            += -lz -lm -lstdc++
LOCAL_LDLIBS            += -pthread
LOCAL_LDLIBS            += -LD:/CV/opencv-2.4.13.4-android-sdk/sdk/native/libs/armeabi-v7a
LOCAL_LDLIBS            += -lopencv_core -lopencv_java
LOCAL_LDLIBS            += -llog    # enable Android local log system
LOCAL_LDLIBS            += -L../../libs/armeabi-v7a/ -ldscv -lface -lmio

# do NOT strip
# cmd-strip := 

# for shared lib:
# include $(BUILD_SHARED_LIBRARY)

# for static lib:
# include $(BUILD_STATIC_LIBRARY)

# for executable:
include $(BUILD_EXECUTABLE)
