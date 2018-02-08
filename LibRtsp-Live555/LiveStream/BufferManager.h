#pragma once
#include <map>
#include <string>
#include "CircularQueue.h"

using namespace std;

struct CBuffer_t
{
	unsigned int lenght;
	unsigned char* data;
	unsigned long long pts;
};

class CBufferInfo_t
{
public:
	CBufferInfo_t(int nQueueSize = 6);
	virtual ~CBufferInfo_t();
public:
	CCircularQueue<CBuffer_t>* usingQueue(){ return m_BufferQueue; };

public:
	CCircularQueue<CBuffer_t>* m_BufferQueue;
};

class CBufferManager_t
{
public:
	CBufferManager_t();
	virtual ~CBufferManager_t();

public:
	bool Init(unsigned int maxLength);
	bool Register(string name, CBufferInfo_t* info);
	bool UnRegister(string name);
	void Sync(); 
	bool Write(unsigned char* data, unsigned int length, unsigned long long pts);

private:
	map<string, CBufferInfo_t*> 	m_RegMap;	//user map - witch use this buffer
	unsigned char* m_pStart;	//buffer start point
	unsigned int m_Lenght;	//total buffer length
	unsigned int m_Offset;	//current position
};
