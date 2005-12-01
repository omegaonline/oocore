#include "./PassThruStub.h"
#include "./OutputStream_CDR.h"
#include "./InputStream_CDR.h"

OOCore::Impl::PassThruStub::PassThruStub(OOCore::ProxyStubManager* manager, const OOObject::cookie_t& proxy_key, const OOObject::cookie_t& stub_key) :
	m_manager(manager),
	m_proxy_key(proxy_key),
	m_stub_key(stub_key)
{
}

int 
OOCore::Impl::PassThruStub::Invoke(unsigned int method, OOObject::int32_t& ret_code, InputStream* input, OutputStream* output)
{
	// Create a request output stream
	OOObject::uint32_t trans_id;
	OOCore::Object_Ptr<OOCore::OutputStream> req_output;
	OOObject::bool_t sync = (output!=0);

	if (m_manager->CreateRequest(m_proxy_key,method,sync,&trans_id,&req_output) != 0)
		return -1;

	// Copy input to req_output
	if (copy(input,req_output) != 0)
	{
		m_manager->CancelRequest(trans_id);
		return -1;
	}

	// Send the request
	OOCore::Object_Ptr<OOCore::InputStream> req_input;
	ret_code = m_manager->SendAndReceive(req_output,trans_id,sync ? &req_input : 0);
	if (ret_code!=0 && ret_code!=1)
		return -1;

	if (ret_code==0 && sync)
	{
		// Copy req_input to output
		if (copy(req_input,output) != 0)
			ret_code = -1;
	}

	if (ret_code==1)
	{
		// Remote stub has gone!
		m_manager->ReleaseStub(m_stub_key);
		Release();
		ret_code = 0;
	}

	return ret_code;
}

int 
OOCore::Impl::PassThruStub::copy(OOCore::InputStream* in, OOCore::OutputStream* out)
{
	// Check the magic numbers and copy
	OutputStream_CDR* out_cdr = reinterpret_cast<OutputStream_CDR*>(out);
	InputStream_CDR* in_cdr = reinterpret_cast<InputStream_CDR*>(in);
	if (out_cdr->get_magic()!=in_cdr->get_magic())
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid input or output stream\n")),-1);

	return out_cdr->copy_from(in_cdr);
}