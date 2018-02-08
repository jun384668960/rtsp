#include "RtspSession.h"
#include "PrintLog.h"
#include <string.h>
#include "NTime.h"

CRtspSession::CRtspSession()
{
	m_fun = NULL;
	m_user_info = 0;
	memset( m_recv_buf, 0, sizeof(m_recv_buf) );
	m_recv_len = 0;
	//////////////////
	memset( m_cseq, 0, sizeof(m_cseq) );
	m_rtp_ch = 0;
	memset( m_session, 0, sizeof(m_session) );
	memset( m_url, 0, sizeof(m_url) );
}

CRtspSession::~CRtspSession()
{
	Destroy();
	WaitExit();
}

int CRtspSession::Start( int fd, NotifyFun fun, long user_info )
{
	CGuard lock( m_mutex );
	m_fun = fun;
	m_user_info = user_info;
	m_sock.AttachFd( fd );
	if( Create( "RtspSessionThread", 0 ) < 0 ){
		LogError( "create thread  RtspSessionThread failed\n" );
		return -1;
	}
	return 0;
}

void CRtspSession::thread_proc( long user_info )
{
	int ret = 0;
	while( IsDestroyed() == false ){
		CGuard lock( m_mutex );
		fd_set read_fd_set;
		fd_set except_fd_set;
		FD_ZERO( &read_fd_set );	//WRITE_EVENT
		FD_ZERO( &except_fd_set );	//EXCEPT_EVENT
		FD_SET( m_sock.GetFd(), &read_fd_set );
		FD_SET( m_sock.GetFd(), &except_fd_set );
#ifdef _WIN32
		struct timeval tmv_timeout={ 0L, 1000000L };//单位微秒，默认1秒超时
#else
		struct timeval tmv_timeout={ 0L, 1000L };//单位毫秒，默认1秒超时
#endif
		if( select( m_sock.GetFd()+1, &read_fd_set, NULL, &except_fd_set, &tmv_timeout ) > 0  ){
			if( FD_ISSET( m_sock.GetFd(), &read_fd_set ) != 0 ){
				if( recv_data() < 0 ){
					ret = -1;
					break;
				}
			}
			if( FD_ISSET( m_sock.GetFd(), &except_fd_set ) != 0 ){
				LogError( "session net exception\n" );
				ret = -1;
				break;
			}
		}
	}
	if( m_fun != NULL )
		m_fun( m_sock.GetFd(), RTSP_SESSION_CLOSE, m_user_info );
}

int CRtspSession::recv_data()
{
	int recv_len = sizeof(m_recv_buf)-1-m_recv_len;
	if( recv_len <= 0 ){
		LogError( "recv buf len <=0\n" );
		return -1;
	}
	int ret = m_sock.Recv( m_recv_buf+m_recv_len, recv_len );
	if( ret < 0 ){
		LogError( "recv data failed\n" );
		return -1;
	}
	m_recv_len += ret;
	return handle_data();
}

int CRtspSession::handle_data()
{
	char *recv_buf = m_recv_buf;
	m_recv_buf[m_recv_len] = '\0';
	int ret = -1;
	while( m_recv_len > 0 ){
		ret = 0;
		if( '$' == *recv_buf ){
			if( m_recv_len <= sizeof(struct RtpTcpHdr) )
				break;
			struct RtpTcpHdr* r_t_hd = (struct RtpTcpHdr *)recv_buf;
			uint32_t r_t_len = ntohs(r_t_hd->len);
			if( m_recv_len < r_t_len + 4 )
				break;
			;//handle rtcp
			m_recv_len -= r_t_len + 4;
			recv_buf += r_t_len + 4;
		}else{
			int parser_ret = parse_data( recv_buf, m_recv_len );
			if( parser_ret < 0 )
				break;
			PRINT_RECV_CMD( recv_buf, parser_ret );
			if( handle_cmd( recv_buf, parser_ret ) < 0 )
				return -1;
			m_recv_len -= parser_ret;
			recv_buf += parser_ret;
		}
	}
	if( m_recv_len > 0 )		
		memmove( m_recv_buf , recv_buf, m_recv_len );
	return ret;
}

int CRtspSession::parse_data( const char* data, int len )
{
	const char *start = data;
	const char *end_mark = "\r\n\r\n";
	const char *end = NULL;
	if( NULL == (end = strstr(start, end_mark)) )
		return -1;
	int header_len = end - start + strlen(end_mark);
	int content_len = 0;
	const char* conten_len_mark = "Content-Length ";
	const char* content_len_pos = strstr(end, conten_len_mark);
	if( content_len_pos != NULL && strstr(content_len_pos, "\r\n") != NULL )
		content_len = atoi( content_len_pos+strlen(conten_len_mark) );
	if( len < (header_len + content_len) )
		return -1;
	return (header_len + content_len);
}

int CRtspSession::handle_cmd( const char* data, int len )
{
	RtspMethodT method = parse_method( data, len );
	if( method == RTSP_METHOD_MAX ){
		LogError( "unsupported this method\n" );
		return -1;
	}
	parser_common( data, len );
	switch( method ){
	case RTSP_DESCRIBE:
		return handle_describe( data, len );
	case RTSP_SETUP:
		return handle_setup( data, len );
	case RTSP_PLAY:
		return handle_play( data, len );
	case RTSP_PAUSE:
		return handle_pause();
	case RTSP_TEARDOWN:
		return handle_teardowm();
	default:
		return handle_other_method();
	}
	return 0;
}

////////////////////////////////////////////
RtspMethodT CRtspSession::parse_method( const char* data, int len )
{
	RtspMethodT rtsp_method = RTSP_METHOD_MAX;
	if( *data == 'O' && strncmp( data, g_method[RTSP_OPTIONS].method_str, strlen(g_method[RTSP_OPTIONS].method_str) ) == 0 )
		rtsp_method = RTSP_OPTIONS;
	else if( *data == 'D' && strncmp( data, g_method[RTSP_DESCRIBE].method_str, strlen(g_method[RTSP_DESCRIBE].method_str) ) == 0 )
		rtsp_method = RTSP_DESCRIBE;
	else if( *data == 'S' && strncmp( data, g_method[RTSP_SETUP].method_str, strlen(g_method[RTSP_SETUP].method_str) ) == 0 )
		rtsp_method = RTSP_SETUP;
	else if( *data == 'P' && strncmp( data, g_method[RTSP_PLAY].method_str, strlen(g_method[RTSP_PLAY].method_str) ) == 0 )
		rtsp_method = RTSP_PLAY;
	else if( *data == 'P' && strncmp( data, g_method[RTSP_PAUSE].method_str, strlen(g_method[RTSP_PAUSE].method_str) ) == 0 )
		rtsp_method = RTSP_PAUSE;
	else if( *data == 'T' && strncmp( data, g_method[RTSP_TEARDOWN].method_str, strlen(g_method[RTSP_TEARDOWN].method_str) ) == 0 )
		rtsp_method = RTSP_TEARDOWN;
	else if( *data == 'S' && strncmp( data, g_method[RTSP_SET_PARAMETER].method_str, strlen(g_method[RTSP_SET_PARAMETER].method_str) ) == 0 )
		rtsp_method = RTSP_SET_PARAMETER;
	else if( *data == 'G' && strncmp( data, g_method[RTSP_GET_PARAMETER].method_str, strlen(g_method[RTSP_GET_PARAMETER].method_str) ) == 0 )
		rtsp_method = RTSP_GET_PARAMETER;
	return rtsp_method;
}

void CRtspSession::parser_common( const char* data, int len )
{
	memset( m_cseq, 0, sizeof(m_cseq) );
	if( get_str( data, len, "CSeq:", "\r\n", m_cseq, sizeof(m_cseq)-1-strlen("\r\n") ) >= 0 )
		strncpy( m_cseq+strlen(m_cseq), "\r\n", sizeof(m_cseq)-1 );
}

int CRtspSession::parser_url( const char* data, int len )
{
	memset( m_url, 0, sizeof(m_url) );
	const char* url_s_mark = "rtsp://";
	if( get_str( data, len, url_s_mark, " RTSP", m_url, sizeof(m_url)-1 ) < 0 ){
		LogError( "get url failed\n" );
		return -1;
	}
	return 0;
}

int CRtspSession::get_str( const char* data, int len, const char* s_mark, const char* e_mark, char* dest, int dest_len )
{
	const char* satrt = strstr( data, s_mark );
	if( satrt != NULL ){
		const char* end = strstr( satrt, e_mark );
		if( end != NULL )
			strncpy( dest, satrt, end-satrt>dest_len?dest_len:end-satrt );
		return 0;
	}
	return -1;
}

int CRtspSession::handle_describe( const char* data, int len )
{
	if( parser_url( data, len ) < 0 )
		return send_simple_cmd( 400 );
	if( m_data_src.Init( strstr( strstr(m_url, "rtsp://")+strlen("rtsp://"), "/" )+1, 0, notify_fun, (long)this ) < 0 )
		return send_simple_cmd( 404 );
	const char* sdp = m_data_src.GetSdp();
	char cmd[4096] = "";
	snprintf( cmd, sizeof(cmd), 
		"RTSP/1.0 200 OK\r\n"
		"%s%s"
		"Content-Base: %s/\r\n"
		"Content-Type: application/sdp\r\n"
		"Content-Length: %d\r\n\r\n"
		"%s",
		m_cseq, m_session, m_url, (int)strlen(sdp), sdp );
	return send_cmd( cmd, strlen(cmd) );
}

int CRtspSession::handle_setup( const char* data, int len )
{
	if( strstr( data, "RTP/AVP/TCP" ) == NULL )
		return send_simple_cmd( 461 );
	const char* interleaved = strstr( data, "interleaved=" );
	if( interleaved == NULL )
		return send_simple_cmd( 400 );
	m_rtp_ch = atoi( interleaved+strlen("interleaved=") );
	if( m_rtp_ch < 0 )
		return send_simple_cmd( 400 );
	snprintf( m_session, sizeof(m_session), "Session: %X\r\n", (uint32_t)CNTime::GetCurUs() );
	char cmd[1024] = "";
	snprintf( cmd, sizeof(cmd), 
		"RTSP/1.0 200 OK\r\n"
		"%s%s"
		"Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d\r\n\r\n",
		m_cseq, m_session, m_rtp_ch, m_rtp_ch+1 );
	return send_cmd( cmd, strlen(cmd) );
}

int CRtspSession::handle_play( const char* data, int len )
{
	int range = m_data_src.GetRange();
	int s_sec = 0;
	int e_sec = range;
	const char* range_s = strstr( data, "Range: npt=" );
	if( range_s != NULL ){
		s_sec = atoi( range_s+strlen("Range: npt=") );
		const char* e_sec_pos = strstr( range_s+strlen("Range: npt="), "-" );
		if( e_sec_pos != NULL ){
			int sec = atoi( e_sec_pos+1 );
			if( sec != 0 )
				e_sec = sec;
		}
	}
	m_data_src.PerPlay( s_sec, e_sec );
	CDataSrc::MediaInfo media_info;
	m_data_src.GetMediaInfo( 0, media_info );
	char cmd[1024] = "";
	snprintf( cmd, sizeof(cmd), 
		"RTSP/1.0 200 OK\r\n"
		"%s%s"
		"Range: npt=%d.000-%d.000\r\n"
		"RTP-Info: url=%s/%s;seq=%d;rtptime=%u\r\n\r\n",
		m_cseq, m_session, s_sec, e_sec, m_url, media_info.track_id, media_info.seq, media_info.rtp_time );
	if( send_cmd( cmd, strlen(cmd) ) < 0 )
		return -1;
	return m_data_src.Play( &m_sock, m_rtp_ch );
}

int CRtspSession::handle_pause()
{
	m_data_src.Pause();
	return send_simple_cmd( 200 );
}

int CRtspSession::handle_teardowm()
{
	send_simple_cmd( 200 );
	return -1;
}

int CRtspSession::handle_other_method()
{
	return send_simple_cmd( 200 );
}

int CRtspSession::send_simple_cmd( int code )
{
	int i = 0; 
	for( ; ; i++ ){
		if( g_rsp_code_str[i].code == code )
			break;
		else if( g_rsp_code_str[i].code == 0 ){
			i = 2; //400 Bad Request
			break;
		}
	}
	char cmd[512] = "";
	snprintf( cmd, sizeof(cmd), "RTSP/1.0 %d %s\r\n%s%s\r\n", g_rsp_code_str[i].code, g_rsp_code_str[i].code_str, m_cseq, m_session );
	return send_cmd( cmd, strlen(cmd) );
}

int CRtspSession::send_cmd( const char* data, int len )
{
	PRINT_CMD( data );
	return m_sock.Send( data, len );
}

void CRtspSession::notify_fun( long id, long msg, long user_info )
{
	CRtspSession* obj = (CRtspSession*)user_info;
	if( msg == DATA_SRC_CLOSE && obj->m_fun != NULL )
		obj->m_fun( obj->m_sock.GetFd(), RTSP_SESSION_CLOSE, obj->m_user_info  );
}
