#include "TlsSocketHelper.hh"

#include "openssl/ssl.h"
#include "openssl/rand.h"
#include "openssl/err.h"

#include <sys/socket.h>
#include <stdio.h>

typedef struct _tls_helper{
	SSL *ssl;
	int sslConnected;
	int sock;
}STlsHelper;

SSL_CTX *g_svrSslCtx = NULL;
SSL_CTX *g_cliSslCtx = NULL;
BIO* g_errBio = NULL;

int TlsHelper_UnInitServer();
int TlsHelper_UnInitClient();

void TlsHelper_Init()
{
	SSL_load_error_strings();                /* readable error messages */
	SSL_library_init();                      /* initialize library */
	g_errBio = BIO_new_fd(2, BIO_NOCLOSE); // 0、1、2的fd分别代表标准输入、标准输出和标准错误输出
}

void TlsHelper_UnInit()
{
	TlsHelper_UnInitServer();
	TlsHelper_UnInitClient();
	BIO_free(g_errBio);
}

int TlsHelper_InitServer(const char* caFile, const char* caPath, const char* certPem, const char* keyPem)
{
	int rlt = -1; 
	SSL_METHOD *meth = NULL;
	SSL_CTX* pssl_ctx = NULL;

	do 
	{
		if (g_svrSslCtx == NULL)
		{
			meth = (SSL_METHOD *)SSLv23_method();
			if(meth == NULL)
				break;

			pssl_ctx = SSL_CTX_new(meth);
			if(pssl_ctx == NULL)
				break;
			if (caFile || caPath)
			{
				// 加载CA的证书  
				if( SSL_CTX_load_verify_locations(pssl_ctx, caFile, caPath) !=1 )
				{
					printf("CAfile and CApath are NULL or the processing at one of the locations specified failed!\n");
					break;
				}
			}
//			if( SSL_CTX_use_certificate_file(pssl_ctx,certPem, SSL_FILETYPE_PEM) != 1)
			if( SSL_CTX_use_certificate_chain_file(pssl_ctx,certPem) != 1)
				break;
			if( SSL_CTX_use_PrivateKey_file(pssl_ctx, keyPem, SSL_FILETYPE_PEM) != 1)
				break;
			if( SSL_CTX_check_private_key(pssl_ctx) != 1)
				break;		
			
			g_svrSslCtx = pssl_ctx;
		}
		
		rlt = 0;
	} while (0);

	return rlt;
}

int TlsHelper_UnInitServer()
{
	if (g_svrSslCtx)
	{
		SSL_CTX_free(g_svrSslCtx);
		g_svrSslCtx = NULL;
	}
	return 0;
}

int TlsHelper_InitClient(const char* pCipherList)
{
	int rlt = -1;
	SSL_METHOD *meth = NULL;
	SSL_CTX* pssl_ctx = NULL;
	const char* cipher_list = NULL;

	do 
	{
		if (g_cliSslCtx)
		{
			rlt = 0;
			break;
		}

		if (pCipherList)
		{
			cipher_list = pCipherList;
		}
		else
		{
			//创建SSL context,see https://www.openssl.org/docs/manmaster/man1/ciphers.html
			cipher_list = "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384";
		}

		meth = (SSL_METHOD *)SSLv23_client_method();
// 		meth = (SSL_METHOD *)TLSv1_2_client_method();
		if(meth == NULL)
			break;

		pssl_ctx = SSL_CTX_new(meth);
		if(pssl_ctx == NULL)
			break;

		//生成随机数,srand初始化随机种子,rand产生随机数
// 		int i = 0;
// 		int seed_int[100]={0};    
// 		srand((unsigned)time(NULL));
// 		for (i = 0; i < 100; i++)
// 		{
// 			seed_int[i] = rand();
// 		}
// 		//生成伪随机数
// 		RAND_seed(seed_int, sizeof(seed_int));

		//设置ctx可用的加密类型
// 		SSL_CTX_set_cipher_list(pssl_ctx, cipher_list);
// 		SSL_CTX_set_mode(pssl_ctx, SSL_MODE_AUTO_RETRY);
		
		g_cliSslCtx = pssl_ctx;
		rlt = 0;
	}while(0);

	return rlt;
}

int TlsHelper_UnInitClient()
{
	if (g_cliSslCtx)
	{
		SSL_CTX_free(g_cliSslCtx);
		g_cliSslCtx = NULL;
	}
	return 0;
}

TlsHelperObj TlsHelper_AcceptBlock(int sock)
{
	int rlt = 0;
	int ret = 0;
// 	SSL_METHOD *meth = NULL;
	SSL_CTX* pssl_ctx = NULL;
	SSL *ssl = NULL;
	STlsHelper *tlsHelp = NULL;
	do 
	{
		pssl_ctx = g_svrSslCtx;
		if(pssl_ctx == NULL)
			break;

		ssl = SSL_new(pssl_ctx);
		if(ssl == NULL)
			break;

		if( (ret = SSL_set_fd(ssl, sock)) != 1)
			break;
		
// 		if( (ret = SSL_accept(ssl)) != 1)
// 			break;
		SSL_set_accept_state(ssl);
		ret = SSL_do_handshake(ssl);
		if(ret != 1)
			break;

		rlt = 1;
	} while (0);

	if (rlt != 1)
	{
		if (ssl)
		{
			printf("ssl error = %d\n",SSL_get_error(ssl,ret));
			ERR_print_errors(g_errBio);
			SSL_free(ssl);
		}
	}
	else
	{
		tlsHelp = (STlsHelper*)malloc(sizeof(STlsHelper));
		tlsHelp->ssl = ssl;
		tlsHelp->sock = sock;
		tlsHelp->sslConnected = 1;
	}

	return (TlsHelperObj)tlsHelp;
}

TlsHelperObj TlsHelper_AcceptNonBlock(int sock)
{
	int rlt = 0;
// 	SSL_METHOD *meth = NULL;
	SSL_CTX* pssl_ctx = NULL;
	SSL *ssl = NULL;
	STlsHelper *tlsHelp = NULL;
	do 
	{
		pssl_ctx = g_svrSslCtx;
		if(pssl_ctx == NULL)
			break;

		ssl = SSL_new(pssl_ctx);
		if(ssl == NULL)
			break;

		if( SSL_set_fd(ssl, sock) != 1)
			break;

		SSL_set_accept_state(ssl);

		rlt = 1;
	} while (0);

	if (rlt != 1)
	{
		if (ssl)
		{
			SSL_free(ssl);
		}
	}
	else
	{
		tlsHelp = (STlsHelper*)malloc(sizeof(STlsHelper));
		tlsHelp->ssl = ssl;
		tlsHelp->sock = sock;
		tlsHelp->sslConnected = 0;
	}

	return (TlsHelperObj)tlsHelp;
}

TlsHelperObj TlsHelper_ConnectBolck(int sock)
{
	int rlt = 0;
//	SSL_METHOD *meth = NULL;
	SSL_CTX* pssl_ctx = NULL;
	SSL *ssl = NULL;
	STlsHelper *tlsHelp = NULL;
//	const char* cipher_list = NULL;

	do 
	{
		pssl_ctx = g_cliSslCtx;
		if(pssl_ctx == NULL)
			break;

		ssl = SSL_new(pssl_ctx);
		if(ssl == NULL)
			break;

		if( SSL_set_fd(ssl, sock) != 1)
			break;

		if(SSL_connect(ssl) != 1)
			break;

		SSL_CTX_set_timeout(pssl_ctx,5);

		rlt = 1;
	} while (0);

	if (rlt != 1)
	{
		if (ssl)
		{
			SSL_free(ssl);
		}
	}
	else
	{
		tlsHelp = (STlsHelper*)malloc(sizeof(STlsHelper));
		tlsHelp->ssl = ssl;
		tlsHelp->sock = sock;
		tlsHelp->sslConnected = 1;
	}

	return (TlsHelperObj)tlsHelp;
}

TlsHelperObj TlsHelper_ConnectNonBolck(int sock)
{
	int rlt = 0;
// 	SSL_METHOD *meth = NULL;
	SSL_CTX* pssl_ctx = NULL;
	SSL *ssl = NULL;
	STlsHelper *tlsHelp = NULL;
//	const char* cipher_list = NULL;

	do 
	{
		pssl_ctx = g_cliSslCtx;
		if(pssl_ctx == NULL)
			break;

		ssl = SSL_new(pssl_ctx);
		if(ssl == NULL)
			break;

		if( SSL_set_fd(ssl, sock) != 1)
			break;
		
		SSL_set_connect_state(ssl);
		SSL_CTX_set_timeout(pssl_ctx,5);

		rlt = 1;
	} while (0);

	if (rlt != 1)
	{
		if (ssl)
		{
			SSL_free(ssl);
		}
	}
	else
	{
		tlsHelp = (STlsHelper*)malloc(sizeof(STlsHelper));
		tlsHelp->ssl = ssl;
		tlsHelp->sock = sock;
		tlsHelp->sslConnected = 0;
	}

	return (TlsHelperObj)tlsHelp;
}

void TlsHelper_Shutdown(TlsHelperObj tlsHelp)
{
	if (tlsHelp != TlsHelperObjNull)
	{
		STlsHelper* tmpTlsHelper = (STlsHelper*)tlsHelp;
		if( SSL_shutdown(tmpTlsHelper->ssl) != 1)
			SSL_shutdown(tmpTlsHelper->ssl);
		SSL_free(tmpTlsHelper->ssl);
		shutdown(tmpTlsHelper->sock,1); //关闭基础socket，该socket没有用作其他用处
		free(tmpTlsHelper);
	}
}

int TlsHelper_ReadBlock(TlsHelperObj tlsHelp,void* buf, int buflen)
{
	int nCount = 0;
	if(tlsHelp != TlsHelperObjNull)
	{
		STlsHelper* pHelper = (STlsHelper*)tlsHelp;
		nCount = SSL_read(pHelper->ssl,buf,buflen);
	}
	return nCount;
}

int TlsHelper_WriteBlock(TlsHelperObj tlsHelp,void* buf, int buflen)
{
	int nCount = 0;
//	printf("=============tls write buffer real: \nlen = %d,->%s\n",buflen,(char*)buf);
	if(tlsHelp != TlsHelperObjNull)
	{
		STlsHelper* pHelper = (STlsHelper*)tlsHelp;
		nCount = SSL_write(pHelper->ssl,buf,buflen);
	}
	return nCount;
}

int TlsHelper_WriteNonBlock( TlsHelperObj tlsHelp,void *buf,int num , FdStus *fdStus)
{
	if(buf == NULL || tlsHelp == TlsHelperObjNull)
	{
		return -1;
	}

	int res, count = 0;
// 	int value = 0;
// 	static int size = 0;
// 	int iRet = 0;
// 	int t;
	STlsHelper* pHelper = (STlsHelper*)tlsHelp;
	SSL *ssl = pHelper->ssl;
	//printf("=========non tls write buffer real: \nlen = %d,->%s\n",num,(char*)buf);

	while(count < num)
	{

		res = SSL_write(ssl, buf, num - count);

		int nRes = SSL_get_error(ssl, res);
		if(nRes == SSL_ERROR_NONE)
		{
			if(res > 0)
			{
				count += res;
			}
			//printf("SSL_write number = %d,count = %d\n",res,count);
		}
		else if (nRes == SSL_ERROR_WANT_WRITE)
		{
			*fdStus = FDSTUS_WANT_WRITE;
// #if 0
// 			int iRet = getsockopt(g_chHedgwReal->sock, SOL_SOCKET, SO_SNDBUF, (void *)&size, (socklen_t *)&t);
// 			if (iRet != 0)
// 			{
// 				printf("getsockopt error\n");
// 			}
// 
// 			iRet = ioctl(g_chHedgwReal->sock, SIOCOUTQ, &value);
// 			if (iRet != 0)
// 			{
// 				printf("ioctl error\n");
// 			}
// 
// 			printf("totle:%d, value:%d\n", size, value);
// 			if (value + 1024 > size)
// 			{
// 				//goto end;
// 			}
// #endif
			break;
		}
		else if (nRes == SSL_ERROR_WANT_READ)
		{
			*fdStus = FDSTUS_WANT_READ;
			//printf("SSL_write number = %d,count = %d,wait to write ,sleep 10ms,and continue\n",res,count);
			break;
		}
		else
		{
			*fdStus = FDSTUS_ERROR;
//			printf("xxxxxxxxxxxxxxxxxxxSSL_write number error, %d\n",nRes);
			return res;
		}
	}

	return count;
}

int TlsHelper_ReadNonBlock( TlsHelperObj tlsHelp, void *buf,int num , FdStus *fdStus)
{
	int res, count = 0;
// 	int ntime = 0;

	if(tlsHelp == TlsHelperObjNull)
	{
		return -1;
	}

	STlsHelper* pHelper = (STlsHelper*)tlsHelp;
	SSL *ssl = pHelper->ssl;

	while(count < num)
	{
		res = SSL_read(ssl, buf, num - count);

		int nRes = SSL_get_error(ssl, res);
		if(nRes == SSL_ERROR_NONE)
		{
			if(res > 0)
			{
				count += res;
			}
		}
		else if (nRes == SSL_ERROR_WANT_READ)
		{
			*fdStus = FDSTUS_WANT_READ;
			break;
		}
		else if (nRes == SSL_ERROR_WANT_WRITE)
		{
			*fdStus = FDSTUS_WANT_WRITE;
			break;
		}
		else
		{
			*fdStus = FDSTUS_ERROR;
//			printf("ssl_read failed, res = %d, error res = %d",res,nRes);
			return res;
		}
	}

	return count;
}

void TlsHelper_HandShake(TlsHelperObj tlsHelp, int *isConnected, FdStus *fdStus)
{
	if (tlsHelp != TlsHelperObjNull)
	{
		STlsHelper* tmpTlsHelper = (STlsHelper*)tlsHelp;
		if (!tmpTlsHelper->sslConnected)
		{
			int r = SSL_do_handshake(tmpTlsHelper->ssl);
			if (r == 1) 
			{
				tmpTlsHelper->sslConnected = 1;
				*isConnected = 1;
				return;
			}
			int err = SSL_get_error(tmpTlsHelper->ssl, r);
			if (err == SSL_ERROR_WANT_WRITE) 
			{
				*fdStus = FDSTUS_WANT_WRITE;
			} 
			else if (err == SSL_ERROR_WANT_READ) 
			{
				*fdStus = FDSTUS_WANT_READ;
			}
			else 
			{
				*fdStus = FDSTUS_ERROR;
				ERR_print_errors(g_errBio);
			}
		}
	}
}