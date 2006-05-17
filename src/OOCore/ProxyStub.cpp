#include "./ProxyStub.h"

using namespace OOUtil;

ProxyStub_Base::marshaller_t::marshaller_t() :
	m_failed(true),
	m_flags(0),
	m_trans_id(0),
	m_wait_secs(0)
{ }

ProxyStub_Base::marshaller_t::marshaller_t(OOObject::ProxyStubManager* manager, OOObject::TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, OOObject::OutputStream* output, OOObject::uint32_t trans_id) :
	m_out(output), 
	m_failed(false), 
	m_manager(manager),
	m_flags(flags),
	m_trans_id(trans_id), 
	m_wait_secs(wait_secs)
{ }

OOObject::int32_t 
ProxyStub_Base::marshaller_t::send_and_recv()
{
	if (m_failed)
		return -1;

	m_failed = true;

	InputStream_Ptr ptrIn;
	if (m_manager->SendAndReceive(m_flags,m_wait_secs,m_out,m_trans_id,&ptrIn)!=0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Remote call failed: %m\n")),-1);
	
	m_in = ptrIn;
	m_failed = false;

	return 0;
}
