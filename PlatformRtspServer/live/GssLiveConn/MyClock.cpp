#include "MyClock.h"

#include <stdio.h>


MyClock::MyClock()
{
	m_benable = true;
	m_bPrintf = false;
	Init();
}

MyClock::~MyClock()
{
	Uninit();
}

bool MyClock::Lock()
{
	if(!m_benable)
		return false;
	if(m_bPrintf)
		printf("=============MyClock::Lock()==============\n");
#ifdef WIN32
	DWORD dwWaitResult = WaitForSingleObject( m_mutex, INFINITE); 
	if (WAIT_OBJECT_0 == dwWaitResult)
		return true;
	else
		return false;
#else
	if(pthread_mutex_lock(&m_mutex) == 0)
		return true;
	else
		return false;
#endif
}

bool MyClock::Unlock()
{
	if(!m_benable)
		return false;
	if(m_bPrintf)
		printf("=============MyClock::Unlock()==============\n");
#ifdef WIN32 
	return ReleaseMutex(m_mutex);
#else
	if(pthread_mutex_unlock(&m_mutex) == 0)
		return true;
	else
		return false;
#endif
}

bool MyClock::Init()
{
#ifdef WIN32
	m_mutex = CreateMutex(NULL,false,NULL);
	if (m_mutex == NULL)
		return false;
	else
		return true;
#else
	if(pthread_mutex_init(&m_mutex,NULL) == 0)
		return true;
	else
		return false;
#endif
}

bool MyClock::Uninit()
{
#ifdef WIN32
	return CloseHandle(m_mutex);
#else
	if(pthread_mutex_destroy(&m_mutex) == 0)
		return true;
	else
		return false;
#endif
}

void MyClock::Enable( bool bEnable )
{
	m_benable = bEnable;
}