#ifndef OOCORE_OOCORE_H_INCLUDED_
#define OOCORE_OOCORE_H_INCLUDED_

#include "./OOObject.h"

#ifdef _DEBUG
#define DEFAULT_WAIT	30
#else
#define DEFAULT_WAIT	5
#endif

class ACE_Reactor;

namespace OOCore
{
	
	/*typedef int (*CreateProxy_Function)(OOObject::ProxyStubManager* manager, const OOObject::guid_t& iid, const OOObject::uint32_t& key, OOObject::Object** proxy);
	typedef int (*CreateStub_Function)(OOObject::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::uint32_t& key, OOObject::Stub** stub);
	typedef int (*RegisterLib_Function)(bool bRegister);
	typedef int (*GetTypeInfo_Function)(const OOObject::guid_t& iid, OOObject::TypeInfo** typeinfo);

	OOCore_Export int RegisterProxyStub(const OOObject::guid_t& iid, const char* dll_name);
	OOCore_Export int UnregisterProxyStub(const OOObject::guid_t& iid, const char* dll_name);
    OOCore_Export OOObject::int32_t RegisterObjectFactory(OOObject::ObjectFactory::Flags_t flags, const OOObject::guid_t& oid, OOObject::ObjectFactory* pFactory);
	OOCore_Export OOObject::int32_t UnregisterObjectFactory(const OOObject::guid_t& oid);
	OOCore_Export OOObject::int32_t RegisterStaticInterface(const OOObject::guid_t& iid, OOObject::ObjectFactory* pFactory);
	OOCore_Export OOObject::int32_t UnregisterStaticInterface(const OOObject::guid_t& iid);
	OOCore_Export OOObject::int32_t RegisterProtocol(const OOObject::char_t* name, OOObject::Protocol* protocol);
	OOCore_Export OOObject::int32_t UnregisterProtocol(const OOObject::char_t* name);
	OOCore_Export int GetTypeInfo(const OOObject::guid_t& iid, OOObject::TypeInfo** type_info);*/

	// App helpers
	OOCore_Export int ExecProcess(ACE_CString strExeName);
	OOCore_Export int SetRunningObjectTable(OOObject::RunningObjectTable* pROT);

	// Engine helpers
	OOCore_Export ACE_Reactor* GetEngineReactor();
	OOCore_Export int OpenEngine(int argc, ACE_TCHAR* argv[]);
	OOCore_Export int CloseEngine();

	typedef bool (*PUMP_CONDITION_FN)(void* cond_fn_args);
	OOCore_Export int PumpRequests(ACE_Time_Value* timeout = 0, PUMP_CONDITION_FN cond_fn = 0, void* cond_fn_args = 0);
};

#endif // OOCORE_OOCORE_H_INCLUDED_
