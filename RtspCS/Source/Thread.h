#ifndef __INCLUDE_THREAD_H__
#define __INCLUDE_THREAD_H__

#include "Def.h"
#ifdef _WIN32
#include <WinSock2.h>
#else //_WIN32
#include <pthread.h>
#endif	//_WIN32

//线程类，用户可继承CThread实现thread_proc并在thread_proc中处理事件，
//也可不继承CThread，通过设置ThreadFun以处理事件。
class CThread
{
public:
	CThread();
	virtual ~CThread();
public:
	//线程函数定义
	//参数：thread：线程对象执指针，user_info：用户信息
	typedef void (*ThreadFun)( CThread* thread, long user_info );
	//创建
	//参数：thread_name：线程名，user_info：用户信息，
	//		fun：线程函数,NULL则在新线程中调用thread_proc，否则调用fun
	//返回值：0成功，-1失败
	int Create( const char* thread_name, long user_info, ThreadFun fun = NULL );
	//销毁
	void Destroy();
	//等待退出线程，需先调用Destroy
	void WaitExit();
	//是否已经销毁，在新线程处理函数中需判断是否已经销毁，若销毁则需返回
	//返回值：true：已销毁，false：未销毁
	bool IsDestroyed();
	//新线程是否正在运行
	//返回值：true：正在运行，false：未运行
	bool IsRuning();
	//获取当前线程ID
	//返回值：当前线程ID
	static int GetCurThreadId();
private:
	//新线程执行处理函数，由用户继承实现
	//参数：user_info：用户信息
	virtual void thread_proc( long user_info ){ return; }
private:
#ifdef _WIN32
	static DWORD WINAPI thread_fun( LPVOID arg );
#else
	static void* thread_fun( void *arg );
#endif
private:
	bool m_is_destroyed;
	bool m_is_runing;
#ifdef _WIN32
	HANDLE m_handle;
#else
	pthread_t m_handle;
#endif
	bool m_is_exited;
	ThreadFun m_fun;
	long m_user_info;
	char m_thread_name[128];
};

#endif