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
	int connect_transport(const char* remote_host, OOCore_Transport_Base*& transport);

protected:
	virtual bool AddressIsEqual(const char* addr1, const char* addr2) = 0;

private:
	const char* m_name;
	ACE_Hash_Map_Manager<ACE_CString,OOCore_Transport_Base*,ACE_RW_Thread_Mutex> m_transport_map;
};
