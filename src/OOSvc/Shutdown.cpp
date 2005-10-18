#include ".\shutdown.h"

typedef ACE_Singleton<ACE_Future<int>, ACE_Thread_Mutex> SHUTDOWN;

bool OOSvc_Shutdown_Observer::m_signalled = false;

void OOSvc_Export OOSvc_Shutdown()
{
	SHUTDOWN::instance()->set(1);

	ACE_Reactor::instance()->end_reactor_event_loop();
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