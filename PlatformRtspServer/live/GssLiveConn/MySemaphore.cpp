#include "MySemaphore.h"

#ifndef WIN32
#include <errno.h>
#endif

MySem::MySem()
{
	m_bInited = Init();
}

MySem::~MySem()
{
	UnInit();
}

bool MySem::Init()
{
	bool bsuc =false;

#ifdef WIN32
	m_sem = CreateSemaphore( 
		NULL,           // default security attributes
		0,  // initial count
		1,  // maximum count
		NULL);          // unnamed semaphore
	if(m_sem == NULL)
	{
		printf("CreateSemaphore error: %d\n", GetLastError());
		bsuc = false;
	}
	else 
	{
		bsuc = true;
	}
#else
	if(sem_init(&m_sem,0,0) != 0)
	{
		printf("CreateSemaphore error: %d\n", errno);
		bsuc = false;
	}
	else
	{
		bsuc = true;
	}
#endif
	
	m_bInited = bsuc;

	return bsuc;
}

bool MySem::Wait()
{
#ifdef WIN32
	DWORD  dwWaitResult = WaitForSingleObject( 
		m_sem,   // handle to semaphore
		INFINITE);
	if (WAIT_OBJECT_0 == dwWaitResult)
		return true;
	else
		return false;
#else
	if( 0 == sem_wait(&m_sem) )
		return true;
	else
		return false;
#endif
}

bool MySem::Post()
{
#ifdef WIN32
	if(ReleaseSemaphore( 
		m_sem,  // handle to semaphore
		1,            // increase count by one
		NULL))
		return true;
	else 
		return false;
#else
	if( sem_post(&m_sem) == 0 )
	{
		return true;
	}
	else
	{
		printf("sem Post failed! %d\n",errno);
		return false;
	}
#endif
}

bool MySem::UnInit()
{
#ifdef WIN32
	CloseHandle(m_sem);
#else
	sem_destroy(&m_sem);
#endif
	m_bInited = false;
	return true;
}