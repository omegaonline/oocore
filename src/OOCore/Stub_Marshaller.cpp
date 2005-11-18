#include "./Stub_Marshaller.h"

#include "./Object_Marshaller.h"

Impl::Stub_Marshaller::Stub_Marshaller(OOCore::ProxyStubManager* manager, OOCore::InputStream* input) :
	Marshaller_Base(manager),
	m_input(input)
{
	if (!m_input)
		m_failed = true;
}

Impl::Stub_Marshaller::~Stub_Marshaller()
{
}

bool 
Impl::Stub_Marshaller::unpack_object_p(OOObj::Object*& val, const OOObj::guid_t& iid)
{
	Object_Marshaller obm;
	if (!obm.read(*this,m_input,false))
		return false;

	Marshalled_Param_Holder<Object_Marshaller>* p = pack_param(obm,false);
	if (p == 0)
		return false;
	
	// Pass out value
	val = *(p->value().objref());
	return true;
}

bool 
Impl::Stub_Marshaller::unpack_object_pp(OOObj::Object**& val, const OOObj::guid_t& iid)
{
	Object_Marshaller obm;
	if (!obm.read(*this,m_input,false))
		return false;

	Marshalled_Param_Holder<Object_Marshaller>* p = pack_param(obm,true);
	if (p == 0)
		return false;
	
	// Pass out value
	val = p->value().objref();
	return true;
}

bool 
Impl::IOWrappers::read_param(Marshaller_Base* mshl, OOCore::InputStream* input, StringHolder& val, bool response)
{
	return false;
}

bool 
Impl::IOWrappers::write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const StringHolder& val, bool response)
{
	return false;
}