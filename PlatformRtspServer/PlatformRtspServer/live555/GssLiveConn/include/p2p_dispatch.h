#ifndef __P2P_DISPATCH_H__
#define __P2P_DISPATCH_H__

#include "p2p_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*DISPATCH_CALLBACK)(void* dispatcher, int status, void* user_data, char* server, unsigned short port, unsigned int server_id); 

P2P_DECL(int) p2p_request_dispatch_server(char* user, 
										  char* password, 
										  char* ds_addr, 
										  void* user_data, 
										  DISPATCH_CALLBACK cb,
										  void** dispatcher);

P2P_DECL(int) p2p_query_dispatch_server(char* dest_user, 
										char* ds_addr, 
										void* user_data, 
										DISPATCH_CALLBACK cb,
										void** dispatcher);

P2P_DECL(int) gss_request_dispatch_server(char* user, 
										  char* password, 
										  char* ds_addr, 
										  void* user_data, 
										  DISPATCH_CALLBACK cb,
										  void** dispatcher);

P2P_DECL(int) gss_query_dispatch_server(char* dest_user, 
										char* ds_addr, 
										void* user_data, 
										DISPATCH_CALLBACK cb,
										void** dispatcher);

P2P_DECL(void) destroy_p2p_dispatch_requester(void* dispatcher);

P2P_DECL(void) destroy_gss_dispatch_requester(void* dispatcher);

#ifdef __cplusplus /*extern "C"*/
}
#endif

#endif /* __P2P_DISPATCH_H__ */