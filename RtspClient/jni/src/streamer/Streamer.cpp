#include "Streamer.h"
#include "player/rtsp/RtspClientPlayer.h"
#include "player/rtmp/RtmpClientPlayer.h"
#include "player/local/FfmpegPlayer.h"
#include "record/FileRecordThread.h"
#include "record/RtmpRecordThread.h"
#include "codec/HwCodec.h"
#include <string.h>

CBufferManager *g_pBufferManager = 0;

LOCAL_LOG_IMPLEMENT(s_logger, "CStreamer");
////////////////////////////////////////////////////////////////////////////////
CPlayer *CStreamer::CreateMyPlayer(JNIEnv *env, jobject obj, const char *uri)
{
	assert(obj != 0 && uri != 0);
	const char *p = ::strstr(uri,"://");
	if( p == 0 )
	{
		LOGE_print(s_logger, "CreatePlayer error: fail to parse uri"); 
		return NULL;
	}

	if( p - uri == 4 &&
		::memcmp(uri, "rtsp", 4) == 0 )
	{//网络播放,rtsp://
		CPlayer *pObject = new CRtspClientPlayer(uri);
		pObject->Doinit(env, obj);
		return pObject;
	}

	if( p - uri == 4 &&
		::memcmp(uri, "rtmp", 4) == 0 )
	{//网络播放,rtmp://
		CPlayer *pObject = new CRtmpClientPlayer(uri);
		pObject->Doinit(env, obj);
		return pObject;
	}

	if( p - uri == 4 &&
		::memcmp(uri, "file", 4) == 0 )
	{//本地播放,file://
		CPlayer *pObject = new CFfmpegPlayer(uri + 6/*skip file:/ */ );
		pObject->Doinit(env, obj);
		return pObject;
	}

	LOGW_print(s_logger, "CreatePlayer error: uri is not support"); 
	return NULL;
}

void CStreamer::DeleteMyPlayer(JNIEnv *env, CPlayer *pObject)
{
	pObject->Stop(); //muset stop first
	pObject->Uninit(env);
	delete pObject;
}

CMediaRecorder *CStreamer::CreateRecorder(JNIEnv *env, jobject obj, CPlayer *pPlayer, const char *uri, int vcodecid, int acodecid)
{
	assert(vcodecid != 0 || acodecid != 0);
	assert(uri != 0);

	if( pPlayer &&
		pPlayer->m_pHook[0]!= 0)
	{
		LOGW_print(s_logger, "CreateRecorder error: hook[%p] has set", pPlayer->m_pHook[0]);
		return NULL;
	}

	const char *p = ::strstr(uri,"://");
	if( p - uri == 4 &&
		::memcmp(uri, "file", 4) == 0 )
	{//本地录制,file://
		CMediaRecorder *pObject = new CFileRecordThread(pPlayer, uri + 6/*skip file:/ */, vcodecid, acodecid);	
		pObject->Doinit(env, obj);
		return pObject;
	}

	if( p - uri == 4 &&
		::memcmp(uri, "rtmp", 4) == 0 )
	{//本地录制,rtmp://
		CMediaRecorder *pObject = new CRtmpRecordThread(pPlayer, uri, vcodecid, acodecid);	
		pObject->Doinit(env, obj);
		return pObject;
	}
	else
	{
		LOGW_print(s_logger, "CreateRecorder error: uri is not support");
		return NULL;
	}
}

void CStreamer::DeleteRecorder(JNIEnv *env, CMediaRecorder *pObject)
{
	pObject->Stop(); //muset stop first
	pObject->Uninit(env);	
	delete pObject;
}

CBuffer *CStreamer::GetThumbnail(char *file, int ipos, int ifmt, int &t, int &w, int &h)
{
	AVFormatContext *avfmtctx = 0;
	AVCodecContext  *videoctx = 0;
	int iret = 0, ivid =-1;
	CScopeBuffer  pDstdata;

	COST_STAT_DECLARE(cost);
	const char *n = strstr(file, "://");
	if( n != 0 ) file = n + 2; //skip file:/

	do{
	iret = avformat_open_input(&avfmtctx, file, NULL, NULL);
	if( iret < 0 ) {
		LOGE_print(s_logger, "call avformat_open_input return %d", iret);
		break;
	}

	iret = avformat_find_stream_info(avfmtctx, 0);
	if( iret < 0 ) {
		LOGE_print(s_logger, "call avformat_find_stream_info return %d", iret);
		break;
	}

	for(int i = 0; i < avfmtctx->nb_streams; i++) 
	{
		if( avfmtctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO ) 
		{
			ivid = i;
			break;
		}
	}
	if( ivid < 0 ) break;

	videoctx = avfmtctx->streams[ivid]->codec; 
	#ifdef _FFMPEG_THREAD
	videoctx->thread_count = 4;
	videoctx->thread_type = FF_THREAD_SLICE;	
	#endif
	iret = avcodec_open2(videoctx, avcodec_find_decoder(videoctx->codec_id), NULL);
	if( iret < 0 ) {
		LOGE_print(s_logger, "call avformat_find_stream_info return %d", iret);
		break;
	}

	if( w <= 0 ) w = videoctx->width;
	if( h <= 0 ) h = videoctx->height;

	if( ipos > 0 ) {
	const AVStream *pStream = avfmtctx->streams[ivid];
	if( ipos > avfmtctx->duration / 1000 ) ipos = avfmtctx->duration / 1000;
	const int64_t skip = av_rescale(ipos, pStream->time_base.den, pStream->time_base.num);
	iret = avformat_seek_file(avfmtctx, ivid, 0, skip/1000, INT64_MAX, AVSEEK_FLAG_FRAME);
	if( iret < 0 ) {
		LOGE_print(s_logger, "call avformat_seek_file return %d", iret);
		break;
	}
	}

	CScopeBuffer pSrcdata(g_pBufferManager->Pop());	
	AVPacket    *pPacket = pSrcdata->GetPacket();

	do{
	iret = av_read_frame(avfmtctx, pPacket);
	if( iret < 0 ) {
		LOGE_print(s_logger, "call av_read_frame return %d", iret);
		pSrcdata.Release();
		break;
	}
	}while(pPacket->stream_index != ivid);

	if( pSrcdata.p == 0 ) {
		break;
	}

	pPacket->pts = pPacket->pts * 1000 * av_q2d(avfmtctx->streams[pPacket->stream_index]->time_base);
//	pPacket->dts = pPacket->dts * 1000 * av_q2d(avfmtctx->streams[pPacket->stream_index]->time_base);

	CMediaDecoder *pDecode = CMediaDecoder::Create(videoctx, false);		
	pDstdata.Attach(pDecode->Decode(pSrcdata.p));
	delete pDecode;
	if( pDstdata.p == 0 ) {
		break;
	}

	//refix w/h
	float a = (float)w / videoctx->width;
	float b = (float)h / videoctx->height;
	float r;
	if( a > b ) { 
		r = b; 
		w = r * videoctx->width;
	} else {
		r = a;
		if( a < b ) h = r * videoctx->height;		
	}

	int flags = SWS_FAST_BILINEAR; //fix flags
	if( r > 1.0 ) flags = SWS_AREA;
	else if( r < 1.0 ) flags = SWS_POINT;

	if( ifmt == MEDIA_DATA_TYPE_YUV ) {//build yuv
	t = videoctx->pix_fmt;
	if( videoctx->width != w || videoctx->height != h ) {
	CSwsScale scale(videoctx->width, videoctx->height, videoctx->pix_fmt, w, h, videoctx->pix_fmt  , flags);
	pDstdata.Attach(scale.Scale(pDstdata->GetPacket()->pts, pDstdata->GetPacket()->data));	
	}
	}
	if( ifmt == MEDIA_DATA_TYPE_RGB ) {//build rgb
	CSwsScale scale(videoctx->width, videoctx->height, videoctx->pix_fmt, w, h, AV_PIX_FMT_RGBA    , flags);
	pDstdata.Attach(scale.Scale(pDstdata->GetPacket()->pts, pDstdata->GetPacket()->data));
	}
	if( ifmt == MEDIA_DATA_TYPE_JPG ) {//build jpg
	CSwsScale scale(videoctx->width, videoctx->height, videoctx->pix_fmt, w, h, AV_PIX_FMT_YUVJ420P, flags);
	pDstdata.Attach(scale.Scale(pDstdata->GetPacket()->pts, pDstdata->GetPacket()->data));	
	if( pDstdata.p == 0 ) {
		break;
	}
	AVCodec *vcodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
	AVCodecContext *pCodecs = avcodec_alloc_context3(vcodec);
	pCodecs->time_base		= (AVRational){1,1};
	pCodecs->width			= w;
	pCodecs->height 		= h;
	pCodecs->pix_fmt		= AV_PIX_FMT_YUVJ420P;
	iret = avcodec_open2(pCodecs, vcodec, 0);
	LOGD_print(s_logger, "call avcodec_open2 return %d", iret);
	CMediaEncoder *pEncode = CMediaEncoder::Create(pCodecs, AV_PIX_FMT_YUVJ420P);
	pDstdata.Attach(pEncode->Encode(pDstdata.p));
	delete pEncode;
	avcodec_free_context(&pCodecs); //will call avcodec_close	
	}
	}while(0);

	if( avfmtctx ) {
	if( videoctx ) avcodec_close( videoctx );
	avformat_close_input(&avfmtctx );
	}

	return pDstdata.Detach();
}
