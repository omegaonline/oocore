#include "./Proxy_Marshaller.h"

#include "./ProxyStub_Handler.h"
#include "./Channel.h"

OOCore_Proxy_Marshaller::OOCore_Proxy_Marshaller(OOCore_ProxyStub_Handler* handler, bool sync = true) :
	OOCore_Marshaller_Base(handler,sync)
{
}

// This one is only used for failure codes
OOCore_Proxy_Marshaller::OOCore_Proxy_Marshaller() :
	OOCore_Marshaller_Base(0,true,true)
{
}

OOCore_Proxy_Marshaller& OOCore_Proxy_Marshaller::operator <<(const ACE_CDR::Char* val)
{
	ACE_CString strVal(val);

	if (!m_failed)
		m_failed = !m_output.write_string(strVal);
	
	if (!m_failed)
		m_failed = (pack_param(strVal,false) == 0);

	return *this;
}

int OOCore_Proxy_Marshaller::operator ()(ACE_Time_Value& wait)
{
	return (*this)(&wait);
}

int OOCore_Proxy_Marshaller::operator ()(ACE_Time_Value* wait)
{
	int ret = send_and_recv(wait);

	if (m_handler != 0)
		m_handler->remove_marshaller(m_trans_key);
	
	return ret;
}

int OOCore_Proxy_Marshaller::send_and_recv(ACE_Time_Value* wait)
{
	if (m_handler==0 || m_failed)
		return -1;

	// Send the request
	ACE_Message_Block* mb = m_output.begin()->duplicate();

	OOCore_Channel* ch = m_handler->channel();
	if (ch==0)
	{
		mb->release();
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Marshaller channel is NULL\n")),-1);
	}

	if (ch->send(mb,wait) == -1)
	{
		mb->release();
		return -1;
	}

	// Wait for a response if necessary
	if (m_sync)
	{
		// Wait for the response
		ACE_InputCDR* input = 0;
		if (m_handler->get_response(m_trans_key,input,wait) != 0 || input==0)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No response recieved\n")),-1);
		
		// Read the response code
		ACE_CDR::Long ret_code;
		if (!input->read_long(ret_code))
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to read return code\n")),-1);

		if (ret_code != 0)
		{
			delete input;
			return ret_code;
		}

		if (input_response(*input) != 0)
		{
			delete input;
			return -1;
		}

		delete input;
	}

	return 0;
}
