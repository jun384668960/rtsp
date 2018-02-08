#ifndef __INCLUDE_RTSP_SVR_H__
#define __INCLUDE_RTSP_SVR_H__

#include "ListenSock.h"
#include <map>
#include "RtspSession.h"

class CRtspSvr : public CListenSock
{
public: 
	CRtspSvr();
	~CRtspSvr();
public:
	int Start( int port );
private:
	virtual void accept_sock( int fd );
	static void notify_fun( long id, long msg, long user_info );
private:
	map<long, CRtspSession*> m_session_map;
	CMutex m_mutex;
};

#endif
