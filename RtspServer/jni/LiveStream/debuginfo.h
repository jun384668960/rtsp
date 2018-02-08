#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>

//define function for short

#define debug_log 	DebugLog::GetInstance()->WritrLog
#define debug_set 	DebugLog::GetInstance()->SetDebugInit

#define LEVEL_ERROR		0
#define LEVEL_CRITICAL	1
#define LEVEL_WARNING	2
#define LEVEL_MESSAGE	3
#define LEVEL_DEBUG		4
#define LEVEL_TRACE		5

#define TARGET_STDERR   0
#define TARGET_LOGFILE  1
#define TARGET_NULL     2

#define MAX_LOG_SIZE	200000

class DebugLog
{
public:
	~DebugLog();
	void SetDebugInit(int level, int target, char* fileName);
	void WritrLog(int level, const char *pszFmt,...);
	static DebugLog* GetInstance();
	static DebugLog*	m_instance;
	
protected:
	DebugLog();
	void LogV(const char *pszFmt,va_list argp);
	
private:
	pthread_mutex_t m_csLog;
	FILE*	m_pflog;
	int 	m_level;
	int 	m_target;
	char 	m_logstr[MAX_LOG_SIZE+1];
	char	m_logFile[256];
	char	m_logFile2[257];
};

#endif
