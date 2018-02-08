#include "HwCodec.h"
#include <sys/system_properties.h>
#include <dlfcn.h> 
#include <errno.h>

#ifdef _DEBUG //实现写日志文件接口
#ifdef _LOGFILE
extern FILE  *__logfp; //在com_bql_MediaPlayer_Native.cpp里定义/初始化, 在JNI_load.cpp里释放
extern CMutex __logmutex;
#endif
#endif

CLASS_LOG_IMPLEMENT(CHwCodec, "CHwCodec[bql]");
////////////////////////////////////////////////////////////////////////////////
CHwCodec::CHwCodec(const char *path)
  : m_dll(0), m_videoDecoderfunc(0), m_audioDecoderfunc(0), m_videoEncoderfunc(0), m_audioEncoderfunc(0)
{
	char szver[126], model[126], board[126] = {0}, szmtk[126];
	__system_property_get("ro.build.version.release", szver);
	__system_property_get("ro.product.model", model);				
	__system_property_get("ro.product.board", board);				
	__system_property_get("ro.mediatek.platform", szmtk);

	THIS_LOGT_print("is created, android: %s, model: %s, board: %s builded on %s %s", szver, model, board, __DATE__, __TIME__);
	int version = atoi(szver);
	if( version > 4 )
	{
		m_dll = dlopen(path, RTLD_NOW);	
		if( m_dll == 0 )
		{
			THIS_LOGW_print("load bql_hwcodec error: %d", errno);
		}
		else
		{
			#ifdef _LOGFILE
			typedef int (*HwInitfunc)(char*, char*, char*, FILE *, void *);
			#else
			typedef int (*HwInitfunc)(char*, char*, char*);
			#endif
			HwInitfunc Initfunc = (HwInitfunc)dlsym(m_dll, "Init");
			THIS_LOGT_print("get bql_hwcodec.Init: %p from so: %p", Initfunc, m_dll);
			if( Initfunc ) {
				int iret = Initfunc(
						   model, board, szmtk
						   #ifdef _LOGFILE
						   , __logfp, __logmutex.Out()
						   #endif
						   );
				THIS_LOGI_print("call bql_hwcodec.Init(%s, %s) return %d", model, board, iret);
				if( iret >= 0 )
				{//init succ
					m_videoDecoderfunc = (CreateDecoderfunc)dlsym(m_dll, "GetVideoDecoderClass");
					m_audioDecoderfunc = (CreateDecoderfunc)dlsym(m_dll, "GetAudioDecoderClass");
					THIS_LOGI_print("get CreateAacDecoder: %p/CreateH264Decoder: %p", m_audioDecoderfunc, m_videoDecoderfunc);			
					m_videoEncoderfunc = (CreateEncoderfunc)dlsym(m_dll, "GetVideoEncoderClass");
			        m_audioEncoderfunc = (CreateEncoderfunc)dlsym(m_dll, "GetAudioEncoderClass");			
					THIS_LOGI_print("get CreateAacEncoder: %p/CreateH264Encoder: %p", m_audioEncoderfunc, m_videoEncoderfunc);
				}
			}
		}
	}
}

CHwCodec::~CHwCodec()
{
	if( m_dll != NULL ) dlclose(m_dll);
	THIS_LOGT_print("is deleted");	
}

CHwMediaDecoder* CHwCodec::CreateH264Decoder(StreamDecodedataParameter *Parameter)
{
	if( m_videoDecoderfunc == NULL )
	{
		return NULL;
	}
	else
	{
		return m_videoDecoderfunc(Parameter);
	}
}

CHwMediaDecoder* CHwCodec::CreateAacDecoder(StreamDecodedataParameter *Parameter)
{
	if( m_audioDecoderfunc == NULL )
	{
		return NULL;
	}
	else
	{
		return m_audioDecoderfunc(Parameter);
	}
}

CHwMediaEncoder* CHwCodec::CreateH264Encoder(StreamEncodedataParameter *Parameter)
{
	if( m_videoEncoderfunc == NULL )
	{
		return NULL;
	}
	else
	{
		return m_videoEncoderfunc(Parameter);
	}
}

CHwMediaEncoder* CHwCodec::CreateAacEncoder(StreamEncodedataParameter *Parameter)
{
	if( m_audioEncoderfunc == NULL )
	{
		return NULL;
	}
	else
	{
		return m_audioEncoderfunc(Parameter);
	}
}
