#ifndef __INCLUDE_N_TIME_H__
#define __INCLUDE_N_TIME_H__

#include "Def.h"

//系统时间类
class CNTime
{
public:
	//获取系统运行至此时的毫秒时间
	//返回值：系统运行至此时的毫秒时间
	static uint64_t GetCurMs();
	//获取系统运行至此时的微秒时间
	//返回值：系统运行至此时的微秒时间
	static uint64_t GetCurUs();
#ifdef _WIN32
private:
	static uint64_t get_freq();
private:
	static uint64_t m_freq;
#endif
};

#endif