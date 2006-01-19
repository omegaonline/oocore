#ifndef OOCORE_PASSTHRUSTUB_H_INCLUDED_
#define OOCORE_PASSTHRUSTUB_H_INCLUDED_

#include "./ObjectManager.h"

namespace OOCore
{
namespace Impl
{

class PassThruStub :
	public OOCore::Object_Impl<OOCore::Stub>
{
public:
	PassThruStub(OOCore::ObjectManager* stub_manager, const OOCore::ProxyStubManager::cookie_t& stub_key, Object_Ptr<OOCore::ProxyStubManager>& proxy_manager, const OOCore::ProxyStubManager::cookie_t& proxy_key, Object_Ptr<OOCore::Proxy>& proxy);
	
	int init(const OOObject::guid_t& iid, Stub* stub);

// Stub members
public:
	int Invoke(OOObject::uint32_t method, TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, InputStream* input, OutputStream* output);
	int GetObject(OOObject::Object** ppVal);
		
private:
	Object_Ptr<OOCore::ProxyStubManager> m_stub_manager;
	OOCore::ProxyStubManager::cookie_t m_stub_key;
	Object_Ptr<OOCore::ProxyStubManager> m_proxy_manager;
	OOCore::ProxyStubManager::cookie_t m_proxy_key;
	Object_Ptr<OOCore::Proxy> m_proxy;
	Object_Ptr<OOCore::Stub> m_stub;
	std::set<size_t> m_iid_methods;
	
	int copy(OOCore::InputStream* in, OOCore::OutputStream* out);
};

};
};

#endif // OOCORE_PASSTHRUSTUB_H_INCLUDED_
