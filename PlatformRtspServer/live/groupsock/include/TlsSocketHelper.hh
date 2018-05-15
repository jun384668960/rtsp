#ifndef _TLS_SOCKET_HELPER_H_
#define _TLS_SOCKET_HELPER_H_

/*
	1.支持TLSv1.2
	2. client : 
		//////////////////////////////////////////////////////////////////////////
		阻塞方式:
		2.1 初始化
					-> TlsHelper_Init();
					-> TlsHelper_InitClient(NULL);
		2.2 连接服务器,建立TLS通道
					-> TlsHelper_ConnectBolck(sock); 此处sock必须是已经跟服务器建立了连接的套接字
		2.3 收发数据
					-> TlsHelper_ReadBlock(tlsHelp,buff,buflen);
					-> TlsHelper_WriteBlock(tlsHelp,buff,buflen);
		
		//////////////////////////////////////////////////////////////////////////
		非阻塞方式:
		2.4 初始化
					-> TlsHelper_Init();
					-> TlsHelper_InitClient(NULL);
		2.5 连接服务器，建立TLS通道
					-> TlsHelper_ConnectNonBolck(sock); 需要在  [合适的位置]   调用TlsHelper_HandShake ，进行TLS的握手动作
					-> TlsHelper_HandShake(tlsHelp,isconnected, fdstus)	isconnected == 1时表示，握手过程完成，TLS通道建立成功
																										isconnected == 0, 查看fdstus的状态
																											fdstus == FDSTUS_WANT_READ,需要监听读事件
																											fdstus == FDSTUS_WRITE_WRITE, 需要监听写事件
																											fdstus == FDSTUS_ERROR 连接发生异常
					[合适的位置] -> 一般当产生可读事件，但是TLS握手还没有完成时，调用TlsHelper_HandShake，如果产生可读事件，TLS通道已经建立了，则可以处理读事件的逻辑
										-> 一般当产生可写事件，但是TLS握手还没有完成时，调用TlsHelper_HandShake，如果产生可写事件，TLS通道已经建立了，则可以处理写事件的逻辑
		2.6 收发数据
					-> TlsHelper_ReadNonBlock(tlsHelp,buff,buflen,fdstus);	当返回的长度跟接收的长度不一致，查看fdstus的状态
																											1.当fdstus == FDSTUS_WANT_READ, 表示当前没有数据可读，可以继续
																											2.其他情况，连接异常
					-> TlsHelper_WriteNonBlock(tlsHelp,buff,buflen,fdstus);	当返回的长度跟发送的长度不一致，查看fdstus的状态
																											1.当fdstus == FDSTUS_WANT_WRITE, 表示当前不能在继续发送数据，需要等待可以发送数据时在进行，可以继续
																											2.其他情况，连接异常
	3.server:
		//////////////////////////////////////////////////////////////////////////
		阻塞方式:
		3.1 初始化
					->TlsHelper_Init();
					-> TlsHelper_InitServer( "cafile"/null, "capath"/null,"cert.pem", "key.pem" ); 加载证书和必要的初始化工作
		3.2 接收连接，建立TLS通道
					-> TlsHelper_AcceptBlock(sock); 此处sock必须是已经建立了连接的套接字
		3.3 收发数据
					-> TlsHelper_ReadBlock(tlsHelp,buff,buflen);
					-> TlsHelper_WriteBlock(tlsHelp,buff,buflen);

		//////////////////////////////////////////////////////////////////////////
		非阻塞方式:
		3.4 初始化
					-> TlsHelper_Init();
					-> TlsHelper_InitServer( "cafile"/null, "capath"/null,"cert.pem", "key.pem" ); 加载证书和必要的初始化工作
		3.5接受连接，建立TLS通道
					-> TlsHelper_AcceptNonBlock(sock); 需要在  [合适的位置]   调用TlsHelper_HandShake ，进行TLS的握手动作
					-> TlsHelper_HandShake(tlsHelp,isconnected, fdstus)	isconnected == 1时表示，握手过程完成，TLS通道建立成功
																										isconnected == 0, 查看fdstus的状态
																										fdstus == FDSTUS_WANT_READ,需要监听读事件
																										fdstus == FDSTUS_WRITE_WRITE, 需要监听写事件
																										fdstus == FDSTUS_ERROR 连接发生异常
					[合适的位置] -> 一般当产生可读事件，但是TLS握手还没有完成时，调用TlsHelper_HandShake，如果产生可读事件，TLS通道已经建立了，则可以处理读事件的逻辑
										-> 一般当产生可写事件，但是TLS握手还没有完成时，调用TlsHelper_HandShake，如果产生可写事件，TLS通道已经建立了，则可以处理写事件的逻辑
		3.6 收发数据
					-> TlsHelper_ReadNonBlock(tlsHelp,buff,buflen,fdstus);	当返回的长度跟接收的长度不一致，查看fdstus的状态
																											1.当fdstus == FDSTUS_WANT_READ, 表示当前没有数据可读，可以继续
																											2.其他情况，连接异常
					-> TlsHelper_WriteNonBlock(tlsHelp,buff,buflen,fdstus);	当返回的长度跟发送的长度不一致，查看fdstus的状态
																											1.当fdstus == FDSTUS_WANT_WRITE, 表示当前不能在继续发送数据，需要等待可以发送数据时在进行，可以继续
																											2.其他情况，连接异常
	4. 关闭连接
		TlsHelper_Shutdown(tlsHelp);
*/

typedef void* TlsHelperObj;
#define TlsHelperObjNull NULL
typedef enum {
	FDSTUS_WANT_WRITE = 0,
	FDSTUS_WANT_READ,
	FDSTUS_ERROR,
}FdStus;

//初始化函数，只需要调用一次
void TlsHelper_Init(); //初始化OPENSSL
void TlsHelper_UnInit();
int TlsHelper_InitServer(const char* caFile, const char* caPath, const char* certPem, const char* keyPem); //加载证书,返回0表示成功
int TlsHelper_InitClient(const char* pCipherList); //pCipherList == NULL, 使用默认, 返回0表示成功

//阻塞
TlsHelperObj TlsHelper_AcceptBlock(int sock); //返回TlsHelperObjNull 表示失败 //sock 必须是连接已经建立好了的
TlsHelperObj TlsHelper_ConnectBolck(int sock); //返回TlsHelperObjNull 表示失败 //sock 必须是连接已经建立好了的
int TlsHelper_ReadBlock(TlsHelperObj tlsHelp,void* buf, int buflen); //>0 表示读取字节数, 否则连接异常
int TlsHelper_WriteBlock(TlsHelperObj tlsHelp,void* buf, int buflen); //>0 表示写入字节数, 否则连接异常

//非阻塞
TlsHelperObj TlsHelper_AcceptNonBlock(int sock); //返回TlsHelperObjNull 表示失败, sock 必须是连接已经建立好了的
TlsHelperObj TlsHelper_ConnectNonBolck(int sock); //返回TlsHelperObjNull 表示失败, sock 必须是连接已经建立好了的
void TlsHelper_HandShake(TlsHelperObj tlsHelp, int *isConnected, FdStus *fdStus); //非阻塞时，需要调用此接口，完成握手

/*
当返回的长度跟接收的长度不一致，查看fdstus的状态
1.当fdstus == FDSTUS_WANT_READ, 表示当前没有数据可读，可以继续
2.其他情况，连接异常
//>0 表示读取字节数
*/
int TlsHelper_ReadNonBlock(TlsHelperObj tlsHelp,void* buf, int buflen, FdStus *fdStus); 

/*
当返回的长度跟发送的长度不一致，查看fdstus的状态
1.当fdstus == FDSTUS_WANT_WRITE, 表示当前不能在继续发送数据，需要等待可以发送数据时在进行，可以继续
2.其他情况，连接异常
//>0 表示写入字节数
*/
int TlsHelper_WriteNonBlock(TlsHelperObj tlsHelp,void* buf, int buflen, FdStus *fdStus); 

void TlsHelper_Shutdown(TlsHelperObj tlsHelp);

#endif //_TLS_SOCKET_HELPER_H_