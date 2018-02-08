#include "Mutex.h"

CMutex::CMutex()
{
#ifdef _WIN32
	InitializeCriticalSection( &m_mutex );
#else
	pthread_mutex_init( &m_mutex, NULL );
#endif
}

CMutex::~CMutex()
{
#ifdef _WIN32
	DeleteCriticalSection( &m_mutex );
#else
	pthread_mutex_destroy( &m_mutex );
#endif
}

void CMutex::Enter()
{ 
#ifdef _WIN32
	EnterCriticalSection( &m_mutex );
#else
	pthread_mutex_lock( &m_mutex );
#endif
}

void CMutex::Leave()
{ 
#ifdef _WIN32
	LeaveCriticalSection( &m_mutex );
#else
	pthread_mutex_unlock( &m_mutex );
#endif
}
