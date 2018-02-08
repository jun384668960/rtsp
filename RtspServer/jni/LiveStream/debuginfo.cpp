#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <jni.h>
#include <android/log.h>
#include "debuginfo.h"

DebugLog*	DebugLog::m_instance = NULL;

DebugLog* DebugLog::GetInstance()
{
    if(m_instance == NULL)
    {
        m_instance = new DebugLog();
    }

    return m_instance;
}

DebugLog::DebugLog()
{
    m_pflog = NULL;
	m_level = LEVEL_ERROR;
	m_target= TARGET_STDERR;

	pthread_mutex_init(&m_csLog, NULL);	

//	if(mkdir(LOG_DIR, S_IRWXU | S_IRGRP | S_IXGRP| S_IROTH) !=0)//创建新目录	
//	{	  
//    	WritrLog(LEVEL_ERROR, "mkdir %s failed\n", LOG_DIR);	  
//	}		    
}

DebugLog::~DebugLog()
{
    if(m_pflog != NULL)
    {
        fclose(m_pflog);
        m_pflog = NULL;
    }
}

void DebugLog::LogV(const char *pszFmt,va_list argp)
{
    if (NULL == pszFmt|| 0 == pszFmt[0]) 
        return;

    vsnprintf(m_logstr,MAX_LOG_SIZE,pszFmt,argp);

    struct timeval tv;
    time_t nowtime; 
    struct tm *nowtm; 
    char tmbuf[64], buf[64]; 
    
    gettimeofday(&tv, NULL);
    nowtime = tv.tv_sec;
    nowtm = localtime(&nowtime);
    strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
    snprintf(buf, sizeof buf, "[%s.%03d]", tmbuf, tv.tv_usec/1000);

    if(m_target == TARGET_LOGFILE)
    {
        if(NULL == m_pflog && m_logFile != NULL)
        {
            m_pflog = fopen(m_logFile, "a");	
        }
        
        if (NULL != m_pflog)
        {
            fprintf(m_pflog,"[%s] %s", buf, m_logstr);
            if (ftell(m_pflog) > MAX_LOG_SIZE)
            {
                fclose(m_pflog);
				m_pflog = NULL;
                if (rename(m_logFile, m_logFile2)) 
                {
                    remove(m_logFile2);
                    rename(m_logFile, m_logFile2);
                }
            }
            else
            {
                fclose(m_pflog);
                m_pflog = NULL;
            }
        }
    }

    //fprintf(stdout,"[%s] %s",buf, m_logstr);
	__android_log_print(ANDROID_LOG_ERROR, "rtsp-jni", "%s", m_logstr);
}

void DebugLog::WritrLog(int level, const char *pszFmt,...)
{    
    if(level <= m_level)
    {
        va_list argp;     
        pthread_mutex_lock(&m_csLog);    
        {        
            va_start(argp,pszFmt);        
            LogV(pszFmt,argp);        
            va_end(argp);    
        }    
        pthread_mutex_unlock(&m_csLog);
    }
}

void DebugLog::SetDebugInit(int level, int target, char* logFile)
{
    if(logFile == NULL)
    {
        return;
    }

    if(strlen(logFile) >= 256)
    {
        return;
    }
    else
    {
        strncpy(m_logFile, logFile, strlen(logFile));
        strncpy(m_logFile2, m_logFile, strlen(m_logFile));
        strncat(m_logFile2, "2", 1);
    }
    
    m_level = level;
    m_target= target;
    
}

