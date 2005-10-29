#include "./Shutdown.h"

#include "..\OOCore\OOCore.h"

typedef ACE_Singleton<ACE_Future<int>, ACE_Thread_Mutex> SHUTDOWN;

bool OOSvc_Shutdown_Observer::m_signalled = false;

int OOSvc_Export OOSvc_Shutdown()
{
	OOSvc_Shutdown_Request* req;
	ACE_NEW_RETURN(req,OOSvc_Shutdown_Request,-1);
	
	return OOCore_PostRequest(req);
}

OOSvc_Shutdown_Observer::OOSvc_Shutdown_Observer(void) : 
	ACE_Future_Observer<int>()
{
	if (!m_signalled)
		SHUTDOWN::instance()->attach(this);
}

OOSvc_Shutdown_Observer::~OOSvc_Shutdown_Observer(void)
{
	if (!m_signalled)
		SHUTDOWN::instance()->detach(this);
}

void OOSvc_Shutdown_Observer::update(const ACE_Future<int>& val)
{
	handle_shutdown();

	m_signalled = true;
}

int OOSvc_Shutdown_Request::call()
{
	SHUTDOWN::instance()->set(1);

	ACE_Time_Value wait(0);
	OOCore_RunReactor(&wait);

	return ACE_Reactor::instance()->end_reactor_event_loop();
}