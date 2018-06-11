#ifndef NALU_UTILS_HH
#define NALU_UTILS_HH

#define USE_SEL_FRAMER 1

typedef struct
{
	int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
	unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
	unsigned max_size;            //! NAL Unit Buffer size
	int forbidden_bit;            //! Should always be FALSE
	int nal_reference_idc;        //! NALU_PRIORITY_xxxx
	int nal_unit_type;            //! NALU_TYPE_xxxx    
	unsigned char *buf;                    //! contains the first byte followed by the EBSP
	unsigned short lost_packets;  //! true, if packet loss is detected
} NALU_t;

int find_start_code2(unsigned char *data);
int find_start_code3(unsigned char *data);
int get_annexb_nalu(unsigned char *frame, int length, NALU_t *nalu);

#endif
