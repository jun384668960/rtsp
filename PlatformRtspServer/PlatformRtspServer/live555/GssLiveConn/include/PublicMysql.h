#ifndef _PUBLICMYSQL_H
#define _PUBLICMYSQL_H

#include <mysql/mysql.h>
#include <pthread.h>
#include <vector>
#include <string>
#include "MyClock.h"

class PublicMySql{
public:
	PublicMySql();
	virtual ~PublicMySql();
	
	bool Init(const char* host, int port,const char* user, const char* passwd,const char* dbname);
	bool UnInit();
	bool OpenDb();
	bool CloseDb();
	bool IsDbOpen();

	bool GetRowsCols(int &nrows, int &ncols);
	const char* GetResult(int nCol,int nRow);
// protected:
	bool ExeSql(const char* stmt);
	//mysql 默认是自动提交，当修改为非自动提交，每次插入，更新操作之后需要手动提交，不然操作失效
	//开启，禁止自动提交功能是针对单次连接有效
	bool EnableAutoCommit(bool bEnable);
	bool IsEnableAutoCommit() { return m_bEnableAutoCommit; }
	bool BeginTransaction();
	bool CommitTransaction();
	bool RollbackTransaction();
// 	bool GetLastInsertMsgId(int &msgid);
	bool KeepAlive();
private:
	bool Lock();
	bool Unlock();

protected:
	MYSQL *m_psql;
	char m_host[128];
	char m_user[64];
	char m_passwd[64];
	char m_dbname[128];
	int m_port; //3306
	bool m_bInited;
	bool m_bOpenedDb;
	static MyClock m_sMutex;
	bool m_bEnableAutoCommit;
	std::vector<std::string> m_results;
	MyClock m_resultsLock;
	int m_row;
	int m_col;
};

#endif