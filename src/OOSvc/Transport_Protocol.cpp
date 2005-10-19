#include "./Transport_Protocol.h"

OOSvc_Transport_Protocol::OOSvc_Transport_Protocol(const char* name) :
	m_name(name)
{
}

OOSvc_Transport_Protocol::~OOSvc_Transport_Protocol(void)
{
}

const char* OOSvc_Transport_Protocol::protocol_name()
{
	return m_name;
}

bool OOSvc_Transport_Protocol::address_is_equal(const char* addr1, const char* addr2)
{
	return false;
}

int OOSvc_Transport_Protocol::open_transport(const char* remote_host, OOCore_Transport_Base*& transport)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	// Check if we already have a connection to the remote_host
	for (std::map<ACE_CString,OOCore_Transport_Base*>::iterator i = m_transport_map.begin();i!=m_transport_map.end();++i)
	{
		if (address_is_equal(remote_host,i->first.c_str()))
		{
			transport = i->second;
			return 0;
		}
	}

	if (connect_transport(remote_host,transport) != 0)
		return -1;

	m_transport_map[remote_host] = transport;
	
	return 0;
}

void OOSvc_Transport_Protocol::transport_closed(OOCore_Transport_Base* transport)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	for (std::map<ACE_CString,OOCore_Transport_Base*>::iterator i = m_transport_map.begin();i!=m_transport_map.end();++i)
	{
		if (i->second == transport)
		{
			m_transport_map.erase(i);
			break;
		}
	}
}
