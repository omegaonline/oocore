#include "./Proxy_Marshaller.h"

Impl::Proxy_Marshaller::Proxy_Marshaller() :
	Marshaller_Base(0,true)
{
}

Impl::Proxy_Marshaller::Proxy_Marshaller(OOCore::ProxyStubManager* manager, OOObj::bool_t sync, OOCore::OutputStream* output, OOObj::uint32_t trans_id) :
	Marshaller_Base(manager),
	m_output(output),
	m_trans_id(trans_id),
	m_sync(sync)
{
}

/*Impl::Proxy_Marshaller& 
Impl::Proxy_Marshaller::operator <<(const ACE_CDR::Char* val)
{
	ACE_CString strVal(val);

	if (!m_failed)
		m_failed = !m_output.write_string(strVal);
	
	if (!m_failed)
		m_failed = (pack_param(strVal,false) == 0);

	return *this;
}*/

int 
Impl::Proxy_Marshaller::operator ()(ACE_Time_Value& wait)
{
	return (*this)(&wait);
}

int 
Impl::Proxy_Marshaller::operator ()(ACE_Time_Value* wait)
{
	if (!m_sync)
	{
		if (send_and_recv(m_output,m_trans_id,0) != 0)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No response recieved\n")),-1);
	}
	else
	{
		// Wait for a response
		OOCore::Object_Ptr<OOCore::InputStream> input;
		if (send_and_recv(m_output,m_trans_id,&input) != 0)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No response recieved\n")),-1);
	
		// Read the response code
		OOObj::int32_t ret_code;
		if (input->ReadLong(ret_code) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read return code\n")),-1);

		if (ret_code != 0)
			return ret_code;
		
		if (input_response(input) != 0)
			return -1;
	}

	return 0;
}
