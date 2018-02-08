#include "TcpSock.h"
#include "PrintLog.h"
#include <string.h>
#ifdef _WIN32
#include <WinSock2.h>
#else	//_WIN32
#include <errno.h>
#include <sys/socket.h>
#endif	//_WIN32

CTcpSock::CTcpSock()
{
}

CTcpSock::~CTcpSock()
{
}

int CTcpSock::Connect( const char* ip, int port )
{
	if( open( SOCK_STREAM ) < 0 )
		return -1;
	set_block_opt( true );
	struct sockaddr_in remote;
	if( get_addr( ip, port, remote ) < 0 )
		return -1;
	if( ::connect( m_fd, (struct sockaddr *)&remote, sizeof(struct sockaddr) ) == -1 ){
		if( errno != N_EINPROGRESS )
		if( ERROR_NO != N_EINPROGRESS && ERROR_NO != N_C_EWOULDBLOCK ){
			Close();
			LogError( "connectt failed, err info:%d %s\n", ERROR_NO, ERROR_STR );
			return -1;
		}
	}
	set_block_opt( false );
	return 0;
}

int CTcpSock::Send( const char* buf, int len )
{
	if( buf == NULL || len <= 0)
		return -1;
	int ret = ::send( m_fd, buf, len, 0);
	if( ret < 0 ){
		if( ERROR_NO == N_EWOULDBLOCK || ERROR_NO == N_EINTR || ERROR_NO == N_EAGAIN )
			return 0;
		return -1;
	}
	return ret;
}

int CTcpSock::Recv( char* buf, int len )
{
	if( buf == NULL || len <= 0)
		return -1;
	int ret = ::recv( m_fd, buf, len, 0 );		
	if( ret < 0 ) {
		if( ERROR_NO == N_EWOULDBLOCK || ERROR_NO == N_EINTR || ERROR_NO == N_EAGAIN )
			return 0;
		return -1;
	}else if( ret == 0 )
		return -1;
	return ret;
}
