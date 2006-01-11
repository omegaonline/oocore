//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "OOCore.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_PROXY_STUB_FACTORY_H_INCLUDED_
#define OOCORE_PROXY_STUB_FACTORY_H_INCLUDED_

#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>
#include <ace/DLL.h>

#include <map>

#include "./OOCore.h"
#include "./ProxyStub.h"

namespace OOCore
{
namespace Impl
{

class Proxy_Stub_Factory
{
public:
	int create_proxy(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, const OOCore::ProxyStubManager::cookie_t& key, OOObject::Object** proxy);
	int create_stub(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOCore::ProxyStubManager::cookie_t& key, OOCore::Stub** ppStub);
	int create_type_info(const OOObject::guid_t& iid, OOCore::TypeInfo** type_info);

private:
	Proxy_Stub_Factory(void);
	virtual ~Proxy_Stub_Factory(void);
	friend class ACE_Singleton<Proxy_Stub_Factory, ACE_Thread_Mutex>;

	struct proxystub_node
	{
		ACE_DLL dll;
		OOCore::CreateProxy_Function proxy_fn;
		OOCore::CreateStub_Function stub_fn;
		OOCore::GetTypeInfo_Function get_type_info_fn;
	};
	std::map<OOObject::guid_t,proxystub_node*> m_dll_map;
	
	ACE_Thread_Mutex m_lock;

	static proxystub_node m_core_node;

	static int CreateProxyStub(int type, OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOCore::ProxyStubManager::cookie_t& key, OOObject::Object** proxy, OOCore::Stub** stub, OOCore::TypeInfo** typeinfo, const char* dll_name);
	static int CreateProxy(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, const OOCore::ProxyStubManager::cookie_t& key, OOObject::Object** proxy);
	static int CreateStub(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOCore::ProxyStubManager::cookie_t& key, OOCore::Stub** stub);
	static int GetTypeInfo(const OOObject::guid_t& iid, OOCore::TypeInfo** type_info);

	int load_proxy_stub(const OOObject::guid_t& iid, proxystub_node*& node);
};

typedef ACE_Singleton<Proxy_Stub_Factory, ACE_Thread_Mutex> PROXY_STUB_FACTORY;

};
};

#endif // OOCORE_PROXY_STUB_FACTORY_H_INCLUDED_
