#include "./Shutdown.h"

#include "../OOCore/Engine.h"

typedef ACE_Singleton<ACE_Future<int>, ACE_Thread_Mutex> SHUTDOWN;

bool OOSvc_Shutdown_Observer::m_signalled = false;

int OOSvc_Export OOSvc_Shutdown()
{
	OOSvc_Shutdown_Request* req;
	ACE_NEW_RETURN(req,OOSvc_Shutdown_Request,-1);
	
	return ENGINE::instance()->post_request(req);
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
	return SHUTDOWN::instance()->set(1);
}
