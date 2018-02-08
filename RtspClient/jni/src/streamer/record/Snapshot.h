#ifndef __SNAPSHOT_H__
#define __SNAPSHOT_H__

#include <string>
#include "com_bql_MSG_Native.h"
#include "SwsScale.h"

////////////////////////////////////////////////////////////////////////////////
class CSnapshot : public CWriter
{
public:
	CSnapshot(JNIStreamerContext *pParams, CSwsScale *pScales, const char *file, int type)
	  : m_pParams(pParams), m_pScales(pScales), m_file(file), m_type(type)
	{
		THIS_LOGT_print("is created of %s", file);
	}
	virtual ~CSnapshot()
	{
		if( m_pScales != 0 ) delete m_pScales;
		THIS_LOGT_print("is deleted");
	}

public: //interface of CWriter
	virtual int Write(JNIEnv *env, void *buffer = 0);

private:
	JNIStreamerContext *m_pParams;
	CSwsScale  		   *m_pScales;
	std::string m_file;
	int         m_type; //0-bmp 1-jpg
	CLASS_LOG_DECLARE(CSnapshot);
};

#endif
