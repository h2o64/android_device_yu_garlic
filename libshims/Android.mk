LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := camera.cpp

LOCAL_SHARED_LIBRARIES := \
  libgui \
  libui

LOCAL_C_INCLUDES := \
    frameworks/native/include

LOCAL_MODULE := libshim_libmmcamera
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
