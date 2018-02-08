#ifndef __INCLUDE_PRINT_LOG_H__
#define __INCLUDE_PRINT_LOG_H__

#include "Def.h"
#include "Mutex.h"
#include <stdio.h>

#define LogDebug( fmt, ... ) CPrintLog::PrintDebug( __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__ )
#define LogInfo( fmt, ... ) CPrintLog::PrintInfo( __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__ )
#define LogError( fmt, ... ) CPrintLog::PrintError( __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__ )

#define LogPoint() CPrintLog::PrintDebug( __FILE__, __FUNCTION__, __LINE__, "\n" )

class CPrintLog
{
public:
	//打开日志文件，打开日志文件后将会将日志信息记录到相应的日志文件中
	//参数：file_name：文件名前缀，file_name_年月日_时分秒.log
	//返回值：>=0成功，<0失败
	static int OpenLogFile( const char* file_name, const char* path = NULL );
	//关闭日志文件
	static void CloseLogFile();
	//设置日志文件最大大小，超过该大小则换一文件，默认1Mb
	//参数：size：日志文件大小，单位Kb
	static void SetLogFileSize( int size );
	enum{
		DEBUG_LEVEL = 0,	//该级别输出Debug、Info、Error信息
		INFO_LEVEL,			//该级别输出Info、Error信息
		ERROR_LEVEL,		//该级别输出Error信息
		NONE_LEVEL,			//该级别不输出任何信息
	};
	//设置打印级别
	//参数：level：打印级别
	static void SetPrintLevel( int level );
	//打印信息携带时间、文件名、函数名、行号、打印级别、日志信息
	static void PrintDebug( const char* file, const char* fun, int line, const char* fmt, ... );
	static void PrintInfo( const char* file, const char* fun, int line, const char* fmt, ... );
	static void PrintError( const char* file, const char* fun, int line, const char* fmt, ... );
	//打印信息仅携带日志信息
	static void PrintDebugSimple( const char* fmt, ... );
	static void PrintInfoSimple( const char* fmt, ... );
	static void PrintErrorSimple( const char* fmt, ... );
private:
	static void print_log( const char* file, const char* fun, int line, const char* name, const char* buf );
	static void print_log_simple( const char* buf );
	static int open_file();
private:
	static FILE* m_log_file;
	static int m_log_level;
	static CMutex m_mutex;
	static int m_max_file_size;
	static char m_file_name[128];
	static char m_path[128];
};

#endif
