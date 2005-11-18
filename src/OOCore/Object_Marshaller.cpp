#include "./Object_Marshaller.h"

bool 
Impl::IOWrappers::read_param(Marshaller_Base* mshl, OOCore::InputStream* input, Object_Marshaller& val, bool response)
{
	return val.read(*mshl,input,response);
}

bool 
Impl::IOWrappers::write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const Object_Marshaller& val, bool response)
{
	return val.write(*mshl,output,response);
}

bool 
Impl::IOWrappers::arg_responds(const Object_Marshaller& val)
{
	return val.responds();
}

ACE_CDR::Boolean 
Impl::Object_Marshaller::read(Marshaller_Base& mshl, OOCore::InputStream* input, bool response)
{
	if (m_node==0)
		return false;

	if (!response)
	{
		if (input->ReadBoolean(m_in) != 0)
			return false;

		if (input->ReadBoolean(m_out) != 0)
			return false;

		if (input->ReadGuid(m_iid) != 0)
			return false;
	}

	if ((!response && m_in) || (response && m_out))
	{
		OOObj::cookie_t key;
		if (input->ReadCookie(key) != 0)
			return false;
	
		// Create a proxy
		if (mshl.create_proxy(m_iid,key,m_objref) != 0)
			return false;
	}
	
	return true;
}

ACE_CDR::Boolean 
Impl::Object_Marshaller::write(Marshaller_Base& mshl, OOCore::OutputStream* output, bool response) const
{
	if (m_node==0)
		return false;

	if (!response)
	{
		if (output->WriteBoolean(m_in) != 0)
			return false;

		if (output->WriteBoolean(m_out) != 0)
			return false;

		if (output->WriteGuid(m_iid) != 0)
			return false;
	}

	if ((!response && m_in) || (response && m_out))
	{
		// Create and output the stub
		if (mshl.create_stub(m_iid,m_node->m_obj,output) != 0)
			return false;
	}

	return true;
}
