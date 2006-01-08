#include "./PassThruStub.h"
#include "./OutputStream_CDR.h"
#include "./InputStream_CDR.h"

OOCore::Impl::PassThruStub::PassThruStub(OOCore::ObjectManager* manager, const OOCore::ProxyStubManager::cookie_t& proxy_key, const OOCore::ProxyStubManager::cookie_t& stub_key) :
	m_manager(manager),
	m_proxy_key(proxy_key),
	m_stub_key(stub_key)
{
}

int 
OOCore::Impl::PassThruStub::Invoke(TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, InputStream* input, OutputStream* output)
{
	// Create a request output stream
	OOObject::uint32_t trans_id;
	OOCore::Object_Ptr<OOCore::OutputStream> req_output;
	
	if (m_manager->CreateRequest(flags,m_proxy_key,&trans_id,&req_output) != 0)
		return -1;

	// Copy input to req_output
	if (copy(input,req_output) != 0)
	{
		m_manager->CancelRequest(trans_id);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to copy params\n")),-1);
	}

	// Send the request
	OOCore::Object_Ptr<OOCore::InputStream> req_input;
	if (m_manager->SendAndReceive(flags,wait_secs,req_output,trans_id,&req_input) != 0)
		return -1;

	if (!(flags & TypeInfo::async))
	{
		// Copy req_input to output
		if (copy(req_input,output) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to copy params\n")),-1);
	}
	
	return 0;
}

int 
OOCore::Impl::PassThruStub::GetObject(OOObject::Object** ppVal)
{
	errno = EFAULT;
	ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("GetObject is not allowed on a PassThruStub\n")),-1);
}

int 
OOCore::Impl::PassThruStub::copy(OOCore::InputStream* in, OOCore::OutputStream* out)
{
	// Check the magic numbers and copy
	Object_Ptr<Impl::OutputStream_CDR> out_cdr;
	if (out->QueryInterface(Impl::InputStream_CDR::IID,reinterpret_cast<OOObject::Object**>(&out_cdr)) != 0)
	{
		errno = EFAULT;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid output stream\n")),-1);
	}

	Object_Ptr<Impl::InputStream_CDR> in_cdr;
	if (in->QueryInterface(Impl::InputStream_CDR::IID,reinterpret_cast<OOObject::Object**>(&in_cdr)) != 0)
	{
		errno = EFAULT;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid input stream\n")),-1);
	}

	return out_cdr->copy_from(in_cdr);
}
