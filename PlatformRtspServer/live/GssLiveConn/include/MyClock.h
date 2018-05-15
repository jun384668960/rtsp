#ifndef _MYCLOCK_H__
#define _MYCLOCK_H__

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

class MyClock
{
public:
	MyClock();
	~MyClock();
	bool Init();
	void Enable(bool bEnable);
	bool Lock();
	bool Unlock();
	bool Uninit();

	void SetPrintfFlag(bool bPrint) { m_bPrintf = bPrint; }
protected:
private:
#ifdef WIN32
	HANDLE m_mutex;
#else
	pthread_mutex_t m_mutex;
#endif
	bool m_benable;
	bool m_bPrintf;
};

#endif //_MYCLOCK_H__