#ifndef CH264HANDLE_H
#define CH264HANDLE_H
#include <stdio.h>
#include <string.h>

#define MAX_NALU_SIZE 512*1024

typedef struct
{
	int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
	unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
	unsigned max_size;            //! NAL Unit Buffer size
	int forbidden_bit;            //! Should always be FALSE
	int nal_reference_idc;        //! NALU_PRIORITY_xxxx
	int nal_unit_type;            //! NALU_TYPE_xxxx    
	char *buf;                    //! contains the first byte followed by the EBSP
	unsigned short lost_packets;  //! true, if packet loss is detected
} NALU_t;

class CH264Handle
{
public:
	CH264Handle(char* pFile = "test.264");
	~CH264Handle(void);

	static int FindStartCode2 (unsigned char *Buf);
	static int FindStartCode3 (unsigned char *Buf);

	int GetAnnexbFrame (unsigned char *frame, int length);
	int GetAnnexbNALU (unsigned char *frame, int &length, NALU_t *nalu);

	int GetAnnexbNALU (NALU_t *nalu);
	
	bool BitStreamFeof();
	
	static NALU_t *AllocNALU(int buffersize);
	static void FreeNALU(NALU_t *n);
	
	void OpenBitstreamFile (char *fn);
	void SaveSPS (unsigned char *sps, int len);
	void SavePPS (unsigned char *pps, int len);
	void GetSPS (unsigned char* &sps, int& len);
	void GetPPS (unsigned char* &pps, int& len);
private:

public:
	int info2;
	int info3;
	unsigned char m_Sps[64];
	int m_SpsLen;
	unsigned char m_Pps[64];
	int m_PpsLen;
    FILE*	m_bitStream;
};
#endif
