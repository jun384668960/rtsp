#ifndef __GSS_TRANSPORT_H__
#define __GSS_TRANSPORT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "p2p_transport.h"

//device main connection callback
typedef struct gss_dev_main_cb
{
	//connect server result, status is 0 ok
	void (*on_connect_result)(void *transport, void* user_data, int status);

	//disconnect from server, status is error code
	void (*on_disconnect)(void *transport, void* user_data, int status);

	//on accept client signaling connection
	void (*on_accept_signaling_client)(void *transport, void* user_data, unsigned short client_conn);

	//on client signaling connection disconnect
	void (*on_disconnect_signaling_client)(void *transport, void* user_data, unsigned short client_conn);  

	//on accept client audio and video stream connection
	void (*on_recv_av_request)(void *transport, void* user_data, unsigned int client_conn); 

	//receive data of signaling connection, arg 3 client_conn is on_accept_signaling_client of on_accept_client
	void (*on_recv)(void *transport, void *user_data, unsigned short client_conn, char* data, int len);
}gss_dev_main_cb;

//device main connection config
typedef struct gss_dev_main_cfg
{
	char* server;
	unsigned short port;
	char* uid; 
	void *user_data;
	gss_dev_main_cb *cb;
}gss_dev_main_cfg;

//device main connection connect server
P2P_DECL(int) gss_dev_main_connect(gss_dev_main_cfg* cfg, void** transport);

//send data to client, client_conn is gss_dev_main_cb.on_accept_signaling_client third argument
P2P_DECL(int) gss_dev_main_send(void *transport, unsigned short client_conn, char* buf, int buffer_len, p2p_send_model model);

//destroy device main connection
P2P_DECL(void) gss_dev_main_destroy(void* transport);


//device audio and video connection callback
typedef struct gss_dev_av_cb
{
	//connect server result, status is 0 ok
	void (*on_connect_result)(void *transport, void* user_data, int status);

	//disconnect from server, status is error code
	void (*on_disconnect)(void *transport, void* user_data, int status);

	//receive client audio and video response data
	void (*on_recv)(void *transport, void *user_data, char* data, int len);

	//client audio and video connection disconnect from server
	void (*on_client_disconnect)(void *transport, void *user_data);
}gss_dev_av_cb;

//device audio and video connection config
typedef struct gss_dev_av_cfg
{
	char* server;
	unsigned short port;
	char* uid; 
	void *user_data;
	unsigned int client_conn; //gss_dev_main_cb.on_recv_av_request third argument
	gss_dev_av_cb *cb;
}gss_dev_av_cfg;

P2P_DECL(int) gss_dev_av_connect(gss_dev_av_cfg* cfg, void** transport);  

//send audio and video data to client
typedef enum GSS_DATA_TYPE
{
	GSS_COMMON_DATA,
	GSS_REALPLAY_DATA,
}GSS_DATA_TYPE;
P2P_DECL(int) gss_dev_av_send(void *transport, char* buf, int buffer_len, p2p_send_model model, int type);

//clean all send buffer data
P2P_DECL(void) gss_dev_av_clean_buf(void* transport);

//destroy device audio and video stream connection
P2P_DECL(void) gss_dev_av_destroy(void* transport);

#define RTMP_EVENT_CONNECT_SUCCESS (0)
#define RTMP_EVENT_CONNECT_FAIL (1)
#define RTMP_EVENT_DISCONNECT (2)

//device push audio and video connection callback
typedef struct gss_dev_push_cb
{
	//connect server result, status is 0 ok
	void (*on_connect_result)(void *transport, void* user_data, int status);

	//disconnect from server, status is error code
	void (*on_disconnect)(void *transport, void* user_data, int status);

	void (*on_rtmp_event)(void *transport, void* user_data, int event);

	void (*on_pull_count_changed)(void *transport, void* user_data, unsigned int count);
}gss_dev_push_cb;

//device push connection config
typedef struct gss_dev_push_conn_cfg
{
	char* server;
	unsigned short port;
	char* uid; 
	void *user_data;
	gss_dev_push_cb *cb;
}gss_dev_push_conn_cfg;

//device push audio and video stream connection connect server
P2P_DECL(int) gss_dev_push_connect(gss_dev_push_conn_cfg* cfg, void** transport);


#define GSS_VIDEO_DATA 0
#define GSS_AUDIO_DATA 1

//send audio and video response data to server
///type 0 video, 1 audio
// is_key : frame is key frame
P2P_DECL(int) gss_dev_push_send(void *transport, char* buf, int buffer_len, char type, unsigned int time_stamp, char is_key, p2p_send_model model);

//push audio and video to third rtmp server
P2P_DECL(int) gss_dev_push_rtmp(void *transport, char* url);

//destroy device push audio and video stream connection
P2P_DECL(void) gss_dev_push_destroy(void* transport);

//client connection callback
typedef struct gss_client_conn_cb
{
	//connect device result, status is 0 ok
	void (*on_connect_result)(void *transport, void* user_data, int status);

	//disconnect from server, status is error code
	void (*on_disconnect)(void *transport, void* user_data, int status);

	//receive device data
	void (*on_recv)(void *transport, void *user_data, char* data, int len);

	//device disconnect from server
	void (*on_device_disconnect)(void *transport, void *user_data);
}gss_client_conn_cb;

typedef struct gss_pull_conn_cb
{
	//connect device result, status is 0 ok
	void (*on_connect_result)(void *transport, void* user_data, int status);

	//disconnect from server, status is error code
	void (*on_disconnect)(void *transport, void* user_data, int status);

	//receive device data
	//type 0 video, 1 audio
	void (*on_recv)(void *transport, void *user_data, char* data, int len, char type, unsigned int time_stamp);

	//device disconnect from server
	void (*on_device_disconnect)(void *transport, void *user_data);
}gss_pull_conn_cb;

//client connection config
typedef struct gss_client_conn_cfg
{
	char* server;
	unsigned short port;
	char* uid; 
	void *user_data;
	gss_client_conn_cb *cb;
}gss_client_conn_cfg;

//client connection config
typedef struct gss_pull_conn_cfg
{
	char* server;
	unsigned short port;
	char* uid; 
	void *user_data;
	gss_pull_conn_cb *cb;
}gss_pull_conn_cfg;

//client signaling connection connect server
P2P_DECL(int) gss_client_signaling_connect(gss_client_conn_cfg* cfg, void** transport);

//send data to device
P2P_DECL(int) gss_client_signaling_send(void *transport, char* buf, int buffer_len, p2p_send_model model);

//destroy client signaling connection
P2P_DECL(void) gss_client_signaling_destroy(void* transport);

//client audio and video stream connection connect server
P2P_DECL(int) gss_client_av_connect(gss_client_conn_cfg* cfg, void** transport);

//send audio and video response data to device
P2P_DECL(int) gss_client_av_send(void *transport, char* buf, int buffer_len, p2p_send_model model);

//destroy client audio and video stream connection
P2P_DECL(void) gss_client_av_destroy(void* transport);

//clean all buffer data
P2P_DECL(void) gss_client_av_clean_buf(void* transport);

//pause receive device data
P2P_DECL(void) gss_client_av_pause_recv(void* transport, int is_pause);

//client audio and video pull stream connection connect server
P2P_DECL(int) gss_client_pull_connect(gss_pull_conn_cfg* cfg, void** transport);

//destroy client audio and video pull stream connection
P2P_DECL(void) gss_client_pull_destroy(void* transport);

#ifdef __cplusplus /*extern "C"*/
}
#endif

#endif /* __GSS_TRANSPORT_H__ */