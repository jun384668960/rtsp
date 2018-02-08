#include "RtspSvr.h"
#include "PrintLog.h"

#ifdef _WIN32
#pragma comment(lib,"ws2_32.lib")
#endif

int un_init_environment()
{
#ifdef _WIN32
	if (WSACleanup() == SOCKET_ERROR)
		return -1;	
#endif
	return 0;
}

int init_environment()
{
#ifdef _WIN32
	WSADATA wsa_data;
	WORD version_req = MAKEWORD( 2, 2 );	
	int nErr = WSAStartup( version_req, &wsa_data );
	if ( nErr != 0 ){
		LogError( "WSAStartup failed!\n" );
		return -1;
	}
	if ( LOBYTE( wsa_data.wVersion ) != 2 || HIBYTE( wsa_data.wVersion ) != 2 ){
		LogError( "check Berkeley Sockets version failed!\n" );
		un_init_environment();
		return -1; 
	}
#endif
	return 0;
}

int main( void )
{
	init_environment();
	CRtspSvr rtsp_svr;
	rtsp_svr.Start( 8554 );
	getchar();
	return 0;
}