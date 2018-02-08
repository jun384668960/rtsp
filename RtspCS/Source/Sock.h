#ifndef __INCLUDE_SOCK_H__
#define __INCLUDE_SOCK_H__

#include "Def.h"
#ifdef _WIN32
#include <Ws2tcpip.h>
#else	//_WIN32
#include <arpa/inet.h>
#endif	//_WIN32

//套接字基类
class CSock{
public:
	CSock();
	virtual ~CSock();
public:
	//关闭套接字，对象析构时将自动调用
	void Close();
	//获取套接字fd
	//返回值：-1表示该套接字类不可用
	int GetFd() const;
	//绑定socket fd
	//参数：fd：socket fd
	//返回值：0成功，-1失败
	int AttachFd( int fd );
	//解除绑定
	//返回值：之前绑定的fd
	int DetachFd();
	//发送
	//参数：buf：发送内容，len：发送长度
	//返回值: <0失败，>=0发送长度
	virtual int Send( const char* buf, int len ){ return -1; }
	//接收
	//参数：buf：接收缓存，len：接收缓存长度
	//返回值: <0失败，>=0接收长度
	virtual int Recv( char* buf, int len ){ return -1; }
protected:
	int open( int sock_type );
	int bind( const char* ip, int port );
	int get_addr( const char* ip, int port, sockaddr_in& addr );
	int set_addr_reuse();
	int set_block_opt( bool is_block );
protected:
	enum{
		INVALID_SOCK_FD = -1,
	};
	int m_fd;
};

#endif
