#include "HwAacDecoder.h"

CLASS_LOG_IMPLEMENT(CHwAacDecoder, "CDecoder[aac.hw]");
////////////////////////////////////////////////////////////////////////////////
CHwAacDecoder::CHwAacDecoder(AVCodecContext *audioctx, CHwMediaDecoder *pDecode)
  : CMediaDecoder(audioctx), m_pDecode(pDecode)
{
	THIS_LOGT_print("is created");
	assert(m_pDecode != 0);	
}

CHwAacDecoder::~CHwAacDecoder()
{
	delete m_pDecode;
	THIS_LOGT_print("is deleted");
}

CBuffer *CHwAacDecoder::Decode(CBuffer *pRawdata)
{//aac->pcm
	if( m_pBufferManager== 0 ) m_pBufferManager = new CBufferManager(m_pCodecs->frame_size * m_pCodecs->channels * sizeof(short));
	AVPacket *pSrcPacket = pRawdata->GetPacket();
	THIS_LOGT_print("do decode[%p]: pts=%lld, data=%p, size=%d", pRawdata, pSrcPacket->pts, pSrcPacket->data, pSrcPacket->size);				
	CScopeBuffer pDstdata(m_pBufferManager->Pop(pSrcPacket->pts));
	AVPacket *pDstPacket = pDstdata->GetPacket();

	if( pSrcPacket->data[0] == 0xff ) pRawdata->Skip(7); //skip adts head
	pDstPacket->size = m_pDecode->Decode(pSrcPacket->data, pSrcPacket->size, pSrcPacket->pts, pDstPacket->data);
	if( pDstPacket->size > 0 )
		return pDstdata.Detach();
	else
		return NULL;
}
