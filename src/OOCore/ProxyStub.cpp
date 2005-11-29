#include "./ProxyStub.h"

OOProxyStub::Impl::marshaller_t::marshaller_t() :
	m_in(0), m_out(0), m_failed(true), m_sync(true), m_trans_id(0)
{
}

OOProxyStub::Impl::marshaller_t::marshaller_t(OOCore::ProxyStubManager* manager, OOObject::bool_t sync, OOCore::OutputStream* output, OOObject::uint32_t trans_id) :
	m_in(0), m_out(output), m_failed(false), m_manager(manager), m_sync(sync), m_trans_id(trans_id)
{
}

OOObject::int32_t 
OOProxyStub::Impl::marshaller_t::send_and_recv()
{
	if (m_failed)
		return -1;

	if (!m_sync)
		return m_manager->SendAndReceive(m_out,m_trans_id,NULL);

	m_failed = true;

	OOCore::Object_Ptr<OOCore::InputStream> ptrIn;
	if (m_manager->SendAndReceive(m_out,m_trans_id,&ptrIn)!=0)
		return -1;
	
	// Read the response code
	OOObject::int32_t ret_code;
	if (ptrIn->ReadLong(ret_code) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read return code\n")),-1);

	if (ret_code != 0)
		return ret_code;

	m_in = OOCore::Impl::InputStream_Wrapper(ptrIn);
	m_failed = false;

	return 0;
}
