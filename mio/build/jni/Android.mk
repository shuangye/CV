LOCAL_PATH              := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE            := mio
# LOCAL_MODULE_FILENAME := mio
LOCAL_CFLAGS            += -DOSA_MODULE_NAME='"MIO"'
LOCAL_CPPFLAGS          += -DOSA_MODULE_NAME='"MIO"'
LOCAL_C_INCLUDES        := ../../include
LOCAL_SRC_FILES         := ../../src/image_manager/image_manager.c ../../src/image_manager/memory.c ../../src/image_manager/producer_v4l2.c ../../src/mio_utils.c ../../src/mjpeg_decoder.c
# LOCAL_SRC_FILES         := ../../src/cvio_camera.cpp
# LOCAL_SRC_FILES       += ../../tests/main.cpp
LOCAL_LDLIBS            += -lz -lm
LOCAL_LDLIBS            += -L../../libs/armeabi-v7a -lmjpegdecode_hard
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
