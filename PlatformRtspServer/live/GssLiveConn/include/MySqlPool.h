
#ifndef _MYSQL_POOL_H_
#define _MYSQL_POOL_H_

#include "PublicMysql.h"
#include "MyClock.h"
#include "Log.h"

#include <string>
#include <list>


class MySqlPool
{
public:
	MySqlPool(std::string ipaddr,int port,std::string username,std::string password,std::string dbname,int maxconns);
	~MySqlPool();

	int InitPool(int conns); //初始化连接池
	void ReleasePoll(); //销毁连接池

	PublicMySql* GetConnection(); //获取一个连接
	void ReleaseConnection(PublicMySql* pconn); //释放一个连接
	int GetCurConnections() { return m_curSize; }
	int GetMaxSize() { return m_maxSize; }
protected:
	PublicMySql* CreateConnection(); //创建一个连接
	void DestroyConnection(PublicMySql* pconn); //销毁一个连接


private:
	int m_curSize; //当前已建立的数据库连接数量
	int m_maxSize; //连接池中定义的最大数据库连接数
	std::list<PublicMySql*> m_connList; //连接池队列
	MyClock m_connListLock;	//连接池队列锁
	std::string m_username;
	std::string m_password;
	std::string m_ipaddr;
	std::string m_dbname;
	int m_port;
};

#endif //_MYSQL_POOL_H_