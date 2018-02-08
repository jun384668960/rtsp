#include "Player.h"
#include "record/Snapshot.h"

CPlayer::CPlayer()
  : m_nStatus(PS_Stopped), m_nSeekpos(-1), m_isFirst(1), m_bIsMute(0), m_position(0), m_duration(0), m_audioctx(0), m_videoctx(0), m_pVideoDecodeThread(0), m_pAudioDecodeThread(0), m_pBufferManager(0)
{
	memset(this, 0, sizeof(JNIStreamerContext));
	m_nBufferInterval = 100; //100ms
	m_hwmode = 1; //默认是软解
}

////////////////////////////////////////////////////////////////////////////////
void CPlayer::Doinit(JNIEnv *env, jobject weak)
{
	m_owner = env->NewGlobalRef(weak);
	m_pAudioTrack = new CAudioTrack();	
	m_pVideoDecodeThread = new CVideoDecodeThread(this);
	m_pAudioDecodeThread = new CAudioDecodeThread(this);	
}

void CPlayer::Uninit(JNIEnv *env)
{
	if( m_pHook[0] != 0 ) m_pHook[0]->Write(0); //通知结束
	if( m_pHook[1] != 0 ) m_pHook[1]->Write(0); //通知结束
	delete m_pAudioDecodeThread;
	delete m_pVideoDecodeThread;
	delete m_pAudioTrack;
	if( m_pSurfaceWindow != NULL) ANativeWindow_release(m_pSurfaceWindow);
	env->DeleteGlobalRef(m_owner);
}

////////////////////////////////////////////////////////////////////////////////
CBuffer *CPlayer::GetVideoSpspps(int idx, CBuffer *frame)
{
	frame->GetPacket()->duration = 1;
	switch(idx)
	{
		case 0:
		{
			 assert(m_videoSps.empty() == false);
			 frame->SetSize( m_videoSps.size());
			 memcpy(frame->GetPacket()->data, m_videoSps.c_str(), m_videoSps.size());
			 return frame;
		}
		case 1:
		{
			assert(m_videoPps.empty() == false);
			frame->SetSize( m_videoPps.size());
			memcpy(frame->GetPacket()->data, m_videoPps.c_str(), m_videoPps.size());
			return frame;
		}
		default:
		{
			frame->SetSize( m_videoSps.size() + m_videoPps.size());
			uint8_t *dst = frame->GetPacket()->data;
			memcpy(dst, m_videoSps.c_str(), m_videoSps.size()); dst += m_videoSps.size();
			memcpy(dst, m_videoPps.c_str(), m_videoPps.size());
			return frame;
		}
	}
}

int CPlayer::Snapshot(JNIEnv *env, jint w, jint h, char *file)
{
	if( m_pHook[1] != 0 ) return 1; //snapshot
	if( m_videoctx == 0 ) return 2;

	const char *n = strstr(file, "://");
	if( n != 0 ) file = n + 2; //skip file:/

	//refix w/h
	if( w <= 0 ) w = m_videoctx->width;
	if( h <= 0 ) h = m_videoctx->height;
	float a = (float)w / m_videoctx->width;
	float b = (float)h / m_videoctx->height;
	float r;
	if( a > b ) {
		r = b;
		w = r * m_videoctx->width;
	} else {
		r = a;
		if( a < b ) h = r * m_videoctx->height;		
	}

	int flags = SWS_FAST_BILINEAR; //fix flags
	if( r > 1.0 ) flags = SWS_AREA;
	else if( r < 1.0 ) flags = SWS_POINT;
	
	int t = 1; //only support jpg file

	int        codecid;
	CSwsScale *pScales;
	if( w != m_videoctx->width || h != m_videoctx->height )
	{//scale
		codecid = MEDIA_DATA_TYPE_YUV;
		pScales = new CSwsScale(m_videoctx->width, m_videoctx->height, m_videoctx->pix_fmt, w, h, t==0? AV_PIX_FMT_RGBA:AV_PIX_FMT_YUVJ420P, flags);
	}
	else
	{
		if( m_videoctx->codec_id == AV_CODEC_ID_MJPEG )
		{
			if( t == 0 )
			{
				codecid = MEDIA_DATA_TYPE_RGB;
				pScales = 0;
			}
			else
			{
				codecid = MEDIA_DATA_TYPE_JPG;
				pScales = 0;
			}
		}
		else
		{
			if( t == 0 )
			{
				codecid = MEDIA_DATA_TYPE_RGB;
				pScales = 0;
			}
			else
			{
				codecid = MEDIA_DATA_TYPE_YUV;
				pScales = new CSwsScale(m_videoctx->width, m_videoctx->height, m_videoctx->pix_fmt, w, h, AV_PIX_FMT_YUVJ420P, flags);			
			}
		}
	}

	memset(m_medias + 2 * MAX_MEDIA_FMT, 0, MAX_MEDIA_FMT * sizeof(m_medias[0])); //zero rec subcribe
	SetCodecidEnabled(2, codecid, true);
	m_pHook[1] = new CSnapshot(this, pScales, file, t);
	return 0;
}
