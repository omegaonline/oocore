#include "./Stub_Marshaller.h"

#include "./Object_Marshaller.h"
#include "./Array_Marshaller.h"

OOCore_Stub_Marshaller::OOCore_Stub_Marshaller(OOCore_ProxyStub_Handler* handler, ACE_InputCDR* input, bool sync) :
	OOCore_Marshaller_Base(handler,sync),
	m_input(input)
{
}

OOCore_Stub_Marshaller::~OOCore_Stub_Marshaller()
{
	delete m_input;
}

// Special for strings
bool OOCore_Stub_Marshaller::unpack_i(const ACE_CDR::Char*& val)
{
	ACE_CString strVal;
	if (!m_input->read_string(strVal))
		return false;
	
	OOCore_Marshalled_Param_Holder<ACE_CString>* p = pack_param(strVal,false);
	if (p == 0)
		return false;
	
	// Pass out value
	val = p->value().c_str();
	return true;
}

bool OOCore_Stub_Marshaller::unpack_object_p(OOObj::Object*& val, const OOObj::GUID& iid)
{
	OOCore_Object_Marshaller obm;
	if (!obm.read(*this,*m_input,false))
		return false;

	OOCore_Marshalled_Param_Holder<OOCore_Object_Marshaller>* p = pack_param(obm,false);
	if (p == 0)
		return false;
	
	// Pass out value
	val = *(p->value().objref());
	return true;
}

bool OOCore_Stub_Marshaller::unpack_object_pp(OOObj::Object**& val, const OOObj::GUID& iid)
{
	OOCore_Object_Marshaller obm;
	if (!obm.read(*this,*m_input,false))
		return false;

	OOCore_Marshalled_Param_Holder<OOCore_Object_Marshaller>* p = pack_param(obm,true);
	if (p == 0)
		return false;
	
	// Pass out value
	val = p->value().objref();
	return true;
}
