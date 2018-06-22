#include "MySqlPool.h"
#include "Log.h"

#include <stdio.h>
#include <string.h>

MySqlPool::MySqlPool( std::string ipaddr,int port,std::string username,std::string password,std::string dbname,int maxconns )
{
	m_maxSize = maxconns;
	m_curSize = 0;
	m_port = port;
	m_ipaddr = ipaddr;
	m_username = username;
	m_password = password;
	m_dbname = dbname;
	//InitPool(m_maxSize/2);
}

MySqlPool::~MySqlPool()
{
	ReleasePoll();
}

int MySqlPool::InitPool( int conns )
{
	m_connListLock.Lock();
	for (int i = 0; i < conns; i++)
	{
		PublicMySql* temp = CreateConnection();
		if(temp)
		{
				m_connList.push_back(temp);
		}
		else
		{
			LOG_ERROR("create connection to mysql failed!\n");
		}
	}
	m_connListLock.Unlock();
	if (m_connList.size() > 0)
	{
		LOG_INFO("Mysqlpool init success!");
		return 0;
	}
	else
	{
		LOG_ERROR("Mysqlpool init failed XXXXXXXXXXXXXXXXXXXXXXX!");
		return -1;
	}
}

void MySqlPool::ReleasePoll()
{
	m_connListLock.Lock();
	size_t size = m_connList.size();
	for (; size > 0; size--)
	{
		PublicMySql* tempsql = m_connList.front();
		m_connList.pop_front();
		DestroyConnection(tempsql);
	}
	m_connListLock.Unlock();
}

PublicMySql* MySqlPool::GetConnection()
{
	PublicMySql* pSql = NULL;
	m_connListLock.Lock();
	size_t size = m_connList.size();
	if (size <= 0 && m_curSize >= m_maxSize)
	{
		//已经达到连接池的最大连接数，并且所有连接已经被使用，返回NULL
		LOG_INFO("Get one connection from pool failed, because of there is no connection in pool and is reached the maximum.");
		pSql = NULL;
	}
	else
	{
		if(size > 0)
		{
			pSql = m_connList.front();
			m_connList.pop_front();
		}
		else
		{
			pSql = CreateConnection();
			if(pSql)
			{
				m_connList.push_back(pSql);
			}
			else
			{
				LOG_ERROR("GetConnection ,the pool is empty, then  CreateConnection, but failed!");
			}
		}
	}
	m_connListLock.Unlock();
	return pSql;
}

void MySqlPool::ReleaseConnection( PublicMySql* pconn )
{
	if(pconn)
	{
		m_connListLock.Lock();
		m_connList.push_back(pconn);
		m_connListLock.Unlock();
	}
}

PublicMySql* MySqlPool::CreateConnection()
{
	bool bNewOk = false;
	PublicMySql* usemysql = new PublicMySql();
	if (usemysql)
	{
		if(usemysql->Init(m_ipaddr.c_str(),m_port,m_username.c_str(),m_password.c_str(),m_dbname.c_str()))
		{
			if(usemysql->OpenDb())
			{
				bNewOk = true;
			}
			else
			{
				LOG_ERROR("InitPool ,new PublicMySql success,but open database failed!");
			}
		}
		else
		{
			LOG_ERROR("InitPool ,new PublicMySql success,but init failed!");
		}
	}
	else
	{
		LOG_ERROR("InitPool ,new PublicMySql failed!");
	}
	if(!bNewOk)
	{
		delete usemysql;
		usemysql = NULL;
		return NULL;
	}
	m_curSize++;
	return usemysql;
}

void MySqlPool::DestroyConnection( PublicMySql* pconn )
{
	if (pconn)
	{
		pconn->CloseDb();
		delete pconn;
		m_curSize--;
	}
}