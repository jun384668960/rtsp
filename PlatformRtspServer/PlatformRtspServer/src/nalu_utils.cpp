#include <stdlib.h>
#include "nalu_utils.hh"

int find_start_code3(unsigned char* data, int lenght)
{
	if(data == NULL || lenght < 4) return -2;

	if(data[0] == 0x0 && data[1] == 0x0 && data[2] == 0x0 && data[3] == 0x01)
		return 0;
	else
		return -1;
}
int get_annexb_nalu(unsigned char* data, int lenght)
{
	int pos;
	int info2 = 1;
	
	int info1 = find_start_code3(data, lenght);
	if(info1 == 0)	//开始部分有0x00000001
	{
		pos = 4;
	}
	else
	{
		return 0;
	}
//	LOGD_print("info1=%d,lenght:%d",info1,lenght);
	while(pos<lenght)
	{
		info2 = find_start_code3(data + pos, lenght - pos);
		if(info2 == -2 || info2 == 0) 
			break;
		pos++;	
//		LOGD_print("info2=%d,pos:%d",info2,pos);
	}

	if(info2 == 0)
	{
		return pos;
	}
	else
	{
		return lenght;
	}
}


