#ifndef __INCLUDE_UDP_SOCK_H__
#define __INCLUDE_UDP_SOCK_H__

#include "Def.h"
#include "Sock.h"

//UDP套接字类
class CUdpSock : public CSock
{
public:
	CUdpSock();
	~CUdpSock();
public:
	//打开打开套接字
	int Open( const char* ip, int port );
	//设置远端地址
	//参数：remote：远端网络地址
	//返回值：0成功，-1失败
	int SetRemote( const char* ip, int port );
	//发送
	//参数：buf：发送内容，len：发送长度
	//返回值：0成功，-1失败
	int Send( const char* buf, int len );
	//接收
	//参数：buf：接收缓存，len：缓存长度
	//返回值：0成功，-1失败
	int Recv( char* buf, int len );
public:
	//设置多播发送报文的TTL
	//返回值： -1 失败 0 成功
	int SetMulticastTTL( uint8_t TTL );
	//设置是否禁止组播数据回送，true-回送，false-不回送。
	//返回值: -1 失败 0 成功
	int SetMulticastLoop( bool isloop );
private:
	struct sockaddr_in m_remote_addr;
};

#endif