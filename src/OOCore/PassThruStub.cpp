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
	OOCore::Object_Ptr<OOCore::OutputStream> request;
	
	if (m_manager->CreateRequest(flags,m_proxy_key,&trans_id,&request) != 0)
		return -1;

	// Copy input to request
	if (copy(input,request) != 0)
	{
		m_manager->CancelRequest(trans_id);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to copy params\n")),-1);
	}

	// Send the request
	OOCore::Object_Ptr<OOCore::InputStream> response;
	OOObject::int32_t ret = m_manager->SendAndReceive(flags,wait_secs,request,trans_id,&response);
	
	if (!(flags & TypeInfo::async_method))
	{
		// Write error code out
		if (output->WriteLong(ret) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write ret_code\n")),-1);

		if (ret != 0)
		{
			if (output->WriteLong(errno) != 0)
				ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write errno\n")),-1);
		}
		else
		{
			// Copy response to output
			if (copy(response,output) != 0)
				ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to copy params\n")),-1);
		}
	}
	
	return ret;
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
