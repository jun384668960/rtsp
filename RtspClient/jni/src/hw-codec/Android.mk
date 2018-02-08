LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := hw-codec-jni
LOCAL_SRC_FILES := logimp.cpp HwAacEncoder.cpp HwAacDecoder.cpp Hw264Encoder.cpp Hw264Decoder.cpp HwInt.cpp

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../3rd/libhwcodec \
	$(LOCAL_PATH) \

# for native multimedia
LOCAL_LDLIBS    += -lOpenMAXAL -lmediandk

# for logging
LOCAL_LDLIBS    += -llog

# for native windows
LOCAL_LDLIBS    += -landroid

LOCAL_CFLAGS 	+= -DGL_GLEXT_PROTOTYPES -D_DEBUG -D_RELEASE -D_LOGFILE
include $(BUILD_SHARED_LIBRARY)