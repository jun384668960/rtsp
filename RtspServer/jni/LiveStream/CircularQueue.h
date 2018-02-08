/********************************************************************
filename: 	CircularQueue.h
created:	2016-01-13
author:		donyj

purpose:
            
			

!!!important:	
*********************************************************************/
#pragma once
#include <stdio.h>
#include <pthread.h>

#define DEFAULT_MAX_FRAME_NUM	10

template<class T>
class CCircularQueue
{
public:
	
	CCircularQueue();
	CCircularQueue(int nQueueSize);
	~CCircularQueue(void);
public:
	bool PushBack(const T elem);	
	bool PopFront(T& elem, int &unHandleCnt);

	void SyncRwPoint();
		
	int Size();
	int RemainSize();
	bool IsEmpty();
	bool IsFull();

private:
	T *m_pQueueBase;
	int m_QueueSize;            
	int m_Head;
	int m_Tail;         
	pthread_mutex_t m_csRW;

};


template< class T >
CCircularQueue<T>::CCircularQueue():
m_pQueueBase(NULL),
m_QueueSize(DEFAULT_MAX_FRAME_NUM),
m_Head(0),
m_Tail(0)

{
	m_pQueueBase = new T[m_QueueSize];
	pthread_mutex_init(&m_csRW,NULL);
}

template< class T >
CCircularQueue<T>::CCircularQueue(int nQueueSize):
m_pQueueBase(NULL),
m_QueueSize(nQueueSize),
m_Head(0),
m_Tail(0)
{
	m_pQueueBase = new T[m_QueueSize];
	pthread_mutex_init(&m_csRW,NULL);
}

template< class T >
CCircularQueue<T>::~CCircularQueue(void)
{
	if(m_pQueueBase)
	{
		delete[] m_pQueueBase;
		m_pQueueBase = NULL;
	}
	pthread_mutex_destroy(&m_csRW);
}

template< class T >
bool CCircularQueue<T>::PushBack(const T elem)
{
	if(m_pQueueBase == NULL)
	{
		return false;
	}

	pthread_mutex_lock(&m_csRW);

	if(IsFull())
	{
		printf("queue is full, oldest packet will lost\n");
		m_Head = (m_Head+1)% m_QueueSize;
	}

    memcpy(&m_pQueueBase[m_Tail], &elem, sizeof(elem));

	m_Tail = (m_Tail+1) % m_QueueSize;
	
	pthread_mutex_unlock(&m_csRW);
	return true;
}

template< class T >
bool CCircularQueue<T>::PopFront(T& elem, int &unHandleCnt)
{
	pthread_mutex_lock(&m_csRW);
	if(IsEmpty())
	{
		pthread_mutex_unlock(&m_csRW);
		return false;
	}

    memcpy(&elem, &m_pQueueBase[m_Head], sizeof(m_pQueueBase[m_Head]));

	m_Head = (m_Head+1) % m_QueueSize;
	unHandleCnt = (m_Tail - m_Head + m_QueueSize) % m_QueueSize;
	pthread_mutex_unlock(&m_csRW);
	
	return true;
}

template< class T >
void CCircularQueue<T>::SyncRwPoint()
{
    pthread_mutex_lock(&m_csRW);
    m_Head = m_Tail;    
    pthread_mutex_unlock(&m_csRW);
}

template< class T >
int CCircularQueue<T>::Size()
{
	int size = 0;
	
	pthread_mutex_lock(&m_csRW);
	size = (m_Tail - m_Head + m_QueueSize) % m_QueueSize;
	pthread_mutex_unlock(&m_csRW);
	
	return size;
}

template< class T >
bool CCircularQueue<T>::IsEmpty()
{
	return (m_Head == m_Tail) ? true : false;
}

template< class T >
bool CCircularQueue<T>::IsFull()
{
	return (((m_Tail+1) % m_QueueSize) == m_Head) ? true : false;
}

template< class T >
int CCircularQueue<T>::RemainSize()
{
	return ((m_QueueSize)-Size());
}