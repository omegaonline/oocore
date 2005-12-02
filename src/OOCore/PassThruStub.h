#ifndef OOCORE_PASSTHRUSTUB_H_INCLUDED_
#define OOCORE_PASSTHRUSTUB_H_INCLUDED_

#include "./OOCore_Util.h"

namespace OOCore
{
namespace Impl
{

class PassThruStub :
	public OOCore::Object_Impl<OOCore::Stub>
{
public:
	PassThruStub(OOCore::ProxyStubManager* manager, const OOObject::cookie_t& proxy_key, const OOObject::cookie_t& stub_key);

// Stub members
public:
	int Invoke(Marshall_Flags flags, OOObject::uint16_t wait_secs, InputStream* input, OutputStream* output);

private:
	OOCore::Object_Ptr<OOCore::ProxyStubManager> m_manager;
	const OOObject::cookie_t& m_proxy_key;
	const OOObject::cookie_t& m_stub_key;

	int copy(OOCore::InputStream* in, OOCore::OutputStream* out);
};

};
};

#endif // OOCORE_PASSTHRUSTUB_H_INCLUDED_
