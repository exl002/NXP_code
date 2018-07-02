
LOCAL_PATH := $(call my-dir)


# blhost application
#
include $(CLEAR_VARS)
LOCAL_MODULE := blhost_nova

LOCAL_SRC_FILES := src/blhost.cpp

LOCAL_SHARED_LIBRARIES := \
  libc \
  libusb-1.0

LOCAL_STATIC_LIBRARIES := \
  libblfwk \
  libblcrc

LOCAL_CFLAGS := -DFC_DBG_LOG_TAG_NAME='"blhost"'
LOCAL_CFLAGS += -std=c99 -D LINUX -D BOOTLOADER_HOST -D _GNU_SOURCE
LOCAL_CPPFLAGS += -D LINUX -D BOOTLOADER_HOST -std=c++11 -fexceptions -Wno-non-virtual-dtor -Woverloaded-virtual -frtti

include $(BUILD_EXECUTABLE)


