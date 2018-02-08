#ifndef __INCLUDE_RTSP_SESSION_H__
#define __INCLUDE_RTSP_SESSION_H__

#include "Def.h"
#include "Thread.h"
#include "TcpSock.h"
#include "Mutex.h"
#include <time.h>
#include <string>
#include "DataSrc.h"

class CRtspSession : CThread
{
public:
	CRtspSession();
	~CRtspSession();
public:
	int Start( int fd, NotifyFun fun, long user_info );
private:
	virtual void thread_proc( long user_info );
	int recv_data();
	int handle_data();
	int parse_data( const char* data, int len );
	int handle_cmd( const char* data, int len );
private:
	NotifyFun m_fun;
	long m_user_info;
	CTcpSock m_sock;
	enum{
		MAX_RECV_BUF_LEN = 1024*4,
	};
	char m_recv_buf[MAX_RECV_BUF_LEN];
	uint32_t m_recv_len;
	CMutex m_mutex;
	CDataSrc m_data_src;
private:
	RtspMethodT parse_method( const char* data, int len );
	void parser_common( const char* data, int len );
	int parser_url( const char* data, int len );
	int get_str( const char* data, int len, const char* s_mark, const char* e_mark, char* dest, int dest_len );
	int handle_describe( const char* data, int len );
	int handle_setup( const char* data, int len );
	int handle_play( const char* data, int len );
	int handle_pause();
	int handle_teardowm();
	int handle_other_method();
	int send_simple_cmd( int code ); 
	int send_cmd( const char* data, int len );
private:
	static void notify_fun( long id, long msg, long user_info );
private:
	char m_cseq[128];
	int m_rtp_ch; 
	char m_session[128];
	char m_url[256];
};

#endif
