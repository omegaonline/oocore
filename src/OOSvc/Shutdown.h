#ifndef _OOSVC_SHUTDOWN_H_INCLUDED_
#define _OOSVC_SHUTDOWN_H_INCLUDED_

#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>
#include <ace/Future.h>
#include <ace/Method_Request.h>

#include "./OOSvc_export.h"

int OOSvc_Export OOSvc_Shutdown();

class OOSvc_Export OOSvc_Shutdown_Observer : public ACE_Future_Observer<int>
{
public:
	OOSvc_Shutdown_Observer(void);
	virtual ~OOSvc_Shutdown_Observer(void);
	
	void update(const ACE_Future<int>& val);

protected:
	virtual void handle_shutdown() = 0;

private:
	static bool m_signalled;
};

class OOSvc_Shutdown_Request : public ACE_Method_Request
{
public:
	int call();
};

#endif // _OOSVC_SHUTDOWN_H_INCLUDED_
