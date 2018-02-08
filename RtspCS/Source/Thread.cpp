#include "Thread.h"
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif	//_WIN32
#include "PrintLog.h"

CThread::CThread()
{
	m_is_destroyed = false;
	m_is_runing = false;
#ifndef _WIN32
	m_handle = -1;
#else
	m_handle = NULL;
#endif
	m_is_exited = true;
	m_fun = NULL;
	m_user_info = 0;
	memset( m_thread_name, 0, sizeof(m_thread_name) );
}

CThread::~CThread()
{
#ifdef _WIN32
	if( m_handle != NULL )
		CloseHandle( m_handle );
#else
	//
#endif
}

int CThread::Create( const char* thread_name, long user_info, ThreadFun fun )
{
	strncpy( m_thread_name, thread_name, sizeof(m_thread_name)-1 );
	m_user_info = user_info;
	m_fun = fun;
#ifdef _WIN32
	int thread_id = 0;
	m_handle = CreateThread( NULL, 0, thread_fun, this, 0, (LPDWORD)&thread_id );
	if( m_handle == NULL ){
#else
	if( pthread_create( &m_handle, NULL, thread_fun, this ) != 0 ){
#endif
		LogError( "create thread failed\n" );
		return -1;
	}
	return 0;
}

void CThread::Destroy()
{
	m_is_destroyed = true;
}

void CThread::WaitExit()
{
	int i = 400;
	while( i-- > 0 ){
		if( m_is_exited == true )
			break;
		Sleep(5);
	}
	if( m_is_exited == false ){
		LogInfo( "terminate thread %s\n", m_thread_name );
#ifndef _WIN32
		pthread_cancel( m_handle );
#else
		TerminateThread( m_handle, 0 );
#endif
	}
}

bool CThread::IsDestroyed()
{
	return m_is_destroyed;
}

bool CThread::IsRuning()
{
	return m_is_runing;
}

int CThread::GetCurThreadId()
{
#ifdef _WIN32
	return GetCurrentThreadId();
#else
	return pthread_self();
#endif
}

#ifdef _WIN32
DWORD WINAPI CThread::thread_fun( LPVOID arg )
{
#else
void* CThread::thread_fun( void* arg )
{
	pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL ); // 设置其他线程可以cancel掉此线程
	pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );   //设置立即取消
#endif
	CThread* thread = (CThread*)arg;
	LogInfo( "enter thread %s, id:%d\n", thread->m_thread_name, GetCurThreadId() );
	thread->m_is_runing = true;
	thread->m_is_exited = false;
	if( thread->m_fun == NULL )
		thread->thread_proc( thread->m_user_info );
	else
		thread->m_fun( thread, thread->m_user_info );
	LogInfo( "leave thread %s, id:%d\n", thread->m_thread_name, GetCurThreadId() );
	thread->m_is_runing = false;
	thread->m_is_exited = true;
#ifndef _WIN32
	pthread_detach( pthread_self() );
#endif
	return 0;
}
