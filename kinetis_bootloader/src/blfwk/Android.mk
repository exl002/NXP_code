
LOCAL_PATH := $(call my-dir)
BL_PATH := $(LOCAL_PATH)/..

# BL Framework Includes
LOCAL_BLFWK_INCLUDES := $(BL_PATH)
LOCAL_BLFWK_INCLUDES += $(BL_PATH)/include
LOCAL_BLFWK_INCLUDES += $(BL_PATH)/blfwk
LOCAL_BLFWK_INCLUDES += $(BL_PATH)/sbloader
LOCAL_BLFWK_INCLUDES += $(BL_PATH)/bootloader
LOCAL_BLFWK_INCLUDES += $(BL_PATH)/crc
LOCAL_BLFWK_INCLUDES += $(BL_PATH)/packet
LOCAL_BLFWK_INCLUDES += $(BL_PATH)/property
LOCAL_BLFWK_INCLUDES += $(BL_PATH)/drivers/common
LOCAL_BLFWK_INCLUDES += $(BL_PATH)/bm_usb


# BL Framework Source Files
LOCAL_BLFWK_SRC_FILES := \
  src/Blob.cpp \
  src/Bootloader.cpp \
  src/BusPal.cpp \
  src/BusPalPeripheral.cpp \
  src/Command.cpp \
  src/DataSource.cpp \
  src/DataSourceImager.cpp \
  src/DataTarget.cpp \
  src/ELFSourceFile.cpp \
  src/ExcludesListMatcher.cpp \
  src/format_string.cpp \
  src/GHSSecInfo.cpp \
  src/GlobMatcher.cpp \
  src/hid-android.c \
  src/jsoncpp.cpp \
  src/Logging.cpp \
  src/options.cpp \
  src/SBSourceFile.cpp  \
  src/SearchPath.cpp  \
  src/serial.c \
  src/SerialPacketizer.cpp \
  src/SourceFile.cpp \
  src/SRecordSourceFile.cpp \
  src/IntelHexSourceFile.cpp \
  src/StELFFile.cpp \
  src/StExecutableImage.cpp \
  src/StSRecordFile.cpp \
  src/StIntelHexFile.cpp \
  src/Updater.cpp \
  src/UartPeripheral.cpp \
  src/UsbHidPacketizer.cpp \
  src/UsbHidPeripheral.cpp \
  src/utils.cpp \
  src/Value.cpp \



# blfwk library
#
include $(CLEAR_VARS)
LOCAL_MODULE := libblfwk

LOCAL_C_INCLUDES := $(LOCAL_BLFWK_INCLUDES)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_BLFWK_INCLUDES)

LOCAL_SRC_FILES := $(LOCAL_BLFWK_SRC_FILES)

LOCAL_CFLAGS := -DFC_DBG_LOG_TAG_NAME='"blhost"'
LOCAL_CFLAGS += -std=c99 -D LINUX -D BOOTLOADER_HOST -D _GNU_SOURCE
LOCAL_CPPFLAGS += -D LINUX -D BOOTLOADER_HOST -std=c++11 -fexceptions -Wno-non-virtual-dtor -Woverloaded-virtual -frtti

include $(BUILD_STATIC_LIBRARY)




