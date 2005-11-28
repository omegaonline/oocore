#include "./Marshaller.h"

bool 
Marshall_A::IOWrappers::read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::bool_t& val, bool response)
{
	return (input->ReadBoolean(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::char_t& val, bool response)
{
	return (input->ReadChar(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::byte_t& val, bool response)
{
	return (input->ReadByte(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::int16_t& val, bool response)
{
	return (input->ReadShort(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::uint16_t& val, bool response)
{
	return (input->ReadUShort(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::int32_t& val, bool response)
{
	return (input->ReadLong(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::uint32_t& val, bool response)
{
	return (input->ReadULong(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::int64_t& val, bool response)
{
	return (input->ReadLongLong(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::uint64_t& val, bool response)
{
	return (input->ReadULongLong(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::real4_t& val, bool response)
{
	return (input->ReadFloat(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::real8_t& val, bool response)
{
	return (input->ReadDouble(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::cookie_t& val, bool response)
{
	return (input->ReadCookie(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::guid_t& val, bool response)
{
	return (input->ReadGuid(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::bool_t& val, bool response)
{
	return (output->WriteBoolean(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::char_t& val, bool response)
{
	return (output->WriteChar(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::byte_t& val, bool response)
{
	return (output->WriteByte(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::int16_t& val, bool response)
{
	return (output->WriteShort(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::uint16_t& val, bool response)
{
	return (output->WriteUShort(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::int32_t& val, bool response)
{
	return (output->WriteLong(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::uint32_t& val, bool response)
{
	return (output->WriteULong(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::int64_t& val, bool response)
{
	return (output->WriteLongLong(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::uint64_t& val, bool response)
{
	return (output->WriteULongLong(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::real4_t& val, bool response)
{
	return (output->WriteFloat(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::real8_t& val, bool response)
{
	return (output->WriteDouble(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::cookie_t& val, bool response)
{
	return (output->WriteCookie(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::guid_t& val, bool response)
{
	return (output->WriteGuid(val)==0 ? true : false);
}

bool 
Marshall_A::IOWrappers::write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::char_t* val, bool response)
{
	OOObj::uint32_t len = ACE_OS::strlen(val);

	return (output->WriteBytes(reinterpret_cast<const OOObj::byte_t*>(val),len)==0 ? true : false);
}

Marshall_A::Marshaller_Base::Marshaller_Base(OOCore::ProxyStubManager* manager, bool failed) :
	m_failed(failed || manager==0), m_manager(manager)
{
}

Marshall_A::Marshaller_Base::~Marshaller_Base()
{
	for (unsigned int i=0;i<m_params.size();++i)
	{
		delete m_params[i];
	}
}

int 
Marshall_A::Marshaller_Base::output_response(OOCore::OutputStream* output)
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

int 
Marshall_A::Marshaller_Base::input_response(OOCore::InputStream* input)
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

size_t 
Marshall_A::Marshaller_Base::param_size()
{
	return m_params.size();
}

/*void 
Marshall_A::Marshaller_Base::fail()
{
	m_failed = true;
}*/

/*bool 
Marshall_A::Marshaller_Base::is_okay()
{
	return !m_failed;
}*/

int 
Marshall_A::Marshaller_Base::create_proxy(const OOObj::guid_t& iid, const OOObj::cookie_t& key, OOObj::Object** ppVal)
{
	if (!m_manager || m_failed)
		return -1;

	return m_manager->CreateProxy(iid,key,ppVal);
}

int 
Marshall_A::Marshaller_Base::create_stub(const OOObj::guid_t& iid, OOObj::Object* pObj, OOCore::OutputStream* output)
{
	if (!m_manager || m_failed)
		return -1;

	return m_manager->CreateStub(iid,pObj,output);
}

int 
Marshall_A::Marshaller_Base::send_and_recv(OOCore::OutputStream* output, OOObj::uint32_t trans_id, OOCore::InputStream** input)
{
	if (!m_manager || m_failed)
		return -1;

	return m_manager->SendAndReceive(output,trans_id,input);
}
