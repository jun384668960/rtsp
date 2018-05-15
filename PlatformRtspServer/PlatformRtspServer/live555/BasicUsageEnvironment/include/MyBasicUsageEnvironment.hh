#ifndef _MYBASICUSAGEENVIRONMENT_H_
#define _MYBASICUSAGEENVIRONMENT_H_

#include "BasicUsageEnvironment.hh"
#include "HandlerSet.hh"

#include <sys/epoll.h>
#include <map>

class MyBasicTaskScheduler : public BasicTaskScheduler {
public:
	static MyBasicTaskScheduler* createNew(unsigned maxSchedulerGranularity = 10000/*microseconds*/);
	// "maxSchedulerGranularity" (default value: 10 ms) specifies the maximum time that we wait (in "select()") before
	// returning to the event loop to handle non-socket or non-timer-based events, such as 'triggered events'.
	// You can change this is you wish (but only if you know what you're doing!), or set it to 0, to specify no such maximum time.
	// (You should set it to 0 only if you know that you will not be using 'event triggers'.)
	virtual ~MyBasicTaskScheduler();

protected:
	MyBasicTaskScheduler(unsigned maxSchedulerGranularity);
	// called only by "createNew()"

protected:
	// Redefined virtual functions:
	virtual void SingleStep(unsigned maxDelayTime);

	virtual void setBackgroundHandling(int socketNum, int conditionSet, BackgroundHandlerProc* handlerProc, void* clientData);
	virtual void moveSocketHandling(int oldSocketNum, int newSocketNum);

private:
	int m_epollfd;
	std::map<int , struct epoll_event> m_pevents;
	struct epoll_event *m_EpArr;
};

#endif //_MYBASICUSAGEENVIRONMENT_H_
