#include "UdpSock.h"
#include <stdio.h>
#include "PrintLog.h"
#include <string.h>
#ifdef _WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#define  SOL_IP  IPPROTO_IP
#else	//_WIN32
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif	//_WIN32

CUdpSock::CUdpSock()
{
	memset( &m_remote_addr, 0, sizeof(m_remote_addr) );
}

CUdpSock::~CUdpSock()
{

}

int CUdpSock::Open( const char* ip, int port )
{
	if( CSock::open( SOCK_DGRAM ) < 0 )
		return -1;
	struct in_addr if_req; 
	if_req.s_addr = inet_addr(ip);
	if (if_req.s_addr == INADDR_NONE){
		LogError( "inet_addr failed,, %s\n", ERROR_STR);
		Close();
		return -1;
	}
	int bind_ret = -1;
	uint32_t ip_h = ntohl( if_req.s_addr );
	if( ip_h >= 0xe0000000 && ip_h < 0xf0000000 ){
		struct ip_mreq mul_ip_mreq;
		::memcpy( &(mul_ip_mreq.imr_multiaddr), &if_req.s_addr, sizeof(struct in_addr) );
		mul_ip_mreq.imr_interface.s_addr = htonl( INADDR_ANY );
		if( ::setsockopt( m_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mul_ip_mreq, sizeof(struct ip_mreq) ) < 0 ){
			Close();
			LogError( "setsockopt IP_ADD_MEMBERSHIP failed, %s\n", ERROR_STR);
			return -1;
		}
		bind_ret = bind( "0.0.0.0", port );
	}	
	bind_ret = bind( ip, port );
	if( bind_ret < 0 ){
		Close();
		return -1;
	}
	return 0;
}

int CUdpSock::SetRemote( const char* ip, int port )
{
	return get_addr( ip, port, m_remote_addr );
}

int CUdpSock::Send( const char* buf, int len )
{
	if( buf == NULL || len <= 0 )
		return -1;
	int ret = ::sendto( m_fd, buf, len, 0, (sockaddr*)&m_remote_addr, sizeof(m_remote_addr) );
	if( ret < 0 ){
		if( ERROR_NO == N_EWOULDBLOCK || ERROR_NO == N_EINTR || ERROR_NO == N_EAGAIN )
			return 0;
		return -1;
	}
	return ret;
}

int CUdpSock::Recv( char* buf, int len )
{
	int ret = -1;
	struct sockaddr_in tmp_addr;
#ifdef _WIN32
	int addr_len = sizeof( tmp_addr );
#else
	socklen_t addr_len = sizeof( tmp_addr );
#endif
	ret = ::recvfrom( m_fd, buf, len, 0, (sockaddr *)&tmp_addr, &addr_len );
	if( ret < 0 ){
		if( ERROR_NO == N_EWOULDBLOCK || ERROR_NO == N_EINTR || ERROR_NO == N_EAGAIN )
			return 0;
		return -1;
	}
	return ret;
}

int CUdpSock::SetMulticastTTL( uint8_t TTL )
{
	if( setsockopt( m_fd, SOL_IP, IP_MULTICAST_TTL, (char *)&TTL, sizeof(TTL) ) < 0 ){
		LogError( "IP_MULTICAST_TTL failed, %s\n", ERROR_STR);
		return -1;
	}
	return 0;
}

int CUdpSock::SetMulticastLoop( bool isloop )
{
	int loop = (isloop == true ? 1 : 0);
	if( setsockopt( m_fd, SOL_IP, IP_MULTICAST_LOOP, (char *)&loop, sizeof(loop) ) < 0 ){
		LogError( "IP_MULTICAST_LOOP failed, %s\n", ERROR_STR);
		return -1;
	}
	return 0;
}
