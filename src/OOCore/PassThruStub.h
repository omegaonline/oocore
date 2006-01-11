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
	PassThruStub(OOCore::ObjectManager* manager, const OOCore::ProxyStubManager::cookie_t& proxy_key, const OOCore::ProxyStubManager::cookie_t& stub_key);

// Stub members
public:
	int Invoke(TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, InputStream* input, OutputStream* output);
	int GetObject(OOObject::Object** ppVal);
		
private:
	OOCore::Object_Ptr<OOCore::ObjectManager> m_manager;
	OOCore::ProxyStubManager::cookie_t m_proxy_key;
	OOCore::ProxyStubManager::cookie_t m_stub_key;

	int copy(OOCore::InputStream* in, OOCore::OutputStream* out);
};

};
};

#endif // OOCORE_PASSTHRUSTUB_H_INCLUDED_
