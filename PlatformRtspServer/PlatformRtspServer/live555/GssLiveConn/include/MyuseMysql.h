#ifndef _MYUSEMYSQL_H_
#define _MYUSEMYSQL_H_

#include "PublicMysql.h"
#include <string>
#include <vector>
#include <map>

enum {
	e_table_device_rec_col_id = 0, //自增id
	e_table_device_rec_col_guid, //设备ID
	e_table_device_rec_col_time, //播放时长
	e_table_device_rec_col_starttime, //起始时间
	e_table_device_rec_col_rtsp, //rtsp播放时长
	e_table_device_rec_col_hls, //hls播放时长
	e_table_device_rec_col_counts,
};

class MyuseMysql
{
public:
	MyuseMysql(PublicMySql *psql);
	~MyuseMysql();

	//////////////////////////////////////////////////////////////////////////
	//rtsp/hls 单个guid时长
	bool QueryTimesByGuid(const char* tablename, const char* guid, int & timeSec, unsigned int & starttime, int nCol, int &nColValue);
	bool UpdateTimesByGuid(const char* tablename, const char* guid, int timeSec, unsigned int starttime, int nCol, int nColValue); //update时，没有记录，会插入该记录
	bool ResetTimesByGuid(const char* tablename, const char* guid, int timeSec, unsigned int starttime, int nCol, int nColValue);
	bool InsertTimesByGuid(const char* tablename, const char* guid, int timeSec, unsigned int starttime, int nCol, int nColValue);
	//////////////////////////////////////////////////////////////////////////

private:
	PublicMySql *m_sql;
};

#endif