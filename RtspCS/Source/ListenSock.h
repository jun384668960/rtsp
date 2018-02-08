#ifndef __INCLUDE_LISTEN_SOCK_H__
#define __INCLUDE_LISTEN_SOCK_H__

#include "Def.h"
#include "Sock.h"
#include "TcpSock.h"
#include "Thread.h"

//¼àÌýÌ×½Ó×ÖÀà
class CListenSock : public CSock, public CThread
{
public:
	CListenSock();
	~CListenSock();
public:
	int Listen( const char* ip, int port );
private:
	virtual void accept_sock( int fd ){ return; }
	virtual void thread_proc( long user_info );
};

#endif
