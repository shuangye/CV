LOCAL_PATH              := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE            := cvc
# LOCAL_MODULE_FILENAME := cvc
LOCAL_CFLAGS            += -DOSA_MODULE_NAME='"CVC"'
LOCAL_CPPFLAGS          += -DOSA_MODULE_NAME='"CVC"'
LOCAL_CFLAGS            += -DHAVE_PTHREADS
LOCAL_CPPFLAGS          += -DHAVE_PTHREADS
LOCAL_CFLAGS            += -Wall
LOCAL_C_INCLUDES        := ../../include
LOCAL_SRC_FILES         := ../../src/cvc.c ../../src/cvc_comm.c
LOCAL_SRC_FILES         += ../../jni/cvc_CvcHelper.c
# LOCAL_SRC_FILES         += ../../tests/main.c
LOCAL_LDLIBS            += -pthread
LOCAL_LDLIBS            += -llog    # enable Android local log system

# do NOT strip
# cmd-strip := 

# for shared lib:
include $(BUILD_SHARED_LIBRARY)

# for static lib:
# include $(BUILD_STATIC_LIBRARY)

# for executable:
# include $(BUILD_EXECUTABLE)
