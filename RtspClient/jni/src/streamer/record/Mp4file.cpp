#include "Mp4file.h"

CLASS_LOG_IMPLEMENT(CMp4file, "Cfile[mp4]");
////////////////////////////////////////////////////////////////////////////////
CMp4file::CMp4file(const char *path, int vcodecid, int acodecid)
  : m_fmtctx(0), m_ref(-1)
{
	THIS_LOGT_print("is created of %s, vcodecid: %d, acodecid: %d", path, vcodecid, acodecid);
	memset(m_stream, 0, sizeof(m_stream));
	int ret = avformat_alloc_output_context2(&m_fmtctx, NULL, NULL, path);
	if( ret < 0 ) ret = avformat_alloc_output_context2(&m_fmtctx, NULL, "mpeg", path);
	if( ret < 0 )
	{
		#ifdef _DEBUG
		char buf[128];
	    av_strerror(ret, buf, 128);
		THIS_LOGE_print("call avformat_alloc_output_context2 return %d, error: %s", ret, buf);
		#endif
	}
	else
	{
		int ret = avio_open(&m_fmtctx->pb, path, AVIO_FLAG_WRITE);	
	    if( ret < 0 )
		{
			THIS_LOGE_print("call avio_open return %d", ret);
		}
		else
		{
			if( vcodecid ) m_stream[0] = avformat_new_stream(m_fmtctx, avcodec_find_encoder(vcodecid));
			if( acodecid ) m_stream[1] = avformat_new_stream(m_fmtctx, avcodec_find_encoder(acodecid));
			if( m_fmtctx->oformat->flags | AVFMT_GLOBALHEADER ) {
			if( m_stream[0] != 0 ) m_stream[0]->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
			if( m_stream[1] != 0 ) m_stream[1]->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
			}
		}
	}
}

CMp4file::~CMp4file()
{
	if( m_fmtctx != NULL ) {
		if( m_fmtctx->pb ) {
			int iret = av_write_trailer(m_fmtctx);
			THIS_LOGI_print("call av_write_trailer return %d", iret);
			avio_closep(&m_fmtctx->pb );
		}
	    avformat_free_context(m_fmtctx);
	}
	THIS_LOGT_print("is deleted");
}

int CMp4file::Start()
{
	if( m_fmtctx == NULL ) 
	{
		return 1;
	}

	AVCodecContext *pVideoctx = GetCodecContext(0);
	if( pVideoctx ) {
	    m_stream[0]->time_base = pVideoctx->time_base;
		#ifdef _FFMPEG_THREAD
		pVideoctx->thread_count = 4;
		pVideoctx->thread_type =FF_THREAD_FRAME;
		#endif 
		int iret = avcodec_open2(pVideoctx, pVideoctx->codec, NULL);
		if( iret < 0 ) {
		#ifdef _DEBUG
		char buf[128];
	    av_strerror(iret, buf, 128);
		THIS_LOGE_print("call avcodec_open2 return %d, error: %s", iret, buf);
		#endif
		}
	}
	AVCodecContext *pAudioctx = GetCodecContext(1);
	if( pAudioctx ) {
	    m_stream[1]->time_base = pAudioctx->time_base;
		int iret = avcodec_open2(pAudioctx, pAudioctx->codec, NULL);
		if( iret < 0 ) {
		#ifdef _DEBUG
		char buf[128];
	    av_strerror(iret, buf, 128);
		THIS_LOGE_print("call avcodec_open2 return %d, error: %s", iret, buf);
		#endif
		}
	}

	int iret = avformat_write_header(m_fmtctx, 0);
	THIS_LOGI_print("call avformat_write_header return %d", iret);
	return 0;
}

int CMp4file::Write(AVMediaType type, AVPacket *pPacket)
{
	AVStream *pStream = NULL;
	if( type == AVMEDIA_TYPE_VIDEO ) pStream = m_stream[0];
	if( type == AVMEDIA_TYPE_AUDIO ) pStream = m_stream[1];

	if( pStream == 0 )
	{
		return -1;
	}
	else
	{
		pPacket->stream_index = pStream->index;
		pPacket->pts = Get(pPacket->pts) * pStream->time_base.den / pStream->time_base.num / 1000;
		pPacket->dts = Get(pPacket->dts) * pStream->time_base.den / pStream->time_base.num / 1000;
		THIS_LOGD_print("write[%d]: pts=%lld/%lld, data=%p, size=%d, duration=%lld", pPacket->stream_index, pPacket->pts, pPacket->dts, pPacket->data, pPacket->size, pPacket->duration);
		int iret = av_interleaved_write_frame(m_fmtctx, pPacket);
//		THIS_LOGD_print("call av_interleaved_write_frame return %d", iret);
		return 0;
	}
}
