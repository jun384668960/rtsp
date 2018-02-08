#include "H264Handle.h"
#include <stdlib.h>

static char startBit[4] = {0x00, 0x00, 0x00, 0x01};

CH264Handle::CH264Handle(char* pFile)
{
	info2 = 0;
	info3 = 0;
	m_bitStream = NULL;

	memset(m_Sps, 0x0, 64);
	m_SpsLen = 0;
	memset(m_Pps, 0x0, 64);
	m_PpsLen = 0;

	OpenBitstreamFile(pFile);
}


CH264Handle::~CH264Handle(void)
{
	fclose(m_bitStream);
	m_bitStream = NULL;
}

bool CH264Handle::BitStreamFeof()
{
	return feof(m_bitStream);
}

int CH264Handle::GetAnnexbFrame (unsigned char *frame, int length)
{
	int size = 0;
	NALU_t* n = CH264Handle::AllocNALU(length); 
	
	do{
		int len = CH264Handle::GetAnnexbNALU (n);
		if(len > 0)
		{
			memcpy(frame + size, startBit, 4);
			size += 4;
			memcpy(frame + size, n->buf, n->len);
			size += n->len;
		}
		else
		{
			fprintf(stderr,"GetAnnexbNALU return %d", len);
			break;
		}
	}while(n->nal_unit_type != 1 && n->nal_unit_type != 5 
			&& n->nal_unit_type != 8 && n->nal_unit_type != 7);

	CH264Handle::FreeNALU(n); 

	return size;
}

int CH264Handle::GetAnnexbNALU (unsigned char *frame, int &length, NALU_t *nalu)
{
	int pos = 0;
	int StartCodeFound, rewind;

	nalu->startcodeprefix_len=3;      //Initialize the bits prefix as 3 bytes

	info2 = FindStartCode2 (frame);    //Check whether Buf is 0x000001
	if(info2 != 1) 
	{
		info3 = FindStartCode3 (frame);//Check whether Buf is 0x00000001
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
	
	if(frame[pos] == 0x27)
	{
		printf("Prefix length : %d.\n",nalu->startcodeprefix_len);
	}
	while (!StartCodeFound)
	{
		if (pos > length)
		{
			nalu->len = (pos-1)-nalu->startcodeprefix_len;
			memcpy (nalu->buf, &frame[nalu->startcodeprefix_len], nalu->len);     
			nalu->forbidden_bit = nalu->buf[0] & 0x80;     // 1 bit--10000000
			nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit--01100000
			nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;   // 5 bit--00011111
			return pos-1;
		}
		pos++;
		info3 = FindStartCode3(&frame[pos-4]);		     //Check whether Buf is 0x00000001
		if(info3 != 1)
			info2 = FindStartCode2(&frame[pos-3]);           //Check whether Buf is 0x000001
		StartCodeFound = (info2 == 1 || info3 == 1);
	}

	// Here, we have found another start code (and read length of startcode bytes more than we should
	// have.  Hence, go back in the file
	rewind = (info3 == 1)? -4 : -3;

	// Here the Start code, the complete NALU, and the next start code is in the Buf.  
	// The size of Buf is pos, pos+rewind are the number of bytes excluding the next
	// start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code
	nalu->len = (pos+rewind) - nalu->startcodeprefix_len;
	memcpy (nalu->buf, &frame[nalu->startcodeprefix_len], nalu->len);//Copy one complete NALU excluding start prefix 0x000001 and 0x00000001
	nalu->forbidden_bit = nalu->buf[0] & 0x80;                     //1 bit
	nalu->nal_reference_idc = nalu->buf[0] & 0x60;                 // 2 bit
	nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;                   // 5 bit

	return (pos+rewind);                                           //Return the length of bytes from between one NALU and the next NALU

}

int CH264Handle::GetAnnexbNALU (NALU_t *nalu)
{
	int pos = 0;
	int StartCodeFound, rewind;
	unsigned char *Buf;

	if ((Buf = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL) 
		printf ("GetAnnexbNALU Error: Could not allocate Buf memory\n");

	nalu->startcodeprefix_len=3;      //Initialize the bits prefix as 3 bytes

	if (3 != fread (Buf, 1, 3, m_bitStream))//Read 3 bytes from the input file
	{
		free(Buf);
		return 0;
	}
	info2 = FindStartCode2 (Buf);    //Check whether Buf is 0x000001
	if(info2 != 1) 
	{
		//If Buf is not 0x000001,then read one more byte
		if(1 != fread(Buf+3, 1, 1, m_bitStream))
		{
			free(Buf);
			return 0;
		}
		info3 = FindStartCode3 (Buf);//Check whether Buf is 0x00000001
		if (info3 != 1)              //If not the return -1
		{ 
			free(Buf);
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
	printf("Prefix length : %d.\n",nalu->startcodeprefix_len);
	while (!StartCodeFound)
	{
		if (feof (m_bitStream))
		{
			nalu->len = (pos-1)-nalu->startcodeprefix_len;
			memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);     
			nalu->forbidden_bit = nalu->buf[0] & 0x80;     // 1 bit--10000000
			nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit--01100000
			nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;   // 5 bit--00011111
			free(Buf);
			return pos-1;
		}
		Buf[pos++] = fgetc (m_bitStream);                       //Read one char to the Buffer
		info3 = FindStartCode3(&Buf[pos-4]);		     //Check whether Buf is 0x00000001
		if(info3 != 1)
			info2 = FindStartCode2(&Buf[pos-3]);           //Check whether Buf is 0x000001
		StartCodeFound = (info2 == 1 || info3 == 1);
	}

	// Here, we have found another start code (and read length of startcode bytes more than we should
	// have.  Hence, go back in the file
	rewind = (info3 == 1)? -4 : -3;

	if (0 != fseek (m_bitStream, rewind, SEEK_CUR))						//Set the file position to the end of previous NALU
	{
		free(Buf);
		printf("GetAnnexbNALU Error: Cannot fseek in the bit stream file");
	}

	// Here the Start code, the complete NALU, and the next start code is in the Buf.  
	// The size of Buf is pos, pos+rewind are the number of bytes excluding the next
	// start code, and (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code
	nalu->len = (pos+rewind)-nalu->startcodeprefix_len;
	memcpy (nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);//Copy one complete NALU excluding start prefix 0x000001 and 0x00000001
	nalu->forbidden_bit = nalu->buf[0] & 0x80;                     //1 bit
	nalu->nal_reference_idc = nalu->buf[0] & 0x60;                 // 2 bit
	nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;                   // 5 bit
	free(Buf);

	return (pos+rewind);                                           //Return the length of bytes from between one NALU and the next NALU
}

NALU_t* CH264Handle::AllocNALU(int buffersize)
{
	NALU_t *n;

	if ((n = (NALU_t*)calloc (1, sizeof(NALU_t))) == NULL)
	{
		printf("AllocNALU Error: Allocate Meory To NALU_t Failed ");
		exit(0);
	}

	n->max_size=buffersize;									//Assign buffer size 

	if ((n->buf = (char*)calloc (buffersize, sizeof (char))) == NULL)
	{
		free (n);
		printf ("AllocNALU Error: Allocate Meory To NALU_t Buffer Failed ");
		exit(0);
	}

	return n;
}
void CH264Handle::FreeNALU(NALU_t *n)
{
	if (n)
	{
		if (n->buf)
		{
			free(n->buf);
			n->buf=NULL;
		}
		free (n);
	}
}
void CH264Handle::OpenBitstreamFile (char *fn)
{
	if (NULL == (m_bitStream=fopen(fn, "rb")))
	{
		printf("Error: Open input file error\n");
		exit(0);
	}
	
   printf("File opened.\n");
}

int CH264Handle::FindStartCode2 (unsigned char *Buf)
{
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=1) return 0;              //Check whether buf is 0x000001
	else return 1;
}

int CH264Handle::FindStartCode3 (unsigned char *Buf)
{
	if(Buf[0]!=0 || Buf[1]!=0 || Buf[2] !=0 || Buf[3] !=1) return 0;//Check whether buf is 0x00000001
	else return 1;
}

void CH264Handle::SaveSPS (unsigned char *sps, int len)
{
	if(len < 64)
	{
		memcpy(m_Sps, sps, len);
		m_SpsLen = len;
	}
}
void CH264Handle::SavePPS (unsigned char *pps, int len)
{
	if(len < 64)
	{
		memcpy(m_Pps, pps, len);
		m_PpsLen = len;
	}
}

void CH264Handle::GetSPS (unsigned char* &sps, int& len)
{
	sps = m_Sps;
	len = m_SpsLen;
}
void CH264Handle::GetPPS (unsigned char* &pps, int& len)
{
	pps = m_Pps;
	len = m_PpsLen;
}

