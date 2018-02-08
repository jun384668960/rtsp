#include "Snapshot.h"
#include "codec/MediaEncoder.h"
#include <unistd.h>

CLASS_LOG_IMPLEMENT(CSnapshot, "CSnapshot");
////////////////////////////////////////////////////////////////////////////////
int CSnapshot::Write(JNIEnv *env, void *pRawdata)
{
	CScopeBuffer pSrcdata((CBuffer*)pRawdata);
	if( pSrcdata.p )
	{
		THIS_LOGT_print("snap[%d]: data=%p, size=%d", pSrcdata->m_codecid, pSrcdata->GetPacket()->data, pSrcdata->GetPacket()->size);	
		AVCodec *vcodec = 0;
		switch(pSrcdata->m_codecid)
		{
			case MEDIA_DATA_TYPE_RGB:
			{
				 assert(m_pScales == 0);
				 assert(m_type==0);
				 break;
			}
			case MEDIA_DATA_TYPE_YUV:
			{
				 if( m_pScales ) pSrcdata.Attach(m_pScales->Scale(0, pSrcdata->GetPacket()->data));
				 if( m_type!=0 ) vcodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);			 
				 break;
			}
			case MEDIA_DATA_TYPE_JPG:
			{
				 assert(m_pScales == 0);
				 assert(m_type!=0);
				 break;
			}
			default:
			{
				 assert(false);
				 break;
			}
		}

		if( vcodec &&
			pSrcdata.p )
		{
			AVCodecContext *pCodecs = avcodec_alloc_context3(vcodec);
			pCodecs->time_base		= (AVRational){1,1};
			pCodecs->width   		= m_pScales->m_dstW;
			pCodecs->height  		= m_pScales->m_dstH;
			pCodecs->pix_fmt 		= AV_PIX_FMT_YUVJ420P;
			int iret = avcodec_open2(pCodecs, vcodec, 0);
			THIS_LOGT_print("call avcodec_open2 return %d", iret);
			CMediaEncoder *pEncode = CMediaEncoder::Create(pCodecs, m_pScales->m_dstFormat);
			pSrcdata.Attach(pEncode->Encode(pSrcdata.p));
			delete pEncode;
			avcodec_free_context(&pCodecs); //will call avcodec_close 	
		}

		int succ = 0; //mark succ/fail
		if( pSrcdata.p )
		{
			const FILE *f = fopen(m_file.c_str(), "w");
			if( f != 0 )
			{
				AVPacket *pSrcPacket = pSrcdata->GetPacket();
				int iret = fwrite(pSrcPacket->data, 1, pSrcPacket->size, f);
				fclose(f);
				if( pSrcPacket->size != iret )
				{
					unlink(m_file.c_str());	
				}
				else
				{
					succ = 1;
				}
			}
		}

		Java_fireNotify(env, m_pParams, succ? MEDIA_SHOT_COMPLETED:MEDIA_ERR_SHOT, 0, 0, 0);
	}	

	delete this; //free
	return 0;
}
