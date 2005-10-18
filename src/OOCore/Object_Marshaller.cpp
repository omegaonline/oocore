#include "./Object_Marshaller.h"
#include "./Marshaller.h"
#include "./ProxyStub_Handler.h"

ACE_CDR::Boolean OOCore_Object_Marshaller::read(OOCore_Marshaller_Base& mshl, ACE_InputCDR& input, bool response)
{
	if (m_node==0)
		return false;

	if (!response)
	{
		if (!input.read_boolean(m_in))
			return false;

		if (!input.read_boolean(m_out))
			return false;

		if (!(input >> m_iid))
			return false;
	}

	if ((!response && m_in) || (response && m_out))
	{
		OOObj::cookie_t key;
		if (!(input >> key))
			return false;
	
		// Create a proxy
		if (mshl.handler()->create_proxy(m_iid,key,m_objref) != 0)
			return false;
	}
	
	return true;
}

ACE_CDR::Boolean OOCore_Object_Marshaller::write(OOCore_Marshaller_Base& mshl, ACE_OutputCDR& output, bool response) const
{
	if (m_node==0)
		return false;

	if (!response)
	{
		if (!output.write_boolean(m_in))
			return false;

		if (!output.write_boolean(m_out))
			return false;

		if (!(output << m_iid))
			return false;
	}

	if ((!response && m_in) || (response && m_out))
	{
		// Create and output the stub
		OOObj::cookie_t key;
		if (mshl.handler()->create_stub(m_iid,m_node->m_obj,key) != 0)
			return false;

		if (!(output << key))
		{
			mshl.handler()->remove_stub(key);
			return false;
		}
	}

	return true;
}
