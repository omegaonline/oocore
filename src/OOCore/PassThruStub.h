#ifndef OOCORE_PASSTHRUSTUB_H_INCLUDED_
#define OOCORE_PASSTHRUSTUB_H_INCLUDED_

#include "./ObjectManager.h"

namespace OOCore
{
namespace Impl
{

class PassThruStub :
	public OOUtil::Object_Impl<OOObject::Stub>
{
public:
	PassThruStub(OOCore::ObjectManager* stub_manager, const OOObject::uint32_t& stub_key, OOUtil::Object_Ptr<OOObject::ProxyStubManager>& proxy_manager, const OOObject::uint32_t& proxy_key, OOUtil::Object_Ptr<OOObject::Proxy>& proxy);
		
	int init(const OOObject::guid_t& iid, OOObject::Stub* stub);

// Stub members
public:
	int Invoke(OOObject::uint32_t method, OOObject::TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, OOObject::InputStream* input, OOObject::OutputStream* output);
	int GetObject(OOObject::Object** ppVal);
		
private:
	OOUtil::Object_Ptr<OOObject::ProxyStubManager> m_stub_manager;
	OOObject::uint32_t m_stub_key;
	OOUtil::Object_Ptr<OOObject::ProxyStubManager> m_proxy_manager;
	OOObject::uint32_t m_proxy_key;
	OOUtil::Object_Ptr<OOObject::Proxy> m_proxy;
	OOUtil::Object_Ptr<OOObject::Stub> m_stub;
	std::set<size_t> m_iid_methods;
	
	int copy(OOObject::InputStream* in, OOObject::OutputStream* out);
};

};
};

#endif // OOCORE_PASSTHRUSTUB_H_INCLUDED_
