#ifndef NALU_UTILS_HH
#define NALU_UTILS_HH

#define USE_SEL_FRAMER 1
int find_start_code3(unsigned char* data, int lenght);
int get_annexb_nalu(unsigned char* data, int lenght);

#endif
