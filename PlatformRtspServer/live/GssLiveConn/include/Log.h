

#ifndef __LOG_H__
#define __LOG_H__

typedef enum
{
	Log_NON = 0,
	Log_ERROR = 1,
	Log_WARN = 2,
	Log_INFO = 4,
	Log_DEBUG = 8,
	Log_PLATFORM = 16
}LogLevelOpt;

extern unsigned int GlobalLogLevel;
void LogSetLevel(unsigned int logLevelOpt);
int  LogInit(const char *filepath,const char *prefixname, int maxline);
void LogToFile(LogLevelOpt level, const char *format, ...); 
void LogToScreen(LogLevelOpt level, const char *format, ...);

#ifdef WIN32

	#define LOG_ERROR(fmt, ...) do{ if(GlobalLogLevel & Log_ERROR) LogToFile(Log_ERROR, fmt, ##__VA_ARGS__);}while(0)
	#define LOG_WARN(fmt, ...) do{ if(GlobalLogLevel & Log_WARN) LogToFile(Log_WARN, fmt, ##__VA_ARGS__);}while(0)	
	#define LOG_INFO(fmt, ...) do{ if(GlobalLogLevel & Log_INFO) LogToFile(Log_INFO, fmt, ##__VA_ARGS__);}while(0)	
	#define LOG_DEBUG(fmt, ...) do{ if(GlobalLogLevel & Log_DEBUG) LogToFile(Log_DEBUG, fmt, ##__VA_ARGS__);}while(0)
	#define Log_PLATFORM(fmt, ...) do{ if(GlobalLogLevel & Log_PLATFORM) LogToFile(Log_PLATFORM, fmt, ##__VA_ARGS__);}while(0)
#else

	#define LOG_ERROR(fmt, arg...) do{ if(GlobalLogLevel & Log_ERROR) LogToFile(Log_ERROR, fmt, ##arg);}while(0)
	#define LOG_WARN(fmt, arg...) do{ if(GlobalLogLevel & Log_WARN) LogToFile(Log_WARN, fmt, ##arg);}while(0)	
	#define LOG_INFO(fmt, arg...) do{ if(GlobalLogLevel & Log_INFO) LogToFile(Log_INFO, fmt, ##arg);}while(0)	
	#define LOG_DEBUG(fmt, arg...) do{ if(GlobalLogLevel & Log_DEBUG) LogToFile(Log_DEBUG, fmt, ##arg);}while(0)
	#define Log_PLATFORM(fmt, arg...) do{ if(GlobalLogLevel & Log_PLATFORM) LogToFile(Log_PLATFORM, fmt, ##arg);}while(0)
#endif

#endif

