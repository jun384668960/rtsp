#ifndef _GSSLIVECONN_H__
#define _GSSLIVECONN_H__

#include "MyClock.h"
#include "MySemaphore.h"
#include "MySqlPool.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <map>
#include <string>

#define MAX_AV_VIDEO_SIZE	60
#define MAX_AV_AUDIO_SIZE	100

#pragma   pack(1)
typedef struct GP2pHead_t
{
	int  			flag;		//消息开始标识
	unsigned int 	size;		//接收发送消息大小(不包括消息头)
	char 			type;		//协议类型1 json 2 json 加密
	char			protoType;	//消息类型1 请求2应答3通知
	int 			msgType;	//IOTYPE消息类型
	char 			reserve[6];	//保留
}GP2pHead;
#pragma   pack()

enum{
	SND_VIDEO_LD		= 0x00F0,
	SND_VIDEO_HD		= 0x00F1,
	SND_VIDEO			= 0x00F2,
	SND_AUDIO			= 0x00F3,
	SND_NOTIC	    	= 0x00F4,
	SND_FILE	    	= 0x00F5,
};

typedef enum _gos_frame_type
{
	gos_unknown_frame = 0,				// 未知帧
	gos_video_i_frame = 1,				// I 帧
	gos_video_p_frame = 2,				// P 帧
	gos_video_b_frame = 3,				// B 帧
	gos_video_cut_i_frame = 4,			//剪接录像I帧
	gos_video_cut_p_frame = 5,			//剪接录像P帧
	gos_video_cut_b_frame = 6,			//剪接录像B帧
	gos_video_cut_end_frame = 7,		//剪接录像B帧
	gos_video_preview_i_frame = 10,		//预览图
	gos_video_end_frame,

	gos_audio_frame   = 50,			// 音频帧
	gos_cut_audio_frame   = 51, 		// 音频帧

	gos_special_frame = 100,		// 特殊帧	 gos_special_data 比如门灯灯状态主动上传app
} gos_frame_type_t;

// typedef enum _gos_codec_type
// {
// 	gos_codec_unknown = 0,
// 
// 	gos_video_codec_start = 10,
// 	gos_video_H264,
// 	gos_video_H265,
// 	gos_video_MPEG4,
// 	gos_video_MJPEG,
// 	gos_video_codec_end,
// 
// 	gos_audio_codec_start = 50,
// 	gos_audio_AAC,
// 	gos_audio_G711A,
// 	gos_audio_G711U,
// 	gos_audio_codec_end,
// 
// } gos_codec_type_t;
typedef enum _gos_codec_type
{
	gos_codec_unknown = 0,

	gos_video_codec_start = 10,
	gos_video_H264_AAC,
	gos_video_H264_G711,
	gos_video_H265,
	gos_video_MPEG4,
	gos_video_MJPEG,
	gos_video_codec_end,

	gos_audio_codec_start = 50,
	gos_audio_AAC,
	gos_audio_G711A,
	gos_audio_G711U,
	gos_audio_codec_end,

} gos_codec_type_t;
typedef struct _ggos_frame_head
{
	unsigned int	nFrameNo;			// 帧号
	unsigned int	nFrameType;			// 帧类型	gos_frame_type_t
	unsigned int	nCodeType;			// 编码类型 gos_codec_type_t
	unsigned int	nFrameRate;			// 视频帧率，音频采样率
	unsigned int	nTimestamp;			// 时间戳
	unsigned short	sWidth;				// 视频宽
	unsigned short	sHeight;			// 视频高
	unsigned int	reserved;			// 预留
	unsigned int	nDataSize;			// data数据长度
}GosFrameHead;

class GssBuffer {
#define MAX_BUFSIZE 256*1024
#define MAX_BUFSIZEAUDIO 1024
public:
	GssBuffer(bool bVideo = true) {
		if(bVideo)
			m_maxSize = MAX_BUFSIZE;
		else
			m_maxSize = MAX_BUFSIZEAUDIO;
		m_buffer = (unsigned char*)malloc(m_maxSize);
		m_bufferlen = 0;
		
	}
	~GssBuffer() {
		free(m_buffer);
		m_buffer = NULL;
	}
	bool SetBuffer(unsigned char* pData, int datalen)
	{
		if (pData && datalen > 0)
		{
			if (datalen > m_maxSize)
			{
				free(m_buffer);
				m_buffer = NULL;
				m_maxSize = datalen;
				m_buffer = (unsigned char*)malloc(m_maxSize);
			}
			if(m_buffer)
			{
				memcpy(&m_frameHeader,pData,sizeof(m_frameHeader));
				//网络字节序，转换?
				//datalen - sizeof(m_frameHeader ?= m_frameHeader.nDataSize
				memcpy(m_buffer,pData+sizeof(m_frameHeader),datalen - sizeof(m_frameHeader));
				m_bufferlen = datalen - sizeof(m_frameHeader);
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
public:
	unsigned char* m_buffer;
	int m_bufferlen;
	int m_maxSize;
	GosFrameHead m_frameHeader;
};

typedef enum {
	e_gss_conn_type_rtsp = 0,
	e_gss_conn_type_hls,
	e_gss_conn_type_count,
}EGSSCONNTYPE;

typedef struct _rtspplaytime {
	unsigned int time;
	bool bDel;
}RtspPlayTime;

typedef struct _globalInfo {
	char domainDispath[256];
	unsigned short port;
	char logs[1024];
	int maxPlayTime;
	MyClock lock;
	std::map<std::string,RtspPlayTime> mapTimes;
	EGSSCONNTYPE type;
}SGlobalInfo;

class GssLiveConn { //调用此类进行使用之前，先调用GlobalInit进行全局操作的初始化
public:
	static bool GlobalInit(	const char* pserver, const char* plogpath, int loglvl, //ex: pserver = "120.23.23.33:6001" 或者 "cnp2p.ulifecam.com:6001" ;plogpath = "/var/log/live555"
										const char* sqlHost, int sqlPort, //数据的HOST,PORT
										const char* sqlUser, const char* sqlPasswd, const char* dbName, //数据库登录用户名和密码,数据库名称，
										int maxCounts, //连接池中数据库连接的最大数,假设有n个业务线程使用该连接池，建议:maxCounts=n,假设n>20, 建议maxCounts=20
										int maxPlayTime, //最大播放时长(单位分钟)
										EGSSCONNTYPE type = e_gss_conn_type_rtsp); //type,
	static void GlobalUnInit();
	static bool SetForceLiveSec(int sec);
	
	GssLiveConn();
	GssLiveConn(const char* server, unsigned short port, const char* uid, bool bDispath = true);
	virtual ~GssLiveConn();

	bool Init(char* server, unsigned short port, char* uid, bool bDispath = false);
	bool Start(bool bRestart = false);
	bool Stop();
	bool IsConnected();

	unsigned int referenceCount() const { return m_referenceCount; }
	void incrementReferenceCount() { ++m_referenceCount; }
	void decrementReferenceCount() { if (m_referenceCount > 0) --m_referenceCount; }

	bool IsKnownAudioType();
	bool IsAudioAacType();
	bool IsAudioG711AType();
	bool GetVideoFrame(unsigned char** pData, int &datalen, GosFrameHead& frameHeader); //成功后，当使用完pData之后必须调用FreeVideoFrame
	void FreeVideoFrame();
	bool GetAudioFrame(unsigned char** pData, int &datalen, GosFrameHead& frameHeader);
	void FreeAudioFrame();
	bool Send(unsigned char* pData, int datalen); //备用
	
	void VideoFrameSync();
	int  VideoFrameCount();
	void AudioFrameSync();
	int  AudioFrameCount();
		
protected:
	bool RequestLiveConnServer();

	static void OnRecv(void *transport, void *user_data, char* data, int len, char type, unsigned int time_stamp);
	static void OnDisconnect(void *transport, void* user_data, int status);
	static void OnConnectResult(void *transport, void* user_data, int status);
	static void OnDeviceDisconnect(void *transport, void *user_data);
	static void OnDsCallback(void* dispatcher, int status, void* user_data, char* server, unsigned short port, unsigned int server_id);

	void InitVideoBuffer();
	void ClearVideoBuffer();
	void DropAllVideoBuffer();

	void InitAudioBuffer();
	void ClearAudioBuffer();

	bool AddFrame(unsigned char* pData, int datalen);
	bool AddVideoFrame(unsigned char* pData, int datalen);
	bool AddAudioFrame(unsigned char* pData, int datalen);
	void IncVideoIndex();
	void IncVideoNextInsertIndex();
	void IncAudioIndex();
	void IncAudioNextInsertIndex();

	bool IsReachedMaxPlayTimeofDay(int theMaxTimeMin, const char* guid, int & leftTimeSec);
	bool AddNewPlayTime(const char* guid);
	bool DelPlayTime(const char* guid);
public:
	static bool UpdatePlayTime(int onceTime, const char* guid);
	static bool ResetPlayTime(const char* guid, int nCol, int nColValue);
	static int GetColByType();

private:
	MyClock m_lockVideo;
	GssBuffer* m_bufferVideo[MAX_AV_VIDEO_SIZE];
	int m_currNextIndexVideoInsert;
	int m_countsVideo;
	int m_curVideoIndex;

	MyClock m_lockAudio;
	GssBuffer* m_bufferAudio[MAX_AV_AUDIO_SIZE];
	int m_currNextIndexAudioInsert;
	int m_countsAudio;
	int m_curAudioIndex;

	char m_server[256];
	unsigned short m_port;
	char m_uid[128];
	bool m_isConnected;
	int m_stusRlt;

	void* m_clientPullConn;
	gos_codec_type_t m_audioType;
	unsigned int m_referenceCount;
	bool m_bDroped;

	char m_dispathServer[256];
	unsigned short m_dispathPort;
	
	MySem m_semt;
	bool m_isFirstFrame;
	bool				m_forcePause;
	unsigned int		m_liveRef;
	int m_forceLiveSecLeftTime;

public:
	static bool m_isInitedGlobal;
	static SGlobalInfo 	m_sGlobalInfos;
	static int			m_forceLiveSec;
	static MySqlPool* m_smysqlpool;
	static pthread_t m_spthread;
};

#endif //_GSSLIVECONN_H__
