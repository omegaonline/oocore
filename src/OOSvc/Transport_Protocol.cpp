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

int OOSvc_Transport_Protocol::connect_transport(const char* remote_host, OOCore_Transport_Base*& transport)
{
	// Check if we already have a connection to the remote_host
	ACE_Hash_Map_Manager<ACE_CString,OOCore_Transport_Base*,ACE_RW_Thread_Mutex>::ITERATOR i = m_transport_map.begin();
	for (;i!=m_transport_map.end();++i)
	{
		if (AddressIsEqual(remote_host,(*i).ext_id_.c_str()))
		{
			transport = (*i).int_id_;
			return 0;
		}
	}

	return -1;
}