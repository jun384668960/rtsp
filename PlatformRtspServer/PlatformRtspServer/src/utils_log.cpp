/**************************************************************************
 * 	FileName:		gos_log.c
 *	Description:	日志
 *	Copyright(C):	2014-2020 gos Inc.
 *	Version:		V 1.0
 *	Author:			Chenjb
 *	Created:		2014-06-09
 *	Updated:		
 *					
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils_log.h"
#ifdef __cplusplus
extern "C"{
#endif

static char s_log_buffer[MAX_LOG_BUFSIZE] = {0};
static log_ctrl* s_log_ctrl;
static int s_log_level = LOG_INFO_LVL;


log_ctrl* log_ctrl_create(char* file, int level, int wt)
{
	log_ctrl* log = (log_ctrl*)malloc(sizeof(log_ctrl));

	log->fd = fopen(file, "w+");
	if(log->fd == NULL)
	{
		printf("log_ctrl_create error to open file %s\n", file);
		return NULL;
	}

	strcpy(log->file, file);
	log->level = level;
	log->wt = wt;

	s_log_ctrl = log;
	s_log_level = level;
	
	return log;
}

void log_ctrl_destory(log_ctrl* log)
{
	if(log->fd != NULL)
		fclose(log->fd);
	
	free(log);
}
int  log_ctrl_level_set(log_ctrl* log, int level)
{
	log->level = level;
	s_log_level = level;
	
	return 0;
}
int  log_ctrl_wt_set(log_ctrl* log,int wt)
{
	log->wt = wt;
	
	return 0;

}

int log_ctrl_file_copy(log_ctrl* log)
{
	char buff[1024]; 
	int len;
    FILE *in,*out;  
	char bak[128] = {0};

  	sprintf(bak,"%s.bak",log->file);
    in = fopen(log->file,"r+");  
    out = fopen(bak,"w+");  
  
    while(len = fread(buff,1,sizeof(buff),in))  
    {  
        fwrite(buff,1,len,out);  
    }  

	fclose(in);
	fclose(out);
	
    return 0; 
}

int log_ctrl_file_write(log_ctrl* log, char* data, int len)
{
	if(log == NULL || log->fd == NULL)
	{
		LOGW_print("handle%p, fd:%p", log, log->fd);
		return -1;
	}
		

	fwrite(data, 1, len, log->fd);
	fflush(log->fd);

	//检测文件大小 是否备份刷新
	unsigned long filesize = -1;      
    struct stat statbuff;  
    if(stat(log->file, &statbuff) >= 0)
	{  
        filesize = statbuff.st_size;  
    }
	else
	{
		LOGW_print("get file %s size error", log->file);
	}

	if(filesize > MAX_LOG_FILESIZE)
	{
		fclose(log->fd);
		log_ctrl_file_copy(log);
		
		remove(log->file);
		log->fd = fopen(log->file, "w+");
	}
	
	return 0;
}

int  log_ctrl_print(log_ctrl* log, int level, char* t, ...)
{
	if(log == NULL)
	{
		if(level <= s_log_level)
		{
			struct timeval v;
			gettimeofday(&v, 0);
			struct tm *p = localtime(&v.tv_sec);
			char fmt[256]; //限制t不能太大
			sprintf(fmt, "%s%04d/%02d/%02d %02d:%02d:%02d.%03d %s %s\n"NONE,level==LOG_TRACE_LVL? "":(level==LOG_DEBUG_LVL? LIGHT_GREEN:(level==LOG_INFO_LVL? LIGHT_CYAN:(level==LOG_WARN_LVL?YELLOW:LIGHT_RED)))
					, 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, (int)(v.tv_usec/1000)
					, level==LOG_TRACE_LVL? "TRACE":(level==LOG_DEBUG_LVL? "DEBUG":(level==LOG_INFO_LVL? "!INFO":(level==LOG_WARN_LVL? "!WARN":"ERROR"))), t);

			va_list params;
			va_start(params, t);
			vfprintf(stdout, fmt, params);
			va_end(params);
			fflush(stdout);
		}
		
		return 0;
	}
	else if(level <= log->level)
	{
		if(log->wt == 0)
		{
			
			struct timeval v;
			gettimeofday(&v, 0);
			struct tm *p = localtime(&v.tv_sec);
			char fmt[256]; //限制t不能太大
			sprintf(fmt, "%s%04d/%02d/%02d %02d:%02d:%02d.%03d %s %s\n"NONE,level==LOG_TRACE_LVL? "":(level==LOG_DEBUG_LVL? LIGHT_GREEN:(level==LOG_INFO_LVL? LIGHT_CYAN:(level==LOG_WARN_LVL?YELLOW:LIGHT_RED)))
					, 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, (int)(v.tv_usec/1000)
					, level==LOG_TRACE_LVL? "TRACE":(level==LOG_DEBUG_LVL? "DEBUG":(level==LOG_INFO_LVL? "!INFO":(level==LOG_WARN_LVL? "!WARN":"ERROR"))), t);

			va_list params;
			va_start(params, t);
			vfprintf(stdout, fmt, params);
			va_end(params);
			fflush(stdout);
		}
		else
		{
			
			struct timeval v;
			gettimeofday(&v, 0);
			struct tm *p = localtime(&v.tv_sec);
			char fmt[256]; //限制t不能太大
			sprintf(fmt, "%04d/%02d/%02d %02d:%02d:%02d.%03d %s %s\n"
					, 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, (int)(v.tv_usec/1000)
					, level==LOG_TRACE_LVL? "TRACE":(level==LOG_DEBUG_LVL? "DEBUG":(level==LOG_INFO_LVL? "!INFO":(level==LOG_WARN_LVL? "!WARN":"ERROR"))), t);

			//这里需要上锁
			va_list params;
			va_start(params, t);
			vsprintf(s_log_buffer, fmt, params);
			va_end(params);

			//log_ctrl_file_write(log, s_log_buffer, strlen(s_log_buffer));
			
			printf("!%s%s"NONE,level==LOG_TRACE_LVL? "":(level==LOG_DEBUG_LVL? LIGHT_GREEN:(level==LOG_INFO_LVL? LIGHT_CYAN:(level==LOG_WARN_LVL?YELLOW:LIGHT_RED)))
				, s_log_buffer);
			fflush(stdout);
		}
	}
	
	return 0;
}
#ifdef __cplusplus
}
#endif

