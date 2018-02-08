#ifndef __INCLUDE_TS_PARSER_H__
#define __INCLUDE_TS_PARSER_H__

#include "TsHeader.h"

class CTsParser
{
public:
	CTsParser();
	~CTsParser();
public:
	static int GetPcr( const uint8_t* data, uint64_t& pcr );
	static bool IsUnitStart( const uint8_t* data );
};

#endif
