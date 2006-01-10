#include "./ProxyStub.h"

OOCore::Impl::marshaller_t::marshaller_t() :
	m_in(0), 
	m_out(0),
	m_failed(true),
	m_flags(0),
	m_trans_id(0),
	m_wait_secs(0)
{ }

OOCore::Impl::marshaller_t::marshaller_t(OOCore::ProxyStubManager* manager, TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, OOCore::OutputStream* output, OOObject::uint32_t trans_id) :
	m_in(0),
	m_out(output), 
	m_failed(false), 
	m_manager(manager),
	m_flags(flags),
	m_trans_id(trans_id), 
	m_wait_secs(wait_secs)
{ }

OOObject::int32_t 
OOCore::Impl::marshaller_t::send_and_recv()
{
	if (m_failed)
		return -1;

	m_failed = true;

	OOCore::Object_Ptr<OOCore::InputStream> ptrIn;
	if (m_manager->SendAndReceive(m_flags,m_wait_secs,m_out,m_trans_id,&ptrIn)!=0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) SendAndReceive failed: %m\n")),-1);
	
	m_in = ptrIn;
	m_failed = false;

	return 0;
}
