#ifndef __com_bql_MSG_Native_H__
#define __com_bql_MSG_Native_H__

#include <android/native_window_jni.h> //ANativeWindow
#include "common.h"

enum NATIVE_POST_MESSAGE 
{
	MEDIA_STATUS               = 0,
	MEDIA_PREPARED,
	MEDIA_SEEK_COMPLETED,
	MEDIA_PLAY_COMPLETED,
	MEDIA_SHOT_COMPLETED,
	MEDIA_SHOWFIRSTFRAME,

	MEDIA_DATA                 = 100,
	MEDIA_DATA_TYPE_264,
	MEDIA_DATA_TYPE_AAC,
	MEDIA_DATA_TYPE_PCM,
	MEDIA_DATA_TYPE_RGB,
	MEDIA_DATA_TYPE_JPG, //只能用于录制.Write的参数
	MEDIA_DATA_TYPE_YUV,

	MEDIA_INFO                 = 400,
	MEDIA_INFO_PAUSE_COMPLETED,

	MEDIA_ERR                  = 500,
	MEDIA_ERR_SEEK,
	MEDIA_ERR_PREPARE,
	MEDIA_ERR_PAUSE,
	MEDIA_ERR_PLAY,
	MEDIA_ERR_SHOT,
	MEDIA_ERR_NO_STREAM,
};

class CAudioTrack;
class CWriter
{
public:
	virtual int Write(JNIEnv *env, void *buffer = 0) = 0; //注意: 不管write成功与否，Buffer都将交给CWriter管理
};

#define MAX_MEDIA_FMT (6) //限制最多设置8个媒体数据格式
struct JNIStreamerContext
{
public:
	ANativeWindow *m_pSurfaceWindow;
	CAudioTrack   *m_pAudioTrack; //用于播放音频  由Native层自己创建 自己释放 @ CAudioDecodeThread::Loop  continueAfterSETUP	
	jobject  m_owner; //是Java层 BQLMediaPlayer对象的弱引用  用于下面method:postEventFromNative回调到上层	
	CWriter *m_pHook[2]; //录制/截图处理
	int m_medias[MAX_MEDIA_FMT + MAX_MEDIA_FMT + MAX_MEDIA_FMT]; //应用所关注媒体信息格式列表
	int m_hwmode;//0-auto 1-software 2-hardware 3-hardware

	int HasCodecid(int s, int codecid)
	{
		if( s ) s *= MAX_MEDIA_FMT; //refix s
		int i = codecid - MEDIA_DATA- 1 + s;
		if( m_medias[i] != 0 ) return i + 1;
		return 0;
	}
	int SetCodecidEnabled(int s, int codecid, bool onoff)
	{
		if( s ) s *= MAX_MEDIA_FMT; //refix s	
		int i = codecid - MEDIA_DATA- 1 + s;
		m_medias[i] = onoff? 1:0;
		return i + 1;
	}
};

struct JNIMediaPlayer
{
	jclass thizClass;
	jmethodID post;
};

struct JNIBuffer
{
	jclass thizClass;
	jmethodID constructor;
};

struct JNIMediaRecorder
{
	jclass thizClass;
	jmethodID post;	
};

struct JNIAudioTrack
{
	jclass thizClass;
	jmethodID constructor;
	jmethodID getMinBufferSize;
	jmethodID setVolume;
	jmethodID play;
	jmethodID write;
	jmethodID stop;
	jmethodID release;
};

extern bool Java_drawNativeWindow(ANativeWindow *pSurfaceWindows, unsigned char* pBuf, int width, int height);
extern void Java_fireNotify(JNIEnv *env, JNIStreamerContext *pParams, int type, int  arg1, int arg2, void *obj);
extern void Java_fireBuffer(JNIEnv *env, JNIStreamerContext *pParams, int type, void *obj/*CBuffer*/, int val = 0);
extern int  GetCodecID(int id);

#endif /* __com_bql_MSG_Native_H__ */
