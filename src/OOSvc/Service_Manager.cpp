#include "./Service_Manager.h"

#include "../OOCore/Channel.h"

#include "./Channel_Acceptor.h"

OOSvc_Service_Manager::OOSvc_Service_Manager(void)
{
}

OOSvc_Service_Manager::~OOSvc_Service_Manager(void)
{
}

const ACE_TCHAR* OOSvc_Service_Manager::dll_name(void)
{
	return ACE_TEXT("OOSvc");
}

const ACE_TCHAR* OOSvc_Service_Manager::name(void)
{
	return ACE_TEXT("OOSvc_Service_Manager");
}

int OOSvc_Service_Manager::register_service(OOSvc_Channel_Acceptor_Base* acceptor, bool bLocal)
{
	if (acceptor == 0)
		return -1;

	Publication* pub;
	ACE_NEW_RETURN(pub,Publication,-1);

	pub->bLocal = bLocal;
	pub->acceptor = acceptor;

	if (m_service_acceptor_map.bind(acceptor->name(),pub) != 0)
		return -1;

	return 0;
}

int OOSvc_Service_Manager::unregister_service(const char* name)
{
	Publication* pub;
	if (m_service_acceptor_map.unbind(name,pub) != 0)
		return -1;

	delete pub;

	return 0;
}

int OOSvc_Service_Manager::connect_service(const char* service_name, bool bLocal, OOCore_Channel* channel, ACE_Time_Value* timeout)
{
	Publication* pub;
	if (m_service_acceptor_map.find(service_name,pub) != 0)
		return 1;

	if (pub->bLocal != bLocal)
		return -1;

	if (pub->acceptor->handle_accept(channel) == -1)
		return -1;
		
	return 0;
}

#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)
template class ACE_DLL_Singleton_T<OOSvc_Service_Manager, ACE_Thread_Mutex>;
#elif defined (ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)
#pragma instantiate ACE_DLL_Singleton_T<OOSvc_Service_Manager, ACE_Thread_Mutex>
#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */
