#include "PrintLog.h"
#include <time.h>
#include <string.h>
#include <stdarg.h>

FILE* CPrintLog::m_log_file = NULL;
int CPrintLog::m_log_level = CPrintLog::DEBUG_LEVEL;
CMutex CPrintLog::m_mutex;
int CPrintLog::m_max_file_size = 1024*1024;
char CPrintLog::m_file_name[128];
char CPrintLog::m_path[128];

int CPrintLog::OpenLogFile( const char* file_name, const char* path )
{
	CGuard lock( m_mutex );
	memset( m_file_name, 0, sizeof(m_file_name) );
	strncpy( m_file_name, file_name, sizeof(m_file_name)-1 );
	if( path != NULL )
		strncpy( m_path, path, sizeof(m_path)-1 );
	else
		strncpy( m_path, "./", sizeof(m_path)-1 );
	return open_file();
}

void CPrintLog::CloseLogFile()
{
	CGuard lock( m_mutex );
	if( m_log_file != NULL )
		fclose( m_log_file ), m_log_file = NULL;
}

void CPrintLog::SetLogFileSize( int size )
{
	if( size <= 0 )
		return;
	m_max_file_size = size*1024;
}

void CPrintLog::SetPrintLevel( int level )
{
	CGuard lock( m_mutex );
	if( level >= DEBUG_LEVEL && level <= NONE_LEVEL )
		m_log_level = level;
}

void CPrintLog::PrintDebug( const char* file, const char* fun, int line, const char* fmt, ... )
{
	CGuard lock( m_mutex );
	if( m_log_level <= DEBUG_LEVEL ){
		char buf[1024] = "";
		va_list ap;
		va_start( ap, fmt );
		_vsnprintf( buf, sizeof(buf), fmt, ap );
		va_end(ap);
		print_log( file, fun, line, "debug", buf );
	}
}

void CPrintLog::PrintInfo( const char* file, const char* fun, int line, const char* fmt, ... )
{
	CGuard lock( m_mutex );
	if( m_log_level <= INFO_LEVEL ){
		char buf[1024] = "";
		va_list ap;
		va_start( ap, fmt );
		_vsnprintf( buf, sizeof(buf), fmt, ap );
		va_end(ap);
		print_log( file, fun, line, "info", buf );
	}
}

void CPrintLog::PrintError( const char* file, const char* fun, int line, const char* fmt, ... )
{
	CGuard lock( m_mutex );
	if( m_log_level <= ERROR_LEVEL ){
		char buf[1024] = "";
		va_list ap;
		va_start( ap, fmt );
		_vsnprintf( buf, sizeof(buf), fmt, ap );
		va_end(ap);
		print_log( file, fun, line, "error", buf );
	}
}

void CPrintLog::PrintDebugSimple( const char* fmt, ... )
{
	CGuard lock( m_mutex );
	if( m_log_level <= DEBUG_LEVEL ){
		char buf[1024] = "";
		va_list ap;
		va_start( ap, fmt );
		_vsnprintf( buf, sizeof(buf), fmt, ap );
		va_end(ap);
		print_log_simple( buf );
	}
}

void CPrintLog::PrintInfoSimple( const char* fmt, ... )
{
	CGuard lock( m_mutex );
	if( m_log_level <= INFO_LEVEL ){
		char buf[1024] = "";
		va_list ap;
		va_start( ap, fmt );
		_vsnprintf( buf, sizeof(buf), fmt, ap );
		va_end(ap);
		print_log_simple( buf );
	}
}

void CPrintLog::PrintErrorSimple( const char* fmt, ... )
{
	CGuard lock( m_mutex );
	if( m_log_level <= ERROR_LEVEL ){
		char buf[1024] = "";
		va_list ap;
		va_start( ap, fmt );
		_vsnprintf( buf, sizeof(buf), fmt, ap );
		va_end(ap);
		print_log_simple( buf );
	}
}

void CPrintLog::print_log( const char* file, const char* fun, int line, const char* name, const char* buf )
{
	time_t rawtime;
	struct tm* sys_time = NULL;
	time( &rawtime );
	sys_time = localtime ( &rawtime );
	if( sys_time != NULL )
		printf( "%02d%02d%02d-%02d%02d%02d|%s:%s:%d|%s %s", 
			(sys_time->tm_year+1900)%100, sys_time->tm_mon+1 ,sys_time->tm_mday, sys_time->tm_hour, sys_time->tm_min, sys_time->tm_sec, 
			file, fun, line, name, buf );
	else
		printf( "--|%s:%s:%d|%s %s", file, fun, line, name, buf );
	if( m_log_file != NULL ){
		if( sys_time != NULL )
			fprintf( m_log_file, "%02d%02d%02d-%02d%02d%02d|%s:%s:%d|%s %s", 
				(sys_time->tm_year+1900)%100, sys_time->tm_mon+1 ,sys_time->tm_mday, sys_time->tm_hour, sys_time->tm_min, sys_time->tm_sec, 
				file, fun, line, name, buf );
		else
			fprintf( m_log_file, "--|%s:%s:%d|%s %s", file, fun, line, name, buf );
		fflush( m_log_file );
		if( ftell( m_log_file ) >= m_max_file_size )
			open_file();
	}
}

void CPrintLog::print_log_simple( const char* buf )
{
	printf( "%s", buf );
	if( m_log_file != NULL ){
		fprintf( m_log_file, "%s", buf );
		fflush( m_log_file );
		if( ftell( m_log_file ) >= m_max_file_size )
			open_file();
	}
}

int CPrintLog::open_file()
{
	time_t rawtime;
	struct tm* sys_time = NULL;
	time( &rawtime );
	sys_time = localtime ( &rawtime );
	char file_name[512];
	memset( file_name, 0, sizeof(file_name) );
	if( sys_time != NULL )
		_snprintf( file_name, sizeof(file_name), "%s/%s_%04d%02d%02d-%02d%02d%02d.log", m_path, m_file_name, 
			sys_time->tm_year+1900, sys_time->tm_mon+1 ,sys_time->tm_mday, sys_time->tm_hour, sys_time->tm_min, sys_time->tm_sec );
	else
		_snprintf( file_name, sizeof(file_name), "%s/%s_--.log", m_path, m_file_name );
	m_log_file = fopen( file_name, "w" );
	return ( m_log_file == NULL ? -1 : 0 );
}
