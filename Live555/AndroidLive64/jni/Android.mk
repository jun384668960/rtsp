LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_LDLIBS := -L$(NDKINC)/lib -llog

LOCAL_MODULE := liblive_static

LOCAL_SRC_FILES := \
	./liblive/BasicUsageEnvironment/src/BasicHashTable.cpp \
	./liblive/BasicUsageEnvironment/src/BasicTaskScheduler.cpp \
	./liblive/BasicUsageEnvironment/src/BasicTaskScheduler0.cpp \
	./liblive/BasicUsageEnvironment/src/BasicUsageEnvironment.cpp \
	./liblive/BasicUsageEnvironment/src/BasicUsageEnvironment0.cpp \
	./liblive/BasicUsageEnvironment/src/DelayQueue.cpp \
	./liblive/UsageEnvironment/src/HashTable.cpp \
	./liblive/UsageEnvironment/src/strDup.cpp \
	./liblive/UsageEnvironment/src/UsageEnvironment.cpp \
	./liblive/groupsock/src/GroupEId.cpp \
	./liblive/groupsock/src/Groupsock.cpp \
	./liblive/groupsock/src/GroupsockHelper.cpp \
	./liblive/groupsock/src/inet.c \
	./liblive/groupsock/src/IOHandlers.cpp \
	./liblive/groupsock/src/NetAddress.cpp \
	./liblive/groupsock/src/NetInterface.cpp \
	./liblive/liveMedia/src/AC3AudioFileServerMediaSubsession.cpp \
	./liblive/liveMedia/src/AC3AudioRTPSink.cpp \
	./liblive/liveMedia/src/AC3AudioRTPSource.cpp \
	./liblive/liveMedia/src/AC3AudioStreamFramer.cpp \
	./liblive/liveMedia/src/ADTSAudioFileServerMediaSubsession.cpp \
	./liblive/liveMedia/src/ADTSAudioFileSource.cpp \
	./liblive/liveMedia/src/AMRAudioFileServerMediaSubsession.cpp \
	./liblive/liveMedia/src/AMRAudioFileSink.cpp \
	./liblive/liveMedia/src/AMRAudioFileSource.cpp \
	./liblive/liveMedia/src/AMRAudioRTPSink.cpp \
	./liblive/liveMedia/src/AMRAudioRTPSource.cpp \
	./liblive/liveMedia/src/AMRAudioSource.cpp \
	./liblive/liveMedia/src/AudioInputDevice.cpp \
	./liblive/liveMedia/src/AudioRTPSink.cpp \
	./liblive/liveMedia/src/AVIFileSink.cpp \
	./liblive/liveMedia/src/Base64.cpp \
	./liblive/liveMedia/src/BasicUDPSink.cpp \
	./liblive/liveMedia/src/BasicUDPSource.cpp \
	./liblive/liveMedia/src/BitVector.cpp \
	./liblive/liveMedia/src/ByteStreamFileSource.cpp \
	./liblive/liveMedia/src/ByteStreamMemoryBufferSource.cpp \
	./liblive/liveMedia/src/ByteStreamMultiFileSource.cpp \
	./liblive/liveMedia/src/DeviceSource.cpp \
	./liblive/liveMedia/src/DigestAuthentication.cpp \
	./liblive/liveMedia/src/DVVideoFileServerMediaSubsession.cpp \
	./liblive/liveMedia/src/DVVideoRTPSink.cpp \
	./liblive/liveMedia/src/DVVideoRTPSource.cpp \
	./liblive/liveMedia/src/DVVideoStreamFramer.cpp \
	./liblive/liveMedia/src/EBMLNumber.cpp \
	./liblive/liveMedia/src/FileServerMediaSubsession.cpp \
	./liblive/liveMedia/src/FileSink.cpp \
	./liblive/liveMedia/src/FramedFileSource.cpp \
	./liblive/liveMedia/src/FramedFilter.cpp \
	./liblive/liveMedia/src/FramedSource.cpp \
	./liblive/liveMedia/src/GenericMediaServer.cpp \
	./liblive/liveMedia/src/GSMAudioRTPSink.cpp \
	./liblive/liveMedia/src/H261VideoRTPSource.cpp \
	./liblive/liveMedia/src/H263plusVideoFileServerMediaSubsession.cpp \
	./liblive/liveMedia/src/H263plusVideoRTPSink.cpp \
	./liblive/liveMedia/src/H263plusVideoRTPSource.cpp \
	./liblive/liveMedia/src/H263plusVideoStreamFramer.cpp \
	./liblive/liveMedia/src/H263plusVideoStreamParser.cpp \
	./liblive/liveMedia/src/H264or5VideoFileSink.cpp \
	./liblive/liveMedia/src/H264or5VideoRTPSink.cpp \
	./liblive/liveMedia/src/H264or5VideoStreamDiscreteFramer.cpp \
	./liblive/liveMedia/src/H264or5VideoStreamFramer.cpp \
	./liblive/liveMedia/src/H264VideoFileServerMediaSubsession.cpp \
	./liblive/liveMedia/src/H264VideoFileSink.cpp \
	./liblive/liveMedia/src/H264VideoRTPSink.cpp \
	./liblive/liveMedia/src/H264VideoRTPSource.cpp \
	./liblive/liveMedia/src/H264VideoStreamDiscreteFramer.cpp \
	./liblive/liveMedia/src/H264VideoStreamFramer.cpp \
	./liblive/liveMedia/src/H265VideoFileServerMediaSubsession.cpp \
	./liblive/liveMedia/src/H265VideoFileSink.cpp \
	./liblive/liveMedia/src/H265VideoRTPSink.cpp \
	./liblive/liveMedia/src/H265VideoRTPSource.cpp \
	./liblive/liveMedia/src/H265VideoStreamDiscreteFramer.cpp \
	./liblive/liveMedia/src/H265VideoStreamFramer.cpp \
	./liblive/liveMedia/src/InputFile.cpp \
	./liblive/liveMedia/src/JPEGVideoRTPSink.cpp \
	./liblive/liveMedia/src/JPEGVideoRTPSource.cpp \
	./liblive/liveMedia/src/JPEGVideoSource.cpp \
	./liblive/liveMedia/src/Locale.cpp \
	./liblive/liveMedia/src/MatroskaDemuxedTrack.cpp \
	./liblive/liveMedia/src/MatroskaFile.cpp \
	./liblive/liveMedia/src/MatroskaFileParser.cpp \
	./liblive/liveMedia/src/MatroskaFileServerDemux.cpp \
	./liblive/liveMedia/src/MatroskaFileServerMediaSubsession.cpp \
	./liblive/liveMedia/src/Media.cpp \
	./liblive/liveMedia/src/MediaSession.cpp \
	./liblive/liveMedia/src/MediaSink.cpp \
	./liblive/liveMedia/src/MediaSource.cpp \
	./liblive/liveMedia/src/MP3ADU.cpp \
	./liblive/liveMedia/src/MP3ADUdescriptor.cpp \
	./liblive/liveMedia/src/MP3ADUinterleaving.cpp \
	./liblive/liveMedia/src/MP3ADURTPSink.cpp \
	./liblive/liveMedia/src/MP3ADURTPSource.cpp \
	./liblive/liveMedia/src/MP3ADUTranscoder.cpp \
	./liblive/liveMedia/src/MP3AudioFileServerMediaSubsession.cpp \
	./liblive/liveMedia/src/MP3AudioMatroskaFileServerMediaSubsession.cpp \
	./liblive/liveMedia/src/MP3FileSource.cpp \
	./liblive/liveMedia/src/MP3Internals.cpp \
	./liblive/liveMedia/src/MP3InternalsHuffman.cpp \
	./liblive/liveMedia/src/MP3InternalsHuffmanTable.cpp \
	./liblive/liveMedia/src/MP3StreamState.cpp \
	./liblive/liveMedia/src/MP3Transcoder.cpp \
	./liblive/liveMedia/src/MPEG1or2AudioRTPSink.cpp \
	./liblive/liveMedia/src/MPEG1or2AudioRTPSource.cpp \
	./liblive/liveMedia/src/MPEG1or2AudioStreamFramer.cpp \
	./liblive/liveMedia/src/MPEG1or2Demux.cpp \
	./liblive/liveMedia/src/MPEG1or2DemuxedElementaryStream.cpp \
	./liblive/liveMedia/src/MPEG1or2DemuxedServerMediaSubsession.cpp \
	./liblive/liveMedia/src/MPEG1or2FileServerDemux.cpp \
	./liblive/liveMedia/src/MPEG1or2VideoFileServerMediaSubsession.cpp \
	./liblive/liveMedia/src/MPEG1or2VideoRTPSink.cpp \
	./liblive/liveMedia/src/MPEG1or2VideoRTPSource.cpp \
	./liblive/liveMedia/src/MPEG1or2VideoStreamDiscreteFramer.cpp \
	./liblive/liveMedia/src/MPEG1or2VideoStreamFramer.cpp \
	./liblive/liveMedia/src/MPEG2IndexFromTransportStream.cpp \
	./liblive/liveMedia/src/MPEG2TransportFileServerMediaSubsession.cpp \
	./liblive/liveMedia/src/MPEG2TransportStreamFramer.cpp \
	./liblive/liveMedia/src/MPEG2TransportStreamFromESSource.cpp \
	./liblive/liveMedia/src/MPEG2TransportStreamFromPESSource.cpp \
	./liblive/liveMedia/src/MPEG2TransportStreamIndexFile.cpp \
	./liblive/liveMedia/src/MPEG2TransportStreamMultiplexor.cpp \
	./liblive/liveMedia/src/MPEG2TransportStreamTrickModeFilter.cpp \
	./liblive/liveMedia/src/MPEG2TransportUDPServerMediaSubsession.cpp \
	./liblive/liveMedia/src/MPEG4ESVideoRTPSink.cpp \
	./liblive/liveMedia/src/MPEG4ESVideoRTPSource.cpp \
	./liblive/liveMedia/src/MPEG4GenericRTPSink.cpp \
	./liblive/liveMedia/src/MPEG4GenericRTPSource.cpp \
	./liblive/liveMedia/src/MPEG4LATMAudioRTPSink.cpp \
	./liblive/liveMedia/src/MPEG4LATMAudioRTPSource.cpp \
	./liblive/liveMedia/src/MPEG4VideoFileServerMediaSubsession.cpp \
	./liblive/liveMedia/src/MPEG4VideoStreamDiscreteFramer.cpp \
	./liblive/liveMedia/src/MPEG4VideoStreamFramer.cpp \
	./liblive/liveMedia/src/MPEGVideoStreamFramer.cpp \
	./liblive/liveMedia/src/MPEGVideoStreamParser.cpp \
	./liblive/liveMedia/src/MultiFramedRTPSink.cpp \
	./liblive/liveMedia/src/MultiFramedRTPSource.cpp \
	./liblive/liveMedia/src/OggDemuxedTrack.cpp \
	./liblive/liveMedia/src/OggFile.cpp \
	./liblive/liveMedia/src/OggFileParser.cpp \
	./liblive/liveMedia/src/OggFileServerDemux.cpp \
	./liblive/liveMedia/src/OggFileServerMediaSubsession.cpp \
	./liblive/liveMedia/src/OggFileSink.cpp \
	./liblive/liveMedia/src/OnDemandServerMediaSubsession.cpp \
	./liblive/liveMedia/src/ourMD5.cpp \
	./liblive/liveMedia/src/OutputFile.cpp \
	./liblive/liveMedia/src/PassiveServerMediaSubsession.cpp \
	./liblive/liveMedia/src/ProxyServerMediaSession.cpp \
	./liblive/liveMedia/src/QCELPAudioRTPSource.cpp \
	./liblive/liveMedia/src/QuickTimeFileSink.cpp \
	./liblive/liveMedia/src/QuickTimeGenericRTPSource.cpp \
	./liblive/liveMedia/src/RTCP.cpp \
	./liblive/liveMedia/src/rtcp_from_spec.c \
	./liblive/liveMedia/src/RTPInterface.cpp \
	./liblive/liveMedia/src/RTPSink.cpp \
	./liblive/liveMedia/src/RTPSource.cpp \
	./liblive/liveMedia/src/RTSPClient.cpp \
	./liblive/liveMedia/src/RTSPCommon.cpp \
	./liblive/liveMedia/src/RTSPRegisterSender.cpp \
	./liblive/liveMedia/src/RTSPServer.cpp \
	./liblive/liveMedia/src/RTSPServerSupportingHTTPStreaming.cpp \
	./liblive/liveMedia/src/ServerMediaSession.cpp \
	./liblive/liveMedia/src/SimpleRTPSink.cpp \
	./liblive/liveMedia/src/SimpleRTPSource.cpp \
	./liblive/liveMedia/src/SIPClient.cpp \
	./liblive/liveMedia/src/StreamParser.cpp \
	./liblive/liveMedia/src/StreamReplicator.cpp \
	./liblive/liveMedia/src/T140TextRTPSink.cpp \
	./liblive/liveMedia/src/TCPStreamSink.cpp \
	./liblive/liveMedia/src/TextRTPSink.cpp \
	./liblive/liveMedia/src/TheoraVideoRTPSink.cpp \
	./liblive/liveMedia/src/TheoraVideoRTPSource.cpp \
	./liblive/liveMedia/src/uLawAudioFilter.cpp \
	./liblive/liveMedia/src/VideoRTPSink.cpp \
	./liblive/liveMedia/src/VorbisAudioRTPSink.cpp \
	./liblive/liveMedia/src/VorbisAudioRTPSource.cpp \
	./liblive/liveMedia/src/VP8VideoRTPSink.cpp \
	./liblive/liveMedia/src/VP8VideoRTPSource.cpp \
	./liblive/liveMedia/src/VP9VideoRTPSink.cpp \
	./liblive/liveMedia/src/VP9VideoRTPSource.cpp \
	./liblive/liveMedia/src/WAVAudioFileServerMediaSubsession.cpp \
	./liblive/liveMedia/src/WAVAudioFileSource.cpp \

$(warning =================building src file detail======================)
$(warning $(NDKINC))
#$(warning $(LOCAL_SRC_FILES))
$(warning ===============================================================)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/liblive/UsageEnvironment/include \
	$(LOCAL_PATH)/liblive/BasicUsageEnvironment/include \
	$(LOCAL_PATH)/liblive/groupsock/include \
	$(LOCAL_PATH)/liblive/liveMedia/include \


#LOCAL_CPPFLAGS += -fexceptions
LOCAL_CPPFLAGS += -fexceptions -DXLOCALE_NOT_USED=1 -DNULL=0 -DNO_SSTREAM=1 -UIP_ADD_SOURCE_MEMBERSHIP  
#LOCAL_CPPFLAGS +=  -DLIVE_LOG

#LOCAL_CPPFLAGS += -D__ARM_ARCH_7__
#LOCAL_CPPFLAGS += -D__ARM_ARCH_7A__ 

#LOCAL_CFLAGS += -D__ARM_ARCH_7__
#LOCAL_CFLAGS += -D__ARM_ARCH_7A__ 

include $(BUILD_STATIC_LIBRARY)


# Build shared library
#include $(CLEAR_VARS)

#LOCAL_LDLIBS := -L$(NDKINC)/lib -llog

#LOCAL_MODULE := liblive

#LOCAL_MODULE_TAGS := optional

#LOCAL_WHOLE_STATIC_LIBRARIES = liblive_static

#include $(BUILD_SHARED_LIBRARY)
