#ifndef __COMMON_H__
#define __COMMON_H__

#include <sys/time.h>
#ifdef defined(WIN32) || defined(_WIN32)
#include "windows.h"
#else
#include <pthread.h> 
#include <semaphore.h>
#endif
#include "logimp.h"

///////////////////////////////////////////////////////////////////
//信号量

void *sem_create(int initial_count, int maxcount);
int   sem_delete(void* handle);
bool  sem_wait(void* handle, unsigned int timeout/*毫秒*/);
int   sem_post(void* handle);
int   sem_getcount(void* handle, int *count);

class CSemaphore
{
public:
	CSemaphore(int maxcount = 1)
	{
		m_handle = sem_create(0, maxcount);
	}
	virtual ~CSemaphore()
	{
		sem_delete(m_handle);
	}

public:
	bool Wait(unsigned int timeout/*毫秒*/)
	{
		return sem_wait(m_handle, timeout);
	}
	void Post()
	{
		sem_post(m_handle);
	}

private:
	void *m_handle;
};

///////////////////////////////////////////////////////////////////
//互斥锁
class CMutex
{
public:
	CMutex();
	virtual ~CMutex();

public:
	void Enter();
	void Leave();

public:
#if defined(WIN32) || defined(_WIN32)
	CRITICAL_SECTION *Out() { return &m_lock;}
#else
	pthread_mutex_t  *Out() { return &m_lock;}
#endif

private:
#if defined(WIN32) || defined(_WIN32)
	CRITICAL_SECTION m_lock;
#else
	pthread_mutex_t  m_lock;
#endif
};

class CScopeMutex
{
public:
	CScopeMutex(CMutex &lock)
	  : m_lock(lock)
	{
		m_lock.Enter();
	}
	virtual ~CScopeMutex()
	{
		m_lock.Leave();
	}

private:
	CMutex &m_lock;
};

///////////////////////////////////////////////////////////////////
extern int64_t GetTickCount(); //us

class CCostHelper
{
public:
 	CCostHelper()  { this->ref = GetTickCount();}

public:
	int64_t Get() const { return GetTickCount() - this->ref;}

private:
	int64_t ref;
};

struct ReferTimeScale
{
public:
	ReferTimeScale() { this->ref = this->pts = 0;}

	int     Adjust(int64_t pts, int adj = 100/*0.1sec*/)
	{
		this->ref = GetTickCount() / 1000 + adj - Get(pts);
		return adj;
	}
	void    Set(int64_t pts, int adj = 100/*0.1sec*/) 
	{ 
		this->ref = GetTickCount() / 1000 + adj;
		this->pts = pts;
	}
	int64_t Get(int64_t pts) const { return pts - this->pts;}
	int64_t Now() const
	{
		return GetTickCount() / 1000 - this->ref + this->pts;
	}

	int64_t ref; //system clock tickcount: ms
	int64_t pts; //ms
};

extern int GetIndexBySamplerate(int smaplerate);
extern int nalu_find_first_of(char *data, int size, char *&nalu);
extern int nalu_find_prefix(char *data, int size);

#endif //__COMMON_H__
