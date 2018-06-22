#include "PublicMysql.h"
#include "Log.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


MyClock PublicMySql::m_sMutex;

PublicMySql::PublicMySql()
{
	memset(m_host,0,sizeof(m_host));
	strcpy(m_host,"127.0.0.1");
	memset(m_user,0,sizeof(m_user));
	memset(m_passwd,0,sizeof(m_passwd));
	memset(m_dbname,0,sizeof(m_dbname));
	m_port = 3306;
	m_psql = NULL;
	m_bInited = false;
	m_bOpenedDb = false;
	m_bEnableAutoCommit = true;
}

PublicMySql::~PublicMySql()
{
	UnInit();
}

bool PublicMySql::Init( const char* host, int port,const char* user, const char* passwd,const char* dbname)
{
	bool bsuc = false;
	do 
	{
		if (host == NULL || user == NULL || passwd == NULL || dbname == NULL)
		{
			LOG_ERROR("input invalid param!host:%s, port:%d, user:%s, dbname:%s \n",host,port,user,dbname);
			break;
		}
		memset(m_host,0,sizeof(m_host));
		strcpy(m_host,host);
		memset(m_user,0,sizeof(m_user));
		strcpy(m_user,user);
		memset(m_passwd,0,sizeof(m_passwd));
		strcpy(m_passwd,passwd);
		m_port = port;
		memset(m_dbname,0,sizeof(m_dbname));
		strcpy(m_dbname,dbname);
		Lock();
		m_psql = mysql_init(NULL);
		Unlock();
		if(m_psql == NULL)
		{
			LOG_ERROR("mysql_init failed!\n");
			break;
		}
		bsuc = true;
		m_bInited = true;
	} while (0);
	return bsuc;
}

bool PublicMySql::UnInit()
{
	bool bsuc = CloseDb();
	memset(m_host,0,sizeof(m_host));
	memset(m_user,0,sizeof(m_user));
	memset(m_passwd,0,sizeof(m_passwd));
	memset(m_dbname,0,sizeof(m_dbname));
	m_bInited = false;
	return bsuc;
}

bool PublicMySql::OpenDb()
{
	bool bsuc = false;
	
	do 
	{
		if(!m_bInited)
			break;
		if(m_psql == NULL)
		{
			Lock();
			m_psql = mysql_init(NULL);
			Unlock();
			if(m_psql == NULL)
			{
				LOG_ERROR("mysql_init failed\n");
				break;
			}
		}
		else
		{
			printf("mysql is init!\n");
		}
		my_bool bEnableReccnt = 1;
		if( mysql_options(m_psql,MYSQL_OPT_RECONNECT,(const void*)&bEnableReccnt) != 0 )
			break;
		MYSQL* psql = mysql_real_connect(m_psql,m_host,m_user,m_passwd,m_dbname,m_port,NULL,0);
		if(psql == NULL)
		{
			LOG_ERROR("Failed to connect to database : error %s\n",mysql_error(m_psql));
			break;
		}
		if (!mysql_set_character_set(psql, "utf8"))
		{
			LOG_ERROR("New client character set: %s\n",mysql_character_set_name(psql));
		}
		m_bOpenedDb = true;
		bsuc = true;
	} while (0);

	return bsuc;
}

bool PublicMySql::CloseDb()
{
	if(m_psql)
	{
		mysql_close(m_psql);
		m_psql = NULL;
	}
	m_bOpenedDb = false;
	return true;
}

bool PublicMySql::Lock()
{
	return PublicMySql::m_sMutex.Lock();
}

bool PublicMySql::Unlock()
{
	return PublicMySql::m_sMutex.Unlock();
}

bool PublicMySql::ExeSql( const char* stmt )
{
	if (stmt == NULL || m_psql == NULL || !m_bOpenedDb)
	{
		return false;
	}
	m_resultsLock.Lock();
	m_results.clear();
	m_resultsLock.Unlock();
	LOG_INFO("current to execute sql = %s\n",stmt);
	if(mysql_real_query(m_psql,stmt,strlen(stmt)) != 0)
	{
		LOG_ERROR("mysql_real_query : error %s\n",mysql_error(m_psql));
		return false;
	}
	else
	{
		LOG_DEBUG("mysql_real_query execute success!\n");
		return true;
	}
}

bool PublicMySql::IsDbOpen()
{
	return m_bOpenedDb;
}

bool PublicMySql::EnableAutoCommit(bool bEnable)
{
	if(m_bOpenedDb)
	{
		my_bool mode = bEnable;
		if(mysql_autocommit(m_psql,mode) == 0)	//当mode == 0，关闭mysql的自动提交功能，mysql默认自动提交
		{
			LOG_DEBUG("success to disable auto commit!\n");
			m_bEnableAutoCommit = bEnable;
			return true;
		}
		else
		{
			LOG_ERROR("Failed to disable auto commit: error %s\n",mysql_error(m_psql));
		}
	}
	return false;
}

bool PublicMySql::BeginTransaction()
{
	bool bSuc = false;
	do 
	{
		if(m_psql == NULL)
			break;
		const char *pBegin = "begin";
		bSuc = ExeSql(pBegin);
	} while (0);
	return bSuc;
}

bool PublicMySql::CommitTransaction()
{
	bool bSuc = false;
	do 
	{
		if(m_psql == NULL)
			break;
		const char *pCommit = "commit";
		bSuc = ExeSql(pCommit);
	} while (0);
	return bSuc;
}

bool PublicMySql::RollbackTransaction()
{
	bool bSuc = false;
	do 
	{
		if(m_psql == NULL)
			break;
		const char *pRollback = "rollback";
		bSuc = ExeSql(pRollback);
		if(bSuc)
			LOG_DEBUG("rollback success!\n");
		else
			LOG_ERROR("rollback failed!\n");
	} while (0);
	return bSuc;
}
// 
// bool PublicMySql::GetLastInsertMsgId( int &msgid )
// {
// 	bool bsuc = false;
// 	do 
// 	{
// 		if(m_psql == NULL)
// 			break;
// 		const char* pSql = "select last_insert_id()";
// 		if(!ExeSql(pSql))
// 			break;
// 		MYSQL_RES *res = mysql_store_result(m_psql);
// 		if(res == NULL)
// 		{
// 			printf("GetLastInsertMsgId,mysql_store_result : error %s\n",mysql_error(m_psql));
// 			break;
// 		}
// 		LOG_DEBUG("GetLastInsertMsgId mysql_store_result success\n");
// 		my_ulonglong num = mysql_num_rows(res);
// 		if(num > 0)
// 		{
// 			MYSQL_ROW row = mysql_fetch_row(res);
// 			if(row == NULL)
// 			{
// 				mysql_free_result(res);
// 				break;
// 			}
// 			msgid = atoi(row[0]);
// 			mysql_free_result(res);
// 		}
// 		else
// 		{
// 			mysql_free_result(res);
// 			break;
// 		}
// 
// 		bsuc = true;
// 	} while (0);
// 	return bsuc;
// }

const char* PublicMySql::GetResult( int nCol,int nRow )
{
	if (nCol < m_col && nRow < m_row)
	{
		if ((nRow * m_col + nCol) < (int)m_results.size())
		{
			return m_results[nRow * m_col + nCol].c_str();
		}
		else
		{
			LOG_ERROR("GetResult FAILED,input nCol = %d,nRow = %d,m_results.size = %d",nCol,nRow,m_results.size());
			return "";
		}
	}
	else
	{
		LOG_ERROR("GetResult FAILED,input nCol = %d,nRow = %d,m_col = %d,m_row = %d",nCol,nRow,m_col,m_row);
		return "";
	}
}

bool PublicMySql::GetRowsCols( int &nrows, int &ncols )
{
	bool bsuc = false;
	MYSQL_RES *res = mysql_store_result(m_psql);
	if(res)
	{
		nrows = (int)mysql_num_rows(res);
		ncols = (int)mysql_num_fields(res);
		
		m_resultsLock.Lock();
		m_results.clear();
		//if(m_results.size() <= 0)
		nrows = 0;
		{
			MYSQL_ROW row;
			while ((row = mysql_fetch_row(res)) != NULL)
			{
				for (int i = 0; i < ncols; i++)
				{
					if(row[i] == NULL)
						m_results.push_back("");
					else
						m_results.push_back(row[i]);
				}
				nrows++;
			}
		}
		m_resultsLock.Unlock();

		mysql_free_result(res);
		
		m_row = nrows;
		m_col = ncols;

		if (m_row*m_col != (int)m_results.size() || (int)m_results.size() == 0)
		{
			LOG_ERROR("Query result: row = %d, col = %d, result size = %u",m_row,m_col,m_results.size());
			bsuc = false;
		}
		else
		{
			bsuc = true;
		}
		return bsuc;
	}
	else // mysql_store_result() returned nothing; should it have?
	{
		if(mysql_field_count(m_psql) == 0)
		{
			// query does not return data
			// (it was not a SELECT)
			nrows = 1;
			ncols = 1;
			m_row = 1;
			m_col = 1;

			m_resultsLock.Lock();
			m_results.clear();
			//if(m_results.size() <= 0)
			{
				char pTmp[128] = {0};
				sprintf(pTmp,"%ld",(long)mysql_affected_rows(m_psql));
				m_results.push_back(pTmp);
			}
			m_resultsLock.Unlock();

			if (m_row*m_col != (int)m_results.size() || (int)m_results.size() == 0)
			{
				LOG_ERROR("Query result: mysql_store_result() returned nothing ");
				bsuc = false;
			}
			else
			{
				bsuc = true;
			}
		}
		else // mysql_store_result() should have returned data
		{
			LOG_INFO("GetRows Error: %s\n", mysql_error(m_psql));
			bsuc = false;
		}
		return bsuc;
	}
	m_row = 0;
	m_col = 0;
	return false;
}

bool PublicMySql::KeepAlive()
{
	const char* psql = "show tables";
	if(ExeSql(psql))
	{
		MYSQL_RES *res = mysql_store_result(m_psql);
		if(res)
		{
			mysql_free_result(res);
		}
	}

	return true;
}