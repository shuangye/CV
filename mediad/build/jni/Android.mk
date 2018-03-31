LOCAL_PATH              := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE            := mediad
# LOCAL_MODULE_FILENAME := mediad
LOCAL_CFLAGS            += -DOSA_MODULE_NAME='"MEDIAD"'
LOCAL_CFLAGS            += -DOSA_DEBUG
LOCAL_CFLAGS            += -DHAVE_PTHREADS
LOCAL_CFLAGS            += -pie -fPIE
LOCAL_C_INCLUDES        := ../../include
LOCAL_SRC_FILES         := ../../src/command.c ../../src/mediad_signal.c ../../src/mediad.c ../../src/exception_handler.c
# LOCAL_SRC_FILES         += ../../src/image_consumer_dummy.c 
LOCAL_SRC_FILES         += ../../src/debug.c 
LOCAL_SRC_FILES         += ../../tests/main.c
LOCAL_LDLIBS            += -lz -lm -lstdc++
LOCAL_LDLIBS            += -pthread
LOCAL_LDLIBS            += -llog    # enable Android local log system
LOCAL_LDLIBS            += -L../../libs/armeabi-v7a/ -ldscv -lmio

# do NOT strip
# cmd-strip := 

# for shared lib:
# include $(BUILD_SHARED_LIBRARY)

# for static lib:
# include $(BUILD_STATIC_LIBRARY)

# for executable:
include $(BUILD_EXECUTABLE)
