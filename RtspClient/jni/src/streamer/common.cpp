#include "common.h"
#include <errno.h>

#if defined(WIN32) || defined(_WIN32)
void* sem_create(int initial_count, int max_count)
{
    HANDLE h = CreateEvent(NULL, FALSE, initial_count != 0, NULL); 
	return (void*)h;
}

int sem_delete(void* handle)
{
	assert( NULL != handle);
	return (CloseHandle( (HANDLE)handle ) )? 0 : -1;
}

bool sem_wait(void* handle, unsigned int timeout)
{
	assert( NULL != handle);
	return WaitForSingleObject((HANDLE)handle, timeout) == WAIT_OBJECT_0;
}

int sem_post(void* handle)
{
	assert( NULL != handle);
	return((SetEvent((HANDLE)handle)!=0)? 0:-1);
}

int sem_getcount(void* handle, int *count)
{
	assert( NULL != handle);
	*count = -1;
	return 0;
}

#else
void* sem_create(int initial_count, int max_count)
{
	sem_t *sem = (sem_t*)malloc(sizeof(sem_t));
	::sem_init(sem, 0, initial_count);
	return sem;
}

int sem_delete(void* handle)
{
//	assert( NULL != handle);
	::sem_destroy((sem_t*)handle);
	free(handle);
	return (0);
}

int sem_getcount(void* handle, int *count)
{
//	assert( NULL != handle);
	::sem_getvalue((sem_t *)handle, count);
	return 0;
}

int sem_post(void* handle)
{
//	assert( NULL != handle);
    return ::sem_post((sem_t*)handle);
}

bool sem_wait(void* handle, unsigned int timeout)
{
//	assert( NULL != handle);
	struct timespec abstime;
	abstime.tv_sec  = ::time(NULL) + (timeout/1000);
	abstime.tv_nsec = (timeout%1000) * 1000000;

	do{
	if( ::sem_timedwait((sem_t*)handle, &abstime) == 0 ) return true;
	if( errno != EINTR ) break;
	}while(1);
	return false;
 }
#endif

///////////////////////////////////////////////////////////////////
CMutex::CMutex()
{
#ifdef defined(WIN32) || defined(_WIN32)
	InitializeCriticalSection(&m_lock);
#else
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_lock, &attr);
	pthread_mutexattr_destroy(&attr);
#endif
}

CMutex::~CMutex()
{
#ifdef defined(WIN32) || defined(_WIN32)
	DeleteCriticalSection(&m_lock);
#else
	pthread_mutex_destroy(&m_lock);
#endif
}

void CMutex::Enter()
{
#ifdef defined(WIN32) || defined(_WIN32)
	EnterCriticalSection(&m_lock);
#else	
	pthread_mutex_lock(&m_lock);
#endif
}

void CMutex::Leave()
{
#ifdef defined(WIN32) || defined(_WIN32)
	LeaveCriticalSection(&m_lock);
#else	
	pthread_mutex_unlock(&m_lock);
#endif
}

int64_t GetTickCount()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
}

int GetIndexBySamplerate(int smaplerate)
{
	static int table[] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350 };	

	int index = 0;

	do{//check it from table
	if( table[index] == smaplerate ) break;

	if((++ index) == sizeof(table) ) return 4; //44.1kbps -> 4
	}while(1);

	return index;
}

int nalu_find_first_of(char *data, int size, char *&nalu)
{
	do{
	if( size < 4 ) break;
	if( data[0] == 0 && data[1] == 0 ) {
	if( data[2] == 1 )
	{
		nalu = data + 3;
		return 3;
	}
	if( data[2] == 0 && data[3] == 1 )
	{
		nalu = data + 4;
		return 4;
	}
	}
	data ++;
	size --;
	}while(1);

	return 0;
}

int nalu_find_prefix(char *data, int size)
{
	int ipos = 0;

	do{
	if( size < 4 ) break;
	if( data[0] == 0 && data[1] == 0 ) {
	if( data[2] == 1 ) return ipos; //001
	if( data[2] == 0 && data[3] == 1 ) return ipos; //0001
	}
	data ++;
	size --;
	ipos ++;
	}while(1);

	return ipos + size;
}
