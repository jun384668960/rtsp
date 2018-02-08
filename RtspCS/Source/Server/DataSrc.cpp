#include "DataSrc.h"
#include "PrintLog.h"
#include "NTime.h"

CDataSrc::CDataSrc()
{
	memset( m_sdp, 0, sizeof(m_sdp) );
	m_range = 0;
	m_media_num = MAX_MEDIA_NUM;
	memset( m_media_info, 0, sizeof(m_media_info) );
	m_s_sec = 0;
	m_e_sec = 0;
	m_sock = NULL;
	m_rtp_ch = 0;
	m_start_pcr = -1;
	m_start_ms = -1;
	m_last_pcr = -1;
	m_fun = NULL;
	m_user_info = 0;
}

CDataSrc::~CDataSrc()
{
	Destroy();
	WaitExit();
}

int CDataSrc::Init( const char* content, int id, NotifyFun fun, long user_info )
{
	m_fun = fun;
	m_user_info = user_info;
	if( m_reader.Init( content ) < 0 )
		return -1;
	m_range = m_reader.GetFileRange();
	m_e_sec = m_range;
	strncpy( m_media_info[0].track_id, "track1", sizeof(m_media_info[0].track_id) );
	snprintf( m_sdp, sizeof(m_sdp), 		"v=0\r\n"
		"o=- 0 1 IN IP4 0.0.0.0\r\n"
		"s=RTSP Server\r\n"
		"i=%s\r\n"
		"t=0 0\r\n"
		"a=control:*\r\n"
		"a=range:npt=0-%d\r\n"
		"m=video 0 RTP/AVP 33\r\n"
		"a=control:%s\r\n",
		content, m_range, m_media_info[0].track_id );
	if( Create( "Data Source Thread", 0 ) < 0 ){
		LogError( "data source thread failed\n" );
		return -1;
	}
	return 0;
}

const char* CDataSrc::GetSdp()
{
	return m_sdp;
}

int CDataSrc::GetRange()
{
	return m_range;
}

int CDataSrc::GetMediaNum()
{
	return m_media_num;
}

int CDataSrc::GetMediaInfo( int media_index, MediaInfo& media_info )
{
	if( media_index >= m_media_num )
		return -1;
	media_info = m_media_info[media_index];
	return 0;
}

int CDataSrc::PerPlay( int s_sec, int e_sec )
{
	if( e_sec != -1 && e_sec <= m_range )
		m_e_sec = s_sec;
	if( s_sec != -1 && s_sec < m_e_sec ){
		m_s_sec = s_sec;
		m_reader.SeekByTime( m_s_sec );
	}
	return 0;
}

int CDataSrc::Play( CSock* sock, int rtp_ch )
{
	CGuard lock( m_mutex );
	m_sock = sock;
	m_rtp_ch = rtp_ch;
	return 0;
}

int CDataSrc::Pause()
{
	CGuard lock( m_mutex );
	m_sock = NULL;
	return 0;
}

void CDataSrc::thread_proc( long user_info )
{
	int ret = 0;
	while( IsDestroyed() == false ){
		int sleep_ms = 5;
		m_mutex.Enter();
		if( m_sock != NULL ){
			char buf[7*TS_PKT_LEN+sizeof(RtpTcpHdr)+sizeof(RtpHdr)] = "";
			int len = 0;
			RtpTcpHdr* hdr = NULL;
			if( m_rtp_ch >= 0 ){
				hdr = (RtpTcpHdr*)(buf+len);
				hdr->dollar = 0x24;
				hdr->channel = m_rtp_ch;
				len += sizeof(RtpTcpHdr);
			}
			RtpHdr* rtp_hd = (RtpHdr*)(buf+len);
			len += sizeof(RtpHdr);
			rtp_hd->version = 2;
			rtp_hd->p = 0;
			rtp_hd->x = 0;
			rtp_hd->cc = 0;
			rtp_hd->m = 1;
			rtp_hd->pt = 33;
			rtp_hd->seq = htons(m_media_info[0].seq++);
			rtp_hd->ssrc = htonl(m_media_info[0].ssrc);
			uint64_t pcr = 0;
			int read_ret = m_reader.GetTsPkt( buf+len, 7*TS_PKT_LEN, pcr );
			if( read_ret < 0 ){
				ret = -1;
				m_mutex.Leave();
				break;
			}
			len += read_ret;
			if( read_ret > 0 ){
				if( hdr != NULL )
					hdr->len = htons(sizeof(RtpHdr)+read_ret);
				if( m_last_pcr != (uint64_t)-1 && pcr > m_last_pcr )
					m_media_info[0].rtp_time += (uint32_t)(pcr - m_last_pcr);
				rtp_hd->ts = htonl(m_media_info[0].rtp_time);
				if(  m_sock->Send( buf, len ) < 0 ){
					ret = -1;
					m_mutex.Leave();
					break;
				}
			}
			m_last_pcr = pcr;
			if( m_start_pcr == (uint64_t)-1 ){
				m_start_pcr = pcr;
				m_start_ms = CNTime::GetCurMs();
				m_mutex.Leave();
				Sleep( 5 );
				continue;
			}
			int64_t diff_ms = (pcr - m_start_pcr)/90;
			sleep_ms = int(m_start_ms + diff_ms - CNTime::GetCurMs());
			if( sleep_ms > 1000 || sleep_ms < -1000 ){
				m_start_ms = -1;
				m_start_pcr = -1;
				m_last_pcr = -1;
				sleep_ms = sleep_ms > 0 ? 1000 : 5;
			}
			sleep_ms = sleep_ms < 5 ? 5 : sleep_ms;
		}
		m_mutex.Leave();
		Sleep( sleep_ms );
	}
	if( ret < 0 && m_fun != NULL )
		m_fun( 0, DATA_SRC_CLOSE, m_user_info );
}
