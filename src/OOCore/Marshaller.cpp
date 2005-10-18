#include "./Marshaller.h"

#include "./ProxyStub_Handler.h"
#include "./Object_Marshaller.h"

bool read_param(OOCore_Marshaller_Base* mshl, ACE_InputCDR& input, OOCore_Object_Marshaller& val, bool response)
{
	return val.read(*mshl,input,response);
}

bool write_param(OOCore_Marshaller_Base* mshl, ACE_OutputCDR& output, const OOCore_Object_Marshaller& val, bool response)
{
	return val.write(*mshl,output,response);
}

bool arg_responds(const OOCore_Object_Marshaller& val)
{
	return val.responds();
}

OOCore_Marshaller_Base::OOCore_Marshaller_Base(OOCore_ProxyStub_Handler* handler, bool sync, bool failed) :
	m_handler(handler), m_sync(sync), m_failed(failed)
{
	if (m_handler != 0)
		m_handler->addref();
}

OOCore_Marshaller_Base::~OOCore_Marshaller_Base()
{
	for (unsigned int i=0;i<m_params.size();++i)
	{
		delete m_params[i];
	}

	if (m_handler != 0)
		m_handler->release();
}

int OOCore_Marshaller_Base::output_response(ACE_OutputCDR& output)
{
	for (unsigned int i=0;i<m_params.size();++i)
	{
		if (!m_params[i]->write_response(this,output))
		{
			m_failed = true;
			return -1;
		}
	}
	return 0;
}

int OOCore_Marshaller_Base::input_response(ACE_InputCDR& input)
{
	for (unsigned int i=0;i<m_params.size();++i)
	{
		if (!m_params[i]->read_response(this,input))
		{
			m_failed = true;
			return -1;
		}
	}
	return 0;
}
