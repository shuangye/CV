LOCAL_PATH              := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE            := dscv
# LOCAL_MODULE_FILENAME := dscv
LOCAL_CFLAGS            += -DOSA_MODULE_NAME='"DSCV"'
LOCAL_CPPFLAGS          += -DOSA_MODULE_NAME='"DSCV"'
LOCAL_C_INCLUDES        :=  ../../include D:/CV/opencv-2.4.13.4-android-sdk/sdk/native/jni/include 
LOCAL_SRC_FILES         := ../../src/dscv_calibrator.cpp ../../src/dscv_frame.cpp ../../src/dscv_frame_c.c ../../src/dscv_matutils.cpp ../../src/dscv_rectifier.cpp ../../src/dscv_rectutils.cpp
# LOCAL_SRC_FILES         += ../../src/jni/sv_StereoVisionHelper.c
# LOCAL_SRC_FILES       += ../../tests/main.cpp
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
