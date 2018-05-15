#ifndef __P2P_TRANSPORT_H__
#define __P2P_TRANSPORT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct p2p_transport p2p_transport;

typedef struct _p2p_transport_cb
{
	void (*on_create_complete)(p2p_transport *transport,
		int status,
		void *user_data);

	void (*on_disconnect_server)(p2p_transport *transport,
		int status,
		void *user_data);

	void (*on_connect_complete)(p2p_transport *transport,
		int connection_id,
		int status,
		void *transport_user_data,
		void *connect_user_data);

	void (*on_connection_disconnect)(p2p_transport *transport,
		int connection_id,
		void *transport_user_data,
		void *connect_user_data);

	void (*on_accept_remote_connection)(p2p_transport *transport,
		int connection_id,
		int conn_flag, //to see conn_flag in p2p_transport_connect 
		void *transport_user_data);

	void (*on_connection_recv)(p2p_transport *transport,
		int connection_id,
		void *transport_user_data,
		void *connect_user_data,
		char* data,
		int len);

	void (*on_tcp_proxy_connected)(p2p_transport *transport,
		void *transport_user_data,
		void *connect_user_data,
		unsigned short port, 
		char* addr);
}p2p_transport_cb;

#define P2P_DEVICE_TERMINAL 0

#define P2P_CLIENT_TERMINAL 1

typedef struct _p2p_transport_cfg
{
	char* server;
	unsigned short port;
	unsigned char terminal_type;
	char* user; //if terminal_type is P2P_CLIENT_TERMINAL, user ignore
	char* password;//if terminal_type is P2P_CLIENT_TERMINAL, password ignore
	void *user_data;
	int use_tcp_connect_srv;
	char* proxy_addr;
	p2p_transport_cb *cb;
}p2p_transport_cfg;


typedef enum p2p_addr_type
{
    /*host address*/
    P2P_ADDR_TYPE_HOST,

    /*local reflexive address  */
    P2P_ADDR_TYPE_SRFLX,

    /*peer reflexive address*/
    P2P_ADDR_TYPE_PRFLX,

    /*relayed address */
    P2P_ADDR_TYPE_RELAYED,

} p2p_addr_type;

typedef enum p2p_opt
{
	P2P_SNDBUF,
	P2P_RCVBUF,
	P2P_RESET_BUF,
	P2P_PAUSE_RECV,
}p2p_opt;


#define P2P_DECL(type) type

#define P2P_SUCCESS 0

typedef void (*LOG_FUNC)(const char *data, int len);
P2P_DECL(int) p2p_init(LOG_FUNC log_func);

P2P_DECL(void) p2p_uninit();

/*
*  - 0: fatal error
*  - 1: error
*  - 2: warning
*  - 3: info
*  - 4: debug
*  - 5: trace
*  - 6: more detailed trace
* default is 4
*/
P2P_DECL(void) p2p_log_set_level(int level);

P2P_DECL(int) p2p_transport_create(p2p_transport_cfg* cfg,
								  p2p_transport **transport);

P2P_DECL(void) p2p_transport_destroy(p2p_transport *transport);

P2P_DECL(int) p2p_transport_connect(p2p_transport *transport,
								   char* remote_user,
								   void *user_data,
								   int conn_flag,
								   int* connection_id);

P2P_DECL(int) p2p_get_conn_remote_addr(p2p_transport *transport, int connection_id, char* addr, int* addr_len, p2p_addr_type* addr_type);
P2P_DECL(int) p2p_get_conn_local_addr(p2p_transport *transport, int connection_id, char* addr, int* addr_len, p2p_addr_type* addr_type);

P2P_DECL(int) p2p_set_conn_opt(p2p_transport *transport, int connection_id, p2p_opt opt, const void* optval, int optlen);

P2P_DECL(void) p2p_transport_disconnect(p2p_transport *transport,
									   int connection_id);

typedef enum p2p_send_model
{
	P2P_SEND_BLOCK,
	P2P_SEND_NONBLOCK,
}p2p_send_model;

P2P_DECL(int) p2p_transport_send(p2p_transport *transport,
								int connection_id,
								char* buffer,
								int len,
								p2p_send_model model,
								int* error_code);

P2P_DECL(int) p2p_transport_av_send(p2p_transport *transport,
								 int connection_id,
								 char* buffer,
								 int len,
								 int flag, //0 no resend,  1 resend
								 p2p_send_model model,
								 int* error_code);

P2P_DECL(int) p2p_create_tcp_proxy(p2p_transport *transport, 
								  int connection_id, 
								  unsigned short remote_listen_port,
								  unsigned short* local_proxy_port);

P2P_DECL(void) p2p_destroy_tcp_proxy(p2p_transport *transport,
									int connection_id,
									unsigned short local_proxy_port);

P2P_DECL(void) p2p_strerror(int error_code,
						   char *buf,
						   int bufsize);

typedef void (*ON_DETECT_NET_TYPE)(int status, int nat_type, void* user_data);

P2P_DECL(int) p2p_nat_type_detect(char* turn_server, unsigned short turn_port, ON_DETECT_NET_TYPE callback, void* user_data);

P2P_DECL(char*) p2p_get_ver();

P2P_DECL(int) p2p_proxy_get_remote_addr(p2p_transport *transport, unsigned short port, char* addr, int* add_len);

//return value range is from 1 to PJ_STUN_MAX_TRANSMIT_COUNT, the smaller value is better
P2P_DECL(int) p2p_transport_server_net_state(p2p_transport *transport);

typedef enum p2p_global_opt
{
	P2P_MAX_RECV_PACKAGE_LEN, //default 1 M. min 128k
	P2P_MAX_CLIENT_COUNT, //default 4, min 4
	P2P_ENABLE_RELAY, //p2p relay enable, default enable, 1 enable, 0 disable
	P2P_ONLY_RELAY, //p2p disable HOST SRFLX PRFLX, only enable relay, default 0 , 1 only relay
	P2P_SMOOTH_SPAN, //0 disable p2p smooth, min P2P_SMOOTH_MIN_SPAN ms,default P2P_SMOOTH_DEFAULT_SPAN
	P2P_PORT_GUESS, // 0 disable, 1 enable,default enable
}p2p_global_opt;

P2P_DECL(int) p2p_set_global_opt(p2p_global_opt opt, const void* optval, int optlen);

#ifdef __cplusplus /*extern "C"*/
}
#endif

#endif /* __P2P_TRANSPORT_H__ */