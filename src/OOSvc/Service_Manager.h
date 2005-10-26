#ifndef _OOSVC_SERVICE_MANAGER_H_INCLUDED_
#define _OOSVC_SERVICE_MANAGER_H_INCLUDED_

#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/Hash_Map_Manager_T.h>
#include <ace/SString.h>

#include "../OOCore/Object.h"

#include "./OOSvc_export.h"

class OOCore_Channel;

class OOSvc_Channel_Acceptor_Base;

class OOSvc_Export OOSvc_Service_Manager
{
public:
	OOSvc_Service_Manager(void);
	virtual ~OOSvc_Service_Manager(void);

	const ACE_TCHAR* dll_name(void);
	const ACE_TCHAR* name(void);

	int register_service(OOSvc_Channel_Acceptor_Base* acceptor, bool bLocal);
	int unregister_service(const char* name);
	int connect_service(const char* service_name, bool bLocal, OOCore_Channel* channel, ACE_Time_Value* timeout = 0);

private:
	struct Publication
	{
		bool bLocal;
		OOSvc_Channel_Acceptor_Base* acceptor;
	};
	
	ACE_Hash_Map_Manager<ACE_CString,Publication*,ACE_RW_Thread_Mutex> m_service_acceptor_map;
};

typedef ACE_DLL_Singleton_T<OOSvc_Service_Manager, ACE_Thread_Mutex> SERVICE_MANAGER;
OOSVC_SINGLETON_DECLARE(ACE_DLL_Singleton_T,OOSvc_Service_Manager,ACE_Thread_Mutex);

#endif // _OOSVC_SERVICE_MANAGER_H_INCLUDED_
