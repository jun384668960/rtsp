#ifndef __STREAMER_H__
#define __STREAMER_H__

#include <string>
#include "player/Player.h"
#include "record/MediaRecorder.h"

//CStreamer对象用于管理整个播放/录制过程的对象
////////////////////////////////////////////////////////////////////////////////
class CStreamer
{
public: //对外接口, 返回值=0: 表示成功，其他值表示失败
	static CPlayer        *CreateMyPlayer(JNIEnv *env, jobject obj, const char *uri);
	static void            DeleteMyPlayer(JNIEnv *env, CPlayer *pPlayer);

	static CMediaRecorder *CreateRecorder(JNIEnv *env, jobject obj, CPlayer *pPlayer, const char *uri, int vcodecid, int acodecid);
	static void 	       DeleteRecorder(JNIEnv *env, CMediaRecorder *pRecorder);

	static CBuffer  *GetThumbnail(char *uri, int pos, int fmt, int &t, int &w, int &h);
};

extern CBufferManager *g_pBufferManager;

#endif
