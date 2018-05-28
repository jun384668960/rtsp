

#include "Log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <string.h>
#include <time.h>

#ifndef WIN32
	#include <unistd.h>
	#include<sys/types.h>
	#include<sys/stat.h>
	#include<fcntl.h>
#endif

#define LOG_NAME_PREFIX 32
#define LOG_FILENAME_LEN 64
#define LOG_FILEPATH_LEN 128
static const char *levelstr[] = {"[NON]", "[ERROR]", "[WARN]", "", 
								"[INFO]", "", "", "", "[DEBUG]",
								"", "", "", "",
								"", "", "", "[HLIGHT]", NULL};

//static unsigned int m_logLevel = Log_DEBUG;
unsigned int GlobalLogLevel = Log_DEBUG;

static int m_currentlen = 0;
static int m_maxline = 0;
static char m_filename[LOG_FILENAME_LEN] = {0};
static char m_prefixname[LOG_NAME_PREFIX] = {0};
#ifdef WIN32
static FILE * m_fp = NULL;
#else
static int   m_fp = -1;
#endif
//static int m_th = 0;
//static char log_cache[40960];

int TimeStr(int *year, int *month, int *day, int *hour, int *minute, int *second)
{
	time_t now;
	struct tm *timenow;
	time(&now);
	timenow = localtime(&now);
	/*注释：time_t是一个在time.h中定义好的结构体。而tm结构体的原形如下
	struct   tm
	{
	int   tm_sec;//seconds	0-61
	int   tm_min;//minutes	1-59
	int   tm_hour;//hours	0-23
	int   tm_mday;//day of the month	1-31
	int   tm_mon;//months since jan	0-11
	int   tm_year;//years from 1900
	int   tm_wday;//days since Sunday 0-6
	int   tm_yday;//days since Jan 1, 0-365
	int   tm_isdst;//Daylight Saving time indicator
	}*/
	if(timenow == NULL){
		return -1;
	}
	if(year!= NULL){
		*year = timenow->tm_year + 1900;
	}
	if(month!= NULL){
		*month = timenow->tm_mon + 1;
	}
	if(day!= NULL){
		*day = timenow->tm_mday;
	}
	if(hour!= NULL){
		*hour = timenow->tm_hour;
	}
	if(minute!= NULL){
		*minute = timenow->tm_min;
	}
	if(second!= NULL){
		*second = timenow->tm_sec;
	}
	return 0;
}


void CloseFile()
{
	m_currentlen = 0;
#ifdef  WIN32
	if(m_fp != NULL)
	{
		fflush(m_fp);
		fclose(m_fp);
		m_fp = NULL;
	}
#else
	close(m_fp);
	m_fp = -1;
#endif
}

int OpenFile()
{
	char buf[128];
	int year, mo, day, hour, minute, second;
	memset(buf, 0, 128);	//sprintf(buf,"%d",m_th);
	TimeStr(&year, &mo, &day, &hour, &minute, &second);
// 	if(strcmp("",m_prefixname) == 0)
// 		sprintf(buf,"%s%04d%02d%02d_%02d%02d%02d.log", m_filename, year, mo, day, hour, minute, second);
// 	else
	sprintf(buf,"%s%s%04d%02d%02d_%02d%02d%02d.log", m_filename, m_prefixname, year, mo, day, hour, minute, second);
#ifdef WIN32
	m_fp = fopen(buf, "a+");
	if(m_fp != NULL){
		//m_th++;
		return 0;
	}
#else
	m_fp = open(buf, O_RDWR|O_CREAT, 777);
	if(m_fp != -1){
		return 0;
	}	
#endif
	return -1;
}


void WriteToScreen(char *buffer, int len)
{
	if(buffer != NULL && len > 0){
		printf("%s", buffer);
		fflush(stdout);
	}
}

int WriteToFile(char *buffer, int len)
{
	if(buffer == NULL || len <= 0){
		return 0;
	}
#ifdef WIN32
	if(m_fp == NULL){
		OpenFile();
		if(m_fp == NULL){
			return -1;
		}
	}

	fwrite(buffer, len, 1, m_fp);
	++m_currentlen;
	if(fflush(m_fp) != 0 || m_maxline <= m_currentlen){
		CloseFile();		
	}
#else
	if(m_fp == -1){
		OpenFile();
		if(m_fp == -1){
			return -1;
		}
	}
	write(m_fp, buffer, len);
	++m_currentlen;
	if(m_maxline <= m_currentlen){
		CloseFile();		
	}
#endif
	return 0;
}

void LogSetLevel(unsigned int logLevelOpt)
{
	//m_logLevel = logLevelOpt;
	GlobalLogLevel = logLevelOpt;
}
	
int LogInit(const char *filename, const char *prefixname,int maxline)
{
	int len = 0;
	if(filename != 0)
	{
		len = strlen(filename);	
		len = (len > LOG_FILENAME_LEN ? LOG_FILENAME_LEN:len);
		memcpy(m_filename, filename, len);
		m_filename[len] = 0;
	}
	if (prefixname != 0)
	{
		len = strlen(prefixname);
		len = (len > LOG_NAME_PREFIX ? LOG_NAME_PREFIX : len);
		memcpy(m_prefixname,prefixname,len);
		m_prefixname[len] = 0;
	}
	m_maxline = maxline;
	return 0;
}

void LogToFile(LogLevelOpt level, const char *format, ...)
{
	char log_cache[40960];	
	
	int len = 25;
	int mo, day, hour, minute, second;
	TimeStr(NULL, &mo, &day, &hour, &minute, &second);
	sprintf(log_cache,"%-9s%02d-%02d %02d:%02d:%02d ",levelstr[level], mo, day, hour, minute, second);
	va_list argptr;
	va_start(argptr, format);
	len += vsprintf(log_cache + 24, format, argptr);
	va_end(argptr);
	log_cache[len-1] = '\n';
	log_cache[len] = 0;
	
	if(WriteToFile(log_cache, len) != 0){
		WriteToScreen(log_cache, len);	
	}	
}

void LogToScreen(LogLevelOpt level, const char *format, ...)
{
	char log_cache[40960];
	/*if((m_logLevel & level) == 0){
		return;
	}*/
	int len = 25;
	int mo, day, hour, minute, second;
	TimeStr(NULL, &mo, &day, &hour, &minute, &second);
	sprintf(log_cache,"%-9s%02d-%02d %02d:%02d:%02d ",levelstr[level], mo, day, hour, minute, second);
	va_list argptr;
	va_start(argptr, format);
	len += vsprintf(log_cache + 24, format, argptr);
	va_end(argptr);
	log_cache[len-1] = '\n';
	log_cache[len] = 0;
	WriteToScreen(log_cache, len);		
}

	