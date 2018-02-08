#ifndef __FILERECORDTHREAD_H__
#define __FILERECORDTHREAD_H__

#include "MediaRecorder.h"
#include "Mp4file.h"

//文件录制线程: mp4文件格式，jpg/264/aac编码格式
///////////////////////////////////////////////////////////////////
class CFileRecordThread : public CMediaRecorder
{
public:
	CFileRecordThread(CPlayer *pPlayer, const char *file, int vcodecid, int acodecid)
	  : CMediaRecorder(pPlayer, vcodecid, acodecid)
	{
		THIS_LOGT_print("is created of %s", file);
		m_Mp4file    = new CMp4file(file, vcodecid, acodecid);
		m_pCodecs[2] = m_Mp4file->GetCodecContext(0);
		m_pCodecs[3] = m_Mp4file->GetCodecContext(1);
	}
	virtual ~CFileRecordThread()
	{
		delete m_Mp4file;
		THIS_LOGT_print("is deleted");
	}

protected: //interface of CMediaRecorder
	virtual int PrepareWrite();
	virtual int Write(int index, AVPacket *pPacket);

protected:
	CMp4file *m_Mp4file;
	CLASS_LOG_DECLARE(CFileRecordThread);	
};

#endif
