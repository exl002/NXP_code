
LOCAL_PATH := $(call my-dir)

#
# blfwk library
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  src/crc16.c \
  src/crc32.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/..
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../drivers/common

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_C_INCLUDES)

LOCAL_MODULE := libblcrc

LOCAL_CFLAGS := -std=c99 -D LINUX -D BOOTLOADER_HOST -D _GNU_SOURCE

include $(BUILD_STATIC_LIBRARY)




