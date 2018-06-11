#include <stdlib.h>
#include "nalu_utils.hh"

int find_start_code2 (unsigned char *data)
{
	if(data[0]!=0 || data[1]!=0 || data[2] !=1) return 0;              //Check whether buf is 0x000001
	else return 1;
}

int find_start_code3 (unsigned char *data)
{
	if(data[0]!=0 || data[1]!=0 || data[2] !=0 || data[3] !=1) return 0;//Check whether buf is 0x00000001
	else return 1;
}

int get_annexb_nalu(unsigned char *frame, int length, NALU_t *nalu)
{
	int info2, info3;
	int pos = 0;
	int StartCodeFound, rewind;

	nalu->startcodeprefix_len=3;      //Initialize the bits prefix as 3 bytes

	info2 = find_start_code2 (frame);    //Check whether Buf is 0x000001
	if(info2 != 1) 
	{
		info3 = find_start_code3 (frame);//Check whether Buf is 0x00000001
		if (info3 != 1)              //If not the return -1
		{ 
			return -1;
		}
		else 
		{
			//If Buf is 0x00000001,set the prefix length to 4 bytes
			pos = 4;
			nalu->startcodeprefix_len = 4;
		}
	} 
	else
	{
		//If Buf is 0x000001,set the prefix length to 3 bytes
		pos = 3;
		nalu->startcodeprefix_len = 3;
	}
	//Search for next character sign bit
	StartCodeFound = 0;
	info2 = 0;
	info3 = 0;
	
	while (!StartCodeFound)
	{
		if (pos > length)
		{
			nalu->len = (pos-1)-nalu->startcodeprefix_len;
			//memcpy (nalu->buf, &frame[nalu->startcodeprefix_len], nalu->len);     
			nalu->buf = &frame[nalu->startcodeprefix_len];
			nalu->forbidden_bit = nalu->buf[0] & 0x80;     // 1 bit--10000000
			nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit--01100000
			nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;   // 5 bit--00011111
			return pos-1;
		}
		pos++;
		info3 = find_start_code3(&frame[pos-4]);		     //Check whether Buf is 0x00000001
		if(info3 != 1)
			info2 = find_start_code2(&frame[pos-3]);           //Check whether Buf is 0x000001
		StartCodeFound = (info2 == 1 || info3 == 1);
	}

	// Here, we have found another start code (and read length of startcode bytes more than we should
	// have.  Hence, go back in the file
	rewind = (info3 == 1)? -4 : -3;

	// Here the Start code, the complete NALU, and the next start code is in the Buf.  
	// The size of Buf is pos, pos+rewind are the number of bytes excluding the next
	// start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code
	nalu->len = (pos+rewind) - nalu->startcodeprefix_len;
	//memcpy (nalu->buf, &frame[nalu->startcodeprefix_len], nalu->len);//Copy one complete NALU excluding start prefix 0x000001 and 0x00000001
	nalu->buf = &frame[nalu->startcodeprefix_len];
	nalu->forbidden_bit = nalu->buf[0] & 0x80;                     //1 bit
	nalu->nal_reference_idc = nalu->buf[0] & 0x60;                 // 2 bit
	nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;                   // 5 bit

	return (pos+rewind);                                           //Return the length of bytes from between one NALU and the next NALU

}
