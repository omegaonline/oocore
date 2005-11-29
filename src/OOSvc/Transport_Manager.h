#ifndef _OOSVC_TRANSPORT_MANAGER_H_INCLUDED_
#define _OOSVC_TRANSPORT_MANAGER_H_INCLUDED_

#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/Hash_Map_Manager_T.h>
#include <ace/SString.h>

//#include "../OOCore/Channel.h"
#include "../OOCore/Object.h"

#include "./Transport_Protocol.h"

#include "./OOSvc_export.h"

class OOSvc_Export OOSvc_Transport_Manager
{
public:
	OOSvc_Transport_Manager(void);
	virtual ~OOSvc_Transport_Manager(void);

	const ACE_TCHAR* dll_name(void);
	const ACE_TCHAR* name(void);

	int register_protocol(OOSvc_Transport_Protocol* protocol);
	int unregister_protocol(const char* name);
	
	int open_remote_channel(const char* remote_url, OOCore_Channel** channel, ACE_Time_Value* timeout = 0);
	int create_remote_object(const char* remote_url, const OOObj::GUID& iid, OOObj::Object** ppVal);

private:
	ACE_Hash_Map_Manager<ACE_CString,OOSvc_Transport_Protocol*,ACE_RW_Thread_Mutex> m_network_map;

	int open_transport(const char* remote_url, ACE_CString& channel_part, OOCore_Transport_Base*& transport);
};

typedef ACE_DLL_Singleton_T<OOSvc_Transport_Manager, ACE_Thread_Mutex> TRANSPORT_MANAGER;
OOSVC_SINGLETON_DECLARE(ACE_DLL_Singleton_T,OOSvc_Transport_Manager,ACE_Thread_Mutex);

#endif // _OOSVC_TRANSPORT_MANAGER_H_INCLUDED_
