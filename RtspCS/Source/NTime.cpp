#include "NTime.h"
#include <assert.h>
#include <stdio.h>
#ifdef _WIN32
#include <WinSock2.h>
#else	//_WIN32
#include <sys/time.h>
#endif	//_WIN32

#ifdef _WIN32
uint64_t CNTime::m_freq = 0;
#endif

uint64_t CNTime::GetCurMs()
{
	return GetCurUs()/1000;
}

uint64_t CNTime::GetCurUs()
{
#ifdef _WIN32
	LARGE_INTEGER litmp; 
	QueryPerformanceCounter( &litmp );
	return litmp.QuadPart / ( get_freq() / 1000000 );
#else
	struct timeval cur_time;
	gettimeofday( &cur_time, NULL );
	return cur_time.tv_sec*1000000+cur_time.tv_usec;
#endif
}

#ifdef _WIN32
uint64_t CNTime::get_freq()
{
	if( m_freq == 0 ){
		LARGE_INTEGER litmp; 
		QueryPerformanceFrequency( &litmp );
		m_freq = litmp.QuadPart;
	}
	return m_freq;
}
#endif
