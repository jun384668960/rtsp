#ifndef MP4_PARSER_H
#define MP4_PARSER_H
#include "mp4v2/mp4v2.h"

class CMP4Parser
{
public:
	CMP4Parser();
	virtual ~CMP4Parser();

	int GetTrackCount();
	int GetTrackFormat(int idx);
	int SelectTrack(int type);
	bool SetDataSource(char* file);
	bool UnselectTrack(int idx);
	bool ReadSampleData(unsigned char* data, int& length, uint64_t& pts);
	bool SeekTo();

	void  ParserVideoParams();
	void  ParserAudioParams();
	float GetFrameRate();
	bool GetSpsAndPps(u_int8_t* &sps, uint32_t& spsLen, u_int8_t* &pps,uint32_t& ppsLen);
	uint8_t GetAudioProfile(){return m_profile;}
	uint8_t GetAudioFreIdx(){return m_freIdx;}
	uint8_t GetAudioChannles(){return m_channles;}
protected:

	
private:
	MP4FileHandle m_File;
	uint32_t m_TrackId;
	float m_FrameRate;
	uint32_t m_CurSampleId;
	uint32_t m_NumSamples;

	uint8_t* m_Sps;
	uint8_t* m_Pps;
	uint32_t m_SpsLen;
	uint32_t m_PpsLen;
	uint8_t m_profile;
	uint8_t m_channles;
	uint8_t	m_freIdx;
		
};

#endif
