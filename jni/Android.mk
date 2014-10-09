LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# Tegra optimized OpenCV.mk
#include ../../OpenCV-2.4.5-Tegra-sdk-r2/sdk/native/jni/OpenCV-tegra3.mk
include ../../OpenCV-2.4.9-android-sdk/sdk/native/jni/OpenCV.mk 

# Linker
LOCAL_LDLIBS += -llog

# Our module sources
LOCAL_MODULE    := PanoHDR
LOCAL_SRC_FILES := PanoHDR.cpp Panorama.cpp HDR.cpp NativeLogging.cpp
LOCAL_LDLIBS +=  -llog -ldl

include $(BUILD_SHARED_LIBRARY)


