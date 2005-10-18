#include "./Transport_Manager.h"

#include <ace/SString.h>

#include "../OOCore/Transport_Base.h"

OOSvc_Transport_Manager::OOSvc_Transport_Manager(void)
{
}

OOSvc_Transport_Manager::~OOSvc_Transport_Manager(void)
{
}

const ACE_TCHAR* OOSvc_Transport_Manager::dll_name(void)
{
	return ACE_TEXT("OOSvc");
}

const ACE_TCHAR* OOSvc_Transport_Manager::name(void)
{
	return ACE_TEXT("OOSvc_Transport_Manager");
}

int OOSvc_Transport_Manager::register_protocol(OOSvc_Transport_Protocol* protocol)
{
	if (protocol == 0)
		return -1;

	if (m_network_map.bind(protocol->protocol_name(),protocol) != 0)
		return -1;

	return 0;
}

int OOSvc_Transport_Manager::unregister_protocol(const char* name)
{
	OOSvc_Transport_Protocol* prot;
	if (m_network_map.unbind(name,prot) != 0)
		return -1;

	return 0;
}

int OOSvc_Transport_Manager::connect_transport(const char* remote_url, ACE_CString& channel_part, OOCore_Transport_Base*& transport)
{
	// URL format = <protocol>://<protocol_specific_address>/<channel_name>
	ACE_CString strURL(remote_url);

	// Find the first colon
	ssize_t colon = strURL.find(':');
	if (colon==-1)
		return -1;

	// Check next 2 chars are //
	if (strURL.substr(colon+1,2) != "//")
		return -1;

	// Find the next /
	ssize_t slash = strURL.find('/',colon+3);
	if (slash==-1)
		return -1;

	ACE_CString strProtocol = strURL.substr(0,colon);
	ACE_CString strAddress = strURL.substr(colon+3,slash - colon - 3);
	channel_part = strURL.substr(slash+1);

	// Lookup protocol
	OOSvc_Transport_Protocol* protocol;
	if (m_network_map.find(strProtocol,protocol) != 0)
		return -1;

	if (protocol->connect_transport(strAddress.c_str(),transport) != 0)
		return -1;

	return 0;
}

int OOSvc_Transport_Manager::open_remote_channel(const char* remote_url, OOCore_Channel** channel, ACE_Time_Value* timeout)
{
	ACE_CString channel_part;
	OOCore_Transport_Base* transport = 0;

	if (OOSvc_Transport_Manager::connect_transport(remote_url,channel_part,transport) != 0)
		return -1;

	return transport->open_channel(channel_part.c_str(),channel);
}

int OOSvc_Transport_Manager::create_remote_object(const char* remote_url, const OOObj::GUID& iid, OOObj::Object** ppVal)
{
	ACE_CString channel_part;
	OOCore_Transport_Base* transport = 0;

	if (OOSvc_Transport_Manager::connect_transport(remote_url,channel_part,transport) != 0)
		return -1;

	return transport->create_object(channel_part.c_str(),iid,ppVal);
}

#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)
template class ACE_DLL_Singleton_T<OOSvc_Transport_Manager, ACE_Thread_Mutex>;
#elif defined (ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)
#pragma instantiate ACE_DLL_Singleton_T<OOSvc_Transport_Manager, ACE_Thread_Mutex>
#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */