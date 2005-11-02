#include "./OOControlService.h"

#include <ace/Reactor.h>

#include "../OOCore/Binding.h"
#include "../OOSvc/Shutdown.h"

OOControlService::OOControlService() :
	OOSvc_Object_Acceptor<OOObj::Client_Service,OOControlClient::Create>("control_service"),
	m_stayalive(true)
{
}

OOControlService::~OOControlService()
{
}

void OOControlService::add_client(OOControlClient* client)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	m_client_list.push_front(client);
}

void OOControlService::remove_client(OOControlClient* client)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
	m_client_list.remove(client);
	guard.release();

	if (m_client_list.empty() && !m_stayalive)
	{
		guard.release();

		// Tell the reactor to stop
		OOSvc_Shutdown();
	}
}

ACE_UINT16 OOControlService::stop(bool force)
{
	m_stayalive = false;

	if (force)
	{
		// Tell the reactor to stop
		OOSvc_Shutdown();
	}

	return m_client_list.size();
}

bool OOControlService::stop_pending()
{
	return !m_stayalive;
}

void OOControlService::stay_alive()
{
	m_stayalive = true;
}

#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)
template class ACE_Singleton<OOControlService, ACE_Thread_Mutex>;
#elif defined (ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)
#pragma instantiate ACE_Singleton<OOControlService, ACE_Thread_Mutex>
#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */
