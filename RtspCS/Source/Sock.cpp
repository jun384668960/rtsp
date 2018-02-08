#include "Sock.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "PrintLog.h"
#ifdef _WIN32
#include <WinSock2.h>
#else	//_WIN32
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#endif	//_WIN32

CSock::CSock()
{
	m_fd = INVALID_SOCK_FD;
}

CSock::~CSock()
{
	Close();
}

int CSock::GetFd() const
{
	return m_fd;
}

int CSock::AttachFd( int fd )
{	
	if( m_fd != INVALID_SOCK_FD )
		return -1;
	else
		m_fd = fd;
	if( m_fd > 0 ){
		if( set_block_opt( false ) < 0 ){
			m_fd = INVALID_SOCK_FD;
			return -1;
		}
	}
	return 0;
}

int CSock::DetachFd()
{
	int tmp = m_fd;
	m_fd = INVALID_SOCK_FD;
	return tmp;
}

int CSock::open( int sock_type )
{
	Close();
	m_fd = ::socket( AF_INET, sock_type, 0 );
	if( m_fd == -1 ){
		LogError( "create socket failed, err info:%d %s\n", ERROR_NO, ERROR_STR );
		return -1;
	}
	if( set_block_opt( false ) < 0 ){
		Close();
		return -1;
	}
	return m_fd;
}

void CSock::Close()
{
	if( m_fd != INVALID_SOCK_FD ){
#ifdef _WIN32
		::closesocket( m_fd );
#else
		::close( m_fd );
#endif
		m_fd = INVALID_SOCK_FD;
	}
}

int CSock::bind( const char* ip, int port )
{
	struct sockaddr_in local_addr;
	if( get_addr( ip, port, local_addr ) < 0 )
		return -1;
	if( ::bind( m_fd, (struct sockaddr *)&local_addr, sizeof(local_addr) ) == -1 ){
		LogError( "bind failed, err info:%d %s\n", ERROR_NO, ERROR_STR );
		return -1;
	}
	return 0;
}

int CSock::get_addr( const char* ip, int port, sockaddr_in& addr )
{
	int ip_n = 0;
	if( ip == NULL || ip[0] == '\0' )
		ip_n = 0;
	else if( inet_addr(ip) == INADDR_NONE ){
		struct hostent* hent = NULL;
		if( (hent = gethostbyname( ip )) == NULL ){
			LogError( "get host by name failed, name:%s\n", ip );
			return -1;
		}
		memcpy( &ip_n, *(hent->h_addr_list), sizeof(ip_n) );
	}else
		ip_n = inet_addr(ip);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ip_n;
	addr.sin_port = htons( port );
	return 0;
}

int CSock::set_addr_reuse()
{
	int opt = 1;
	if( ::setsockopt( m_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt) ) < 0 ){
		LogError( "set reuseaddr failed, err info:%d %s\n", ERROR_NO, ERROR_STR );
		return -1;
	}
	return 0;
}

int CSock::set_block_opt( bool is_block )
{
	if( m_fd == INVALID_SOCK_FD )
		return 0;
#ifdef _WIN32
	u_long block = 1;
	if( is_block )
		block = 0;
	if( SOCKET_ERROR == ::ioctlsocket( m_fd, FIONBIO, &block ) ){
		LogError( "set socket block failed!\n" );
		return -1;
	}
#else
	int flags = ::fcntl( m_fd, F_GETFL );
	if( flags < 0 ) {
		LogError( "set socket block failed! %s\n", ERROR_STR );
		return -1;
	}
	if( is_block )
		flags &= ~O_NONBLOCK;
	else
		flags |= O_NONBLOCK;
	int retval = ::fcntl( m_fd, F_SETFL, flags );
	if( retval < 0 ){
		LogError( "set socket block failed! %s\n", ERROR_STR );
		return -1;
	}
#endif
	return 0;
}
