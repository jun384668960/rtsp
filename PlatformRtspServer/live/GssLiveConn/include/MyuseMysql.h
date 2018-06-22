#ifndef _MYUSEMYSQL_H_
#define _MYUSEMYSQL_H_

#include "PublicMysql.h"
#include <string>
#include <vector>
#include <map>

class MyuseMysql
{
public:
	MyuseMysql(PublicMySql *psql);
	~MyuseMysql();

	//////////////////////////////////////////////////////////////////////////
	//rtsp/hls 单个guid时长
	bool QueryTimesByGuid(const char* tablename, const char* guid, int & timeSec, unsigned int & starttime);
	bool UpdateTimesByGuid(const char* tablename, const char* guid, int timeSec, unsigned int starttime); //update时，没有记录，会插入该记录
	bool InsertTimesByGuid(const char* tablename, const char* guid, int timeSec, unsigned int starttime);
	//////////////////////////////////////////////////////////////////////////

private:
	PublicMySql *m_sql;
};

#endif