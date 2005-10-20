#pragma once

#include <ace/Thread_Mutex.h>
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/Hash_Map_Manager_T.h>
#include <ace/SString.h>

#include "../OOCore/Transport_Base.h"

#include "./OOSvc_export.h"

class OOSvc_Export OOSvc_Transport_Protocol
{
public:
	OOSvc_Transport_Protocol(const char* name);
	virtual ~OOSvc_Transport_Protocol(void);

	const char* protocol_name();
	int open_transport(const char* remote_host, OOCore_Transport_Base*& transport);
	void transport_closed(OOCore_Transport_Base* transport);

protected:
	virtual bool address_is_equal(const char* addr1, const char* addr2);
	virtual int connect_transport(const char* remote_host, OOCore_Transport_Base*& transport) = 0;

private:
	ACE_Thread_Mutex m_lock;
	const char* m_name;
	std::map<ACE_CString,OOCore_Transport_Base*> m_transport_map;
};
