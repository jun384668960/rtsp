#include "player/Player.h"
#include "FileRecordThread.h"

CLASS_LOG_IMPLEMENT(CFileRecordThread, "CRecordThread[file]");
///////////////////////////////////////////////////////////////////
int CFileRecordThread::PrepareWrite()
{
	m_Mp4file->Start();
	return CMediaRecorder::PrepareWrite();
}

int CFileRecordThread::Write(int index, AVPacket *pPacket)
{
	if( m_ref < 0 ) m_ref = pPacket->pts;
	if( pPacket->pts > m_now ) m_now = pPacket->pts;

	m_Mp4file->Write(index==0? AVMEDIA_TYPE_VIDEO:AVMEDIA_TYPE_AUDIO, pPacket);

	m_nframes[index] ++;
	return 0;
}
