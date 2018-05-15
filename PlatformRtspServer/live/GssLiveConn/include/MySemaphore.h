#ifndef _MYSEMAPHORE_H_
#define _MYSEMAPHORE_H_

#ifdef WIN32
#include <windows.h>
typedef HANDLE MYSEMH;
#else
#include <semaphore.h> //信号量
typedef sem_t MYSEMH;
#endif

#include <stdio.h>

class MySem {
public:
	MySem();
	~MySem();
	bool IsInitOk() { return m_bInited; }
	bool Init();
	bool Wait();
	bool Post();
	bool UnInit();
protected:
	MYSEMH m_sem;
	bool m_bInited;
};

#endif //_MYSEMAPHORE_H_