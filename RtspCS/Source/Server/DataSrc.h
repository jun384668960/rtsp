#ifndef __INCLUDE_DATA_SRC_H__
#define __INCLUDE_DATA_SRC_H__

#include "Def.h"
#include "Thread.h"
#include "Mutex.h"
#include "TsFileReader.h"
#include "Sock.h"

class CDataSrc : public CThread
{
public:
	CDataSrc();
	~CDataSrc();
public:
	int Init( const char* content, int id, NotifyFun fun, long user_info );
	const char* GetSdp();
	int GetRange();
	int GetMediaNum();
	struct MediaInfo{
		char track_id[32];
		uint16_t seq;
		uint32_t rtp_time;
		uint32_t ssrc;
	};
	int GetMediaInfo( int media_index, MediaInfo& media_info );
	int PerPlay( int s_sec, int e_sec );
	//rtp_ch < 0 RTP OVER UDP; rtp_ch >= 0 RTP OVER RTSP
	int Play( CSock* sock, int rtp_ch );
	int Pause();
private:
	virtual void thread_proc( long user_info );
private:
	char m_sdp[2048];
	int m_range;
	int m_media_num;
	enum{
		MAX_MEDIA_NUM = 1,
	};
	MediaInfo m_media_info[MAX_MEDIA_NUM];
	int m_s_sec;
	int m_e_sec;
	CMutex m_mutex;
	CTsFileReader m_reader;
	CSock* m_sock;
	int m_rtp_ch;
	uint64_t m_start_pcr;
	uint64_t m_start_ms;
	uint64_t m_last_pcr;
	NotifyFun m_fun;
	long m_user_info;
};

#endif
