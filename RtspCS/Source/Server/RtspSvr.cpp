#include "RtspSvr.h"
#include "PrintLog.h"
#include <string.h>

CRtspSvr::CRtspSvr()
{
}

CRtspSvr::~CRtspSvr()
{
	m_mutex.Enter();
	for( map<long, CRtspSession*>::iterator it = m_session_map.begin(); it != m_session_map.end(); ){
		delete it->second;
		m_session_map.erase( it++ );
	}
	m_mutex.Leave();
	Destroy();
	WaitExit();
}

int CRtspSvr::Start( int port )
{
	Close();
	if( Listen( "0.0.0.0", 8554 ) < 0 ){
		LogError( "open listen socket error\n" );
		return -1;
	}
	LogInfo( "Start RTSP Server success! listen port: %d\n", port );
	return 0;
}

void CRtspSvr::accept_sock( int fd )
{
	LogInfo( "Accept a new connect, the fd: %d\n", fd );
	CRtspSession* session = new CRtspSession;
	if( session->Start( fd, notify_fun, (long)this ) >= 0 ){
		m_mutex.Enter();
		m_session_map[fd] = session;
		m_mutex.Leave();
	}else
		delete session;
}

void CRtspSvr::notify_fun( long id, long msg, long user_info )
{
	CRtspSvr* obj = (CRtspSvr*)user_info;
	if( obj != NULL ){
		if( msg == RTSP_SESSION_CLOSE ){
			obj->m_mutex.Enter();
			map<long, CRtspSession*>::iterator it = obj->m_session_map.find( id );
			if( it != obj->m_session_map.end() ){
				delete it->second, it->second = NULL;
				obj->m_session_map.erase( it );
			}
			obj->m_mutex.Leave();
		}
	}
}