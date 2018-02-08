#ifndef __INCLUDE_DEF_H__
#define __INCLUDE_DEF_H__

#include <stdint.h>
#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#else
#include <WinSock2.h>
#endif
#include <string.h>
#include <stdlib.h>

using namespace std;

#ifndef WIN32
#define Sleep( X ) usleep( 1000*X )
#define _vsnprintf vsnprintf
#define _snprintf snprintf
#define _strnicmp strncasecmp
#define _stricmp strcasecmp
#else
#define snprintf _snprintf
#define fopen64 fopen
#define fseeko64 _fseeki64
#define ftello64 _ftelli64
#endif

//系统错误信息
#ifdef _WIN32
#define ERROR_NO WSAGetLastError()
#define ERROR_STR gai_strerror(WSAGetLastError())
#define N_EINPROGRESS WSAEINPROGRESS
#define N_C_EWOULDBLOCK WSAEWOULDBLOCK
#define N_EINTR WSAEINTR
#define N_EINPROGRESS WSAEINPROGRESS
#define N_EWOULDBLOCK WSAEWOULDBLOCK
#define N_EAGAIN WSAEWOULDBLOCK
#else //_WIN32
#define ERROR_NO errno
#define ERROR_STR strerror( errno )
#define N_EINPROGRESS EINPROGRESS
#define N_C_EWOULDBLOCK EINPROGRESS
#define N_EINTR EINTR
#define N_EINPROGRESS EINPROGRESS
#define N_EWOULDBLOCK EWOULDBLOCK
#define N_EAGAIN EWOULDBLOCK
#endif	//_WIN32

//字节对齐
#ifndef _WIN32
#define BYTE_PACKED __attribute__((packed))
#else
#define BYTE_PACKED
#endif

#ifdef _WIN32
#pragma pack(push,1) //设定为1字节对齐
#endif

struct RtpHdr
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	uint8_t cc : 4,  			// CSRC count
			x : 1,   			// header extend
			p : 1,   			// padding flag
			version : 2;		// version
	uint8_t pt : 7,				// payload type
			m : 1;				// mark bit
#elif __BYTE_ORDER == __BIG_ENDIAN
#error "unsupported big-endian"
#else
#error "Please fix <endian.h>"
#endif
	uint16_t seq;				// sequence number;
	uint32_t ts;				// timestamp
	uint32_t ssrc;				// sync source
} BYTE_PACKED;

struct RtpTcpHdr
{
	int8_t	dollar;
	int8_t	channel;
	int16_t	len;
} BYTE_PACKED;

#ifdef _WIN32
#pragma pack(pop) //恢复对齐状态
#endif

#define PRINT_CMD( msg ) LogDebug( "cmd:\n%s\n", msg );
#define PRINT_RECV_CMD( msg, len ) {\
	char msg_tmp[1024];\
	memset( msg_tmp, 0, sizeof(msg_tmp) );\
	memcpy( msg_tmp, msg, len>(int)sizeof(msg_tmp)-1?(int)sizeof(msg_tmp)-1:len );\
	LogDebug( "cmd:\n%s\n", msg_tmp );\
}

typedef enum{
	RTSP_OPTIONS = 0,
	RTSP_DESCRIBE = 1,
	RTSP_SETUP = 2,
	RTSP_PLAY = 3,
	RTSP_PAUSE = 4,
	RTSP_TEARDOWN = 5,
	RTSP_SET_PARAMETER = 6,
	RTSP_GET_PARAMETER = 7,
	RTSP_METHOD_MAX
}RtspMethodT;

struct RtspMethodStr{
	int method;
	const char* method_str;
};

extern const RtspMethodStr g_method[RTSP_METHOD_MAX];

struct RspCodeStr{
	int code;
	const char* code_str;
};

extern const RspCodeStr g_rsp_code_str[15];

enum{
	TS_PKT_LEN = 188,
};

typedef void (*NotifyFun)( long id, long msg,  long user_info );

enum{
	RTSP_SESSION_CLOSE = 0,
	DATA_SRC_CLOSE,
};

#endif
