#ifndef __INCLUDED_TS_HEADER_H__
#define __INCLUDED_TS_HEADER_H__

#include "Def.h"

#ifdef _WIN32
#pragma pack(push,1) //设定为1字节对齐
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN

/************************************************************************/
/* little-endian                                                        */
/************************************************************************/

// TS header
typedef struct
{
	uint8_t sync;
	uint8_t pid_high5: 5,
			transport_priority: 1,
			unit_start: 1,
			transport_error: 1;
	uint8_t pid_low8;
	uint8_t counter: 4,
			field_control: 2,
			scrambling_control: 2;
	uint8_t field_len;
	uint8_t field_extension: 1,
			private_data: 1,
			splicing_point: 1,
			opcr: 1,
			pcr: 1,
			stream_priority: 1,
			random_access: 1,
			discontinuity: 1;
} BYTE_PACKED
TsHeader;

typedef struct{
	uint32_t pcr_base;
	uint16_t pcr_ext:9,
			 reserved:6,
			 pcr_basebit:1;
} BYTE_PACKED
PcrT;

#ifdef _WIN32
#pragma pack(pop) //恢复对齐状态
#endif

#elif __BYTE_ORDER == __BIG_ENDIAN

/************************************************************************/
/* big-endian                                                           */
/************************************************************************/

#error "unsupported big-endian"

#else
#error "Please fix <endian.h>"
#endif

#endif 
