#ifndef __INCLUDE_TS_FILE_READER_H__
#define __INCLUDE_TS_FILE_READER_H__

#include "Def.h"
#include <stdio.h>
#include "TsParser.h"

class CTsFileReader
{
public:
	CTsFileReader();
	~CTsFileReader();
public:
	int Init( const char* filename );
	int GetFileRange();
	bool SeekByTime( uint64_t sec );
	int GetTsPkt( char* buf, int len, uint64_t& pcr );
private:
	int init_file();
private:
	FILE* m_file;
	char m_data_buf[TS_PKT_LEN];
	uint64_t m_file_size;
	uint64_t m_cur_pcr;
	uint64_t m_first_pcr;
	uint64_t m_last_pcr;
};


#endif
