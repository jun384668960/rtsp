#ifndef __INCLUDE_TCP_SOCK_H__
#define __INCLUDE_TCP_SOCK_H__

#include "Def.h"
#include "Sock.h"

//TCP套接字
class CTcpSock : public CSock
{
public:
	CTcpSock();
	~CTcpSock();
public:
	int Connect( const char* ip, int port );
	//发送
	//参数：buf：发送内容，len：发送长度
	//返回值: <0失败，>=0发送长度
	int Send( const char* buf, int len );
	//接收
	//参数：buf：接收缓存，len：接收缓存长度
	//返回值: <0失败，>=0接收长度
	int Recv( char* buf, int len );
};

#endif
