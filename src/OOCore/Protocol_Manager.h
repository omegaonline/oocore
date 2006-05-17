//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "OOCore.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_PROTOCOL_MANAGER_H_INCLUDED_
#define OOCORE_PROTOCOL_MANAGER_H_INCLUDED_

#include <ace/Naming_Context.h>
#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>

#include <map>

#include "./OOUtil.h"

#include "./OOCore_export.h"

namespace OOCore
{
namespace Impl
{

class Protocol_Manager
{
public:
	OOObject::int32_t RegisterProtocol(const OOObject::char_t* name, OOObject::Protocol* protocol);
	OOObject::int32_t UnregisterProtocol(const OOObject::char_t* name);

	OOObject::int32_t create_remote_object(const OOObject::char_t* remote_url, const OOObject::guid_t& oid, OOObject::Object* pOuter, const OOObject::guid_t& iid, OOObject::Object** ppVal);

private:
	friend class ACE_DLL_Singleton<Protocol_Manager, ACE_Thread_Mutex>;

	Protocol_Manager(void) {}
	virtual ~Protocol_Manager(void) {}

	ACE_Thread_Mutex	m_lock;
	std::map<ACE_CString,OOUtil::Object_Ptr<OOObject::Protocol> > m_protocol_map;
};

typedef ACE_DLL_Singleton<Protocol_Manager, ACE_Thread_Mutex> PROTOCOL_MANAGER;

};
};

#endif // OOCORE_PROTOCOL_MANAGER_H_INCLUDED_
