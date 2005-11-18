//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "OOCore.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_PROXY_STUB_FACTORY_H_INCLUDED_
#define OOCORE_PROXY_STUB_FACTORY_H_INCLUDED_

#include <map>

#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>
#include <ace/DLL.h>

#include "./OOCore.h"

namespace Impl
{

class Proxy_Stub_Factory
{
public:
	int create_proxy(OOCore::ProxyStubManager* manager, const OOObj::guid_t& iid, const OOObj::cookie_t& cookie, OOObj::Object** proxy);
	int create_stub(OOCore::ProxyStubManager* manager, const OOObj::guid_t& iid, OOObj::Object* obj, OOCore::Stub** ppStub);

private:
	Proxy_Stub_Factory(void) {}
	virtual ~Proxy_Stub_Factory(void) {}
	friend ACE_Singleton<Proxy_Stub_Factory, ACE_Thread_Mutex>;

	struct proxystub_node
	{
		ACE_DLL dll;
		OOCore::CreateProxy_Function proxy_fn;
		OOCore::CreateStub_Function stub_fn;
	};
	std::map<OOObj::guid_t,proxystub_node*> m_dll_map;
	ACE_Thread_Mutex m_lock;

	static proxystub_node m_core_node;
	
	static int CreateProxy(OOCore::ProxyStubManager* manager, const OOObj::guid_t& iid, const OOObj::cookie_t& key, OOObj::Object** proxy);
	static int CreateStub(OOCore::ProxyStubManager* manager, const OOObj::guid_t& iid, OOObj::Object* obj, OOCore::Stub** stub);

	int load_proxy_stub(const OOObj::guid_t& iid, proxystub_node*& node);
};

typedef ACE_Singleton<Proxy_Stub_Factory, ACE_Thread_Mutex> PROXY_STUB_FACTORY;

};

#endif // OOCORE_PROXY_STUB_FACTORY_H_INCLUDED_
