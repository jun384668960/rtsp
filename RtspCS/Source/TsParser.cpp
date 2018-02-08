#include "TSParser.h"
#include "PrintLog.h"

CTsParser::CTsParser()
{

}

CTsParser::~CTsParser()
{

}

int CTsParser::GetPcr( const uint8_t* data, uint64_t& pcr )
{
	TsHeader* t_h = (TsHeader*)data;
	if( t_h->sync != 0x47 || t_h->field_control < 2 
		|| t_h->field_len < 1 || t_h->pcr == 0 )
		return -1;
	PcrT* pcr_info = (PcrT*)(data+sizeof(TsHeader));
	pcr = ntohl(pcr_info->pcr_base);
	pcr = pcr << 1 | pcr_info->pcr_basebit;
	pcr = pcr * 300 + pcr_info->pcr_ext;
	pcr /= 300;
	//LogInfo( "pcr:%llu\n", pcr );
	return 0;
}


bool CTsParser::IsUnitStart( const uint8_t* data )
{
	TsHeader* t_h = (TsHeader*)data;
	return t_h->sync == 0x47 && t_h->unit_start == 1 ? true : false;
}

