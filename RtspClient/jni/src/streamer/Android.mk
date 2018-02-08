LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := ffmpeg_avformat 
LOCAL_SRC_FILES := ../../lib/libavformat.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := ffmpeg_avcodec
LOCAL_SRC_FILES := ../../lib/libavcodec.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := ffmpeg_avutil
LOCAL_SRC_FILES := ../../lib/libavutil.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := ffmpeg_swscale
LOCAL_SRC_FILES := ../../lib/libswscale.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libfaac
LOCAL_SRC_FILES := ../../lib/libfaac.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libx264
LOCAL_SRC_FILES := ../../lib/libx264.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := librtsp
LOCAL_SRC_FILES := ../../lib/$(TARGET_ARCH_ABI)/librtsp.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := librtmp
LOCAL_SRC_FILES := ../../lib/$(TARGET_ARCH_ABI)/librtmp.a
include $(PREBUILT_STATIC_LIBRARY)

#################################################################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := streamuser

#VisualGDBAndroid: AutoUpdateSourcesInNextLine
LOCAL_SRC_FILES := buffer/Buffer.cpp buffer/BufferManager.cpp render/AudioTrack.cpp codec/AudioDecoder.cpp codec/VideoDecoder.cpp codec/VideoEncoder.cpp codec/AudioEncoder.cpp codec/HwAacDecoder.cpp codec/HwAacEncoder.cpp codec/HwCodec.cpp codec/Hw264Decoder.cpp codec/Hw264Encoder.cpp codec/MediaDecoder.cpp codec/MediaEncoder.cpp common.cpp logimp.cpp com_bql_MediaPlayer_Native.cpp com_bql_MSG_Native.cpp JNI_load.cpp Streamer.cpp player/Player.cpp player/local/FfmpegPlayer.cpp player/rtmp/RtmpClientPlayer.cpp player/rtsp/RtspClientPlayer.cpp player/rtsp/RtspMediaSink.cpp record/Mp4file.cpp record/Snapshot.cpp record/MediaRecorder.cpp record/FileRecordThread.cpp record/RtmpRecordThread.cpp thread/AudioDecodeThread.cpp thread/VideoDecodeThread.cpp SwsScale.cpp

LOCAL_C_INCLUDES += \
$(LOCAL_PATH)/../../3rd/librtsp/UsageEnvironment/include \
	$(LOCAL_PATH)/../../3rd/librtsp/BasicUsageEnvironment/include \
	$(LOCAL_PATH)/../../3rd/librtsp/groupsock/include \
	$(LOCAL_PATH)/../../3rd/librtsp/liveMedia/include \
	$(LOCAL_PATH)/../../3rd/librtmp \
	$(LOCAL_PATH)/../../3rd/libffmpeg \
	$(LOCAL_PATH)/../../3rd/libhwcodec \
	$(LOCAL_PATH) \


LOCAL_STATIC_LIBRARIES := ffmpeg_avformat ffmpeg_avcodec ffmpeg_swscale ffmpeg_avutil libx264 libfaac librtsp librtmp
LOCAL_SHARED_LIBRARIES := 
LOCAL_LDLIBS := -L$(NDKINC)/lib -llog -ljnigraphics -landroid

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -D_DEBUG -D_RELEASE -D_LOGFILE

LOCAL_CPPFLAGS += -fpermissive -fexceptions
include $(BUILD_SHARED_LIBRARY)
