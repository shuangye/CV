LOCAL_PATH              := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE            := mediac
# LOCAL_MODULE_FILENAME := mediac
LOCAL_CFLAGS            += -DOSA_MODULE_NAME='"MEDIAC"'
LOCAL_CFLAGS            += -DOSA_DEBUG
LOCAL_CFLAGS            += -pie -fPIE
LOCAL_C_INCLUDES        := ../../include
LOCAL_SRC_FILES         := ../../src/mediac.c
LOCAL_SRC_FILES         += ../../jni/mediac_MediacHelper.c
LOCAL_SRC_FILES         += ../../tests/main.c
LOCAL_LDLIBS            += -lz -lm
LOCAL_LDLIBS            += -llog    # enable Android local log system

# do NOT strip
# cmd-strip := 

# for shared lib:
# include $(BUILD_SHARED_LIBRARY)

# for static lib:
# include $(BUILD_STATIC_LIBRARY)

# for executable:
include $(BUILD_EXECUTABLE)
