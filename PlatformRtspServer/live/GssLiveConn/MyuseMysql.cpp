#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MyuseMysql.h"
#include "Log.h"
#include "tm.h"


#define MYSQL_USER		"root"
#define MYSQL_PSW		"goscam66%%DB"
#define MYSQL_PORT		3306
#define MYSQL_DB			"goscam_ulife_db"
#define TABLE_NAME_DEVINFO		"gs_device_info_db"
#define TABLE_COL_DEVID		"device_id"
#define TABLE_COL_SOFTWARE_VERSION	"software_ver"

#define TABLE_NAME_APP_UPGRADE	"gs_app_upgrade"
#define TABLE_COL_PACKAGENAME "packagename"
#define TABLE_COL_VERSIONNUMBER "vername"
enum
{
	e_talbe_app_upgrade_col_id = 0, //自增id
	e_talbe_app_upgrade_col_packagename, //包名
	e_talbe_app_upgrade_col_appname, //app name
	e_talbe_app_upgrade_col_vernum, //版本号,一个int值,用于比较版本大小的
	e_talbe_app_upgrade_col_vername, //版本号,字符串,Ex:6.0.6
	e_talbe_app_upgrade_col_packageurl, //包的url
	e_talbe_app_upgrade_col_md5, //包的md5校验
	e_talbe_app_upgrade_col_updatedes, //该版本的描述信息
};

MyuseMysql::MyuseMysql(PublicMySql *psql)
{
	m_sql = psql;
}

MyuseMysql::~MyuseMysql()
{

}

//////////////////////////////////////////////////////////////////////////
//rtsp/hls 单个guid时长
// enum {
// 	e_table_device_rec_col_id = 0, //自增id
// 	e_table_device_rec_col_guid, //设备ID
// 	e_table_device_rec_col_time, //播放时长
// 	e_table_device_rec_col_starttime, //起始时间
// 	e_table_device_rec_col_rtsp, //rtsp播放时长
// 	e_table_device_rec_col_hls, //hls播放时长
// 	e_table_device_rec_col_counts,
// };

char g_tableDevRec[e_table_device_rec_col_counts][20] = {
	"id",
	"guid",
	"times",
	"starttime",
	"rtsp",
	"hls"
};

/*
create table device_rec(
id BIGINT NOT NULL AUTO_INCREMENT,
guid VARCHAR(64) NOT NULL,
times VARCHAR(64) NOT NULL,
starttime int unsigned not null,
rtsp VARCHAR(64),
hls VARCHAR(64),
PRIMARY KEY(id),
UNIQUE KEY(guid));
*/

bool MyuseMysql::QueryTimesByGuid(const char* tablename, const char* guid, int & timeSec, unsigned int & starttime, int nCol, int &nColValue)
{
	bool bSuc = false;
	do 
	{
		if (tablename  == NULL || guid == NULL || nCol < 0 || nCol >= e_table_device_rec_col_counts)
		{
			LOG_ERROR("MyuseMysql::QueryTimesByGuid invalid params!, tablename = %s, guid = %s\n",tablename, guid);
			break;
		}

		char pSql[256] = {0};
		snprintf(pSql,sizeof(pSql),"select %s,%s,%s from %s where %s = \'%s\'",g_tableDevRec[e_table_device_rec_col_time],g_tableDevRec[e_table_device_rec_col_starttime],
			g_tableDevRec[nCol],
			tablename,g_tableDevRec[e_table_device_rec_col_guid],guid);
		if(!m_sql->ExeSql(pSql))
		{
			LOG_ERROR("exe sql failed : %s",pSql);
			break;
		}

		int nCol = 0,nRow = 0;
		if(m_sql->GetRowsCols(nRow,nCol) && nRow > 0 && nCol > 0)
		{
			//因为有唯一束缚，只可能有一行
			timeSec = atoi(m_sql->GetResult(0,0));
			if(nCol > 1)
			{
				sscanf(m_sql->GetResult(1,0),"%u",&starttime);
				nColValue = atoi(m_sql->GetResult(2,0));
			}
		}
		else
		{
			//没有该记录，就更新一条记录到数据里面
			LOG_ERROR("Query From db success,but no result,exe sql = %s\n",pSql);
			timeSec = 0;
			nColValue = 0;
			starttime = now_ms_time()/1000;
			if( !UpdateTimesByGuid(tablename, guid, timeSec, starttime, nCol, nColValue) )
				break;
		}

		bSuc = true;

	} while (false);

	return bSuc;
}

bool MyuseMysql::ResetTimesByGuid(const char* tablename, const char* guid, int timeSec, unsigned int starttime, int nCol, int nColValue)
{
	bool bSuc = false;
	do 
	{
		if (tablename  == NULL || nCol < 0 || nCol >= e_table_device_rec_col_counts)
		{
			LOG_ERROR("MyuseMysql::QueryTimesByGuid invalid params!, tablename = %s, guid = %s\n",tablename, guid);
			break;
		}

		char pSql[256] = {0};
		char timestr[10] = {0};
		char starttimestr[64] = {0};
		snprintf(timestr,sizeof(timestr),"%d",timeSec);
		snprintf(starttimestr,sizeof(starttimestr),"%u",starttime);
		if(starttime == 0) //
		{
			if(guid != NULL)
			{
				snprintf(pSql,sizeof(pSql),"update %s set %s = \'%s\',%s=%d where %s = \'%s\'",tablename, 
					g_tableDevRec[e_table_device_rec_col_time],timestr,
					g_tableDevRec[nCol],nColValue,
					g_tableDevRec[e_table_device_rec_col_guid],guid);
			}
			else
			{
				snprintf(pSql,sizeof(pSql),"update %s set %s = \'%s\',%s=%d",tablename, 
					g_tableDevRec[e_table_device_rec_col_time],timestr,
					g_tableDevRec[nCol],nColValue);
			}
		}
		else
		{
			if(guid != NULL)
			{
				snprintf(pSql,sizeof(pSql),"update %s set %s = \'%s\', %s=\'%s\',%s=%d where %s = \'%s\'",tablename, 
					g_tableDevRec[e_table_device_rec_col_time],timestr,
					g_tableDevRec[e_table_device_rec_col_starttime],starttimestr,
					g_tableDevRec[nCol],nColValue,
					g_tableDevRec[e_table_device_rec_col_guid],guid);
			}
			else
			{
				snprintf(pSql,sizeof(pSql),"update %s set %s = \'%s\', %s=\'%s\',%s=%d",tablename, 
					g_tableDevRec[e_table_device_rec_col_time],timestr,
					g_tableDevRec[e_table_device_rec_col_starttime],starttimestr,
					g_tableDevRec[nCol],nColValue);
			}
			
		}
		if(!m_sql->ExeSql(pSql))
		{
			LOG_ERROR("exe sql failed : %s",pSql);
			break;
		}
		
		bSuc = true;
	} while (false);

	return bSuc;

}

bool MyuseMysql::UpdateTimesByGuid(const char* tablename, const char* guid, int timeSec, unsigned int starttime, int nCol, int nColValue)
{
	bool bSuc = false;
	do 
	{
		if (tablename  == NULL || guid == NULL || nCol < 0 || nCol >= e_table_device_rec_col_counts)
		{
			LOG_ERROR("MyuseMysql::QueryTimesByGuid invalid params!, tablename = %s, guid = %s\n",tablename, guid);
			break;
		}

		char pSql[256] = {0};
		char timestr[10] = {0};
		char starttimestr[64] = {0};
		snprintf(timestr,sizeof(timestr),"%d",timeSec);
		snprintf(starttimestr,sizeof(starttimestr),"%u",starttime);
		if(starttime == 0) //
		{
			snprintf(pSql,sizeof(pSql),"update %s set %s = \'%s\',%s=%d where %s = \'%s\'",tablename, 
				g_tableDevRec[e_table_device_rec_col_time],timestr,
				g_tableDevRec[nCol],nColValue,
				g_tableDevRec[e_table_device_rec_col_guid],guid);
		}
		else
		{
			snprintf(pSql,sizeof(pSql),"update %s set %s = \'%s\', %s=\'%s\',%s=%d where %s = \'%s\'",tablename, 
				g_tableDevRec[e_table_device_rec_col_time],timestr,
				g_tableDevRec[e_table_device_rec_col_starttime],starttimestr,
				g_tableDevRec[nCol],nColValue,
				g_tableDevRec[e_table_device_rec_col_guid],guid);
		}
		if(!m_sql->ExeSql(pSql))
		{
			LOG_ERROR("exe sql failed : %s",pSql);
			break;
		}

		int nCol = 0,nRow = 0;
		if(m_sql->GetRowsCols(nRow,nCol) && nRow > 0 && nCol > 0)
		{
			if(atoi(m_sql->GetResult(0,0)) == 0) //表示更新对象有0个受影响，即数据库中没有该项
				bSuc = InsertTimesByGuid(tablename,guid,timeSec,starttime,nCol,nColValue);
			else
				bSuc = true;
		}
		else
		{
			//更新记录失败,没有该记录，就插入一条记录到数据里面
			LOG_ERROR("Update to db success,but no result,exe sql = %s\n",pSql);
			break;
		}

	} while (false);

	return bSuc;
}

bool MyuseMysql::InsertTimesByGuid(const char* tablename, const char* guid, int timeSec, unsigned int starttime, int nCol, int nColValue)
{
	bool bSuc = false;
	do 
	{
		if (tablename  == NULL || guid == NULL || nCol < 0 || nCol >= e_table_device_rec_col_counts)
		{
			LOG_ERROR("MyuseMysql::InsertTimesByGuid invalid params!, tablename = %s, guid = %s\n",tablename, guid);
			break;
		}

		char pSql[256] = {0};
		char timestr[10] = {0};
		char starttimestr[64] = {0};
		snprintf(timestr,sizeof(timestr),"%d",timeSec);
		snprintf(starttimestr,sizeof(starttimestr),"%u",starttime);
		snprintf(pSql,sizeof(pSql),"insert into %s(id,guid,times,starttime,%s) values(0,\'%s\',\'%s\',\'%s\',%d)",g_tableDevRec[nCol],tablename, guid, timestr,starttimestr,nColValue);
		if(!m_sql->ExeSql(pSql))
		{
			LOG_ERROR("exe sql failed : %s",pSql);
			break;
		}

		int nCol = 0,nRow = 0;
		if(m_sql->GetRowsCols(nRow,nCol) && nRow > 0 && nCol > 0)
		{
			bSuc = true;
		}
		else
		{
			//更新记录失败,没有该记录，就插入一条记录到数据里面
			LOG_ERROR("Insert into db success,but no result,exe sql = %s\n",pSql);
			break;
		}

		bSuc = true;
	} while (false);

	return bSuc;
}
