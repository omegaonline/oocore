//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "ProxyStub.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_MARSHALLER_H_INCLUDED_
#define OOCORE_MARSHALLER_H_INCLUDED_

#include <ace/Active_Map_Manager.h>
#include <ace/Vector_T.h>

#include "./OOCore_Util.h"

namespace Marshall_A
{

class Marshaller_Base;

namespace IOWrappers
{
	bool read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::bool_t& val, bool response);
	bool read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::char_t& val, bool response);
	bool read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::byte_t& val, bool response);
	bool read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::int16_t& val, bool response);
	bool read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::uint16_t& val, bool response);
	bool read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::int32_t& val, bool response);
	bool read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::uint32_t& val, bool response);
	bool read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::int64_t& val, bool response);
	bool read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::uint64_t& val, bool response);
	bool read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::real4_t& val, bool response);
	bool read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::real8_t& val, bool response);
	bool read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::cookie_t& val, bool response);
	bool read_param(Marshaller_Base* mshl, OOCore::InputStream* input, OOObj::guid_t& val, bool response);

	bool write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::bool_t& val, bool response);
	bool write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::char_t& val, bool response);
	bool write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::byte_t& val, bool response);
	bool write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::int16_t& val, bool response);
	bool write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::uint16_t& val, bool response);
	bool write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::int32_t& val, bool response);
	bool write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::uint32_t& val, bool response);
	bool write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::int64_t& val, bool response);
	bool write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::uint64_t& val, bool response);
	bool write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::real4_t& val, bool response);
	bool write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::real8_t& val, bool response);
	bool write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::cookie_t& val, bool response);
	bool write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::guid_t& val, bool response);
	bool write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const OOObj::string_t val, bool response);

	template <class T>
	bool read_param(Marshaller_Base* mshl, OOCore::InputStream* input, T* val, bool response)
	{
		return read_param(mshl,input,*val,response);
	}

	template <class T>
	bool write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const T* val, bool response)
	{
		return write_param(mshl,output,*val,response);
	}

	template <class T>
	bool arg_responds(const T& val)
	{
		return false;
	}
};

class Marshalled_Param_Holder_Base
{
public:
	virtual bool write_response(Marshaller_Base* mshl, OOCore::OutputStream* output) = 0;
	virtual bool read_response(Marshaller_Base* mshl, OOCore::InputStream* input) = 0;
};

template <class T>
class Marshalled_Param_Holder :
	public Marshalled_Param_Holder_Base
{
public:
	Marshalled_Param_Holder(const T& tin, bool responds) :
		t(tin), m_responds(responds)
	{ }

	T* addr()
	{
		return &t;
	}

	T& value()
	{
		return t;
	}

	bool write_response(Marshaller_Base* mshl, OOCore::OutputStream* output)
	{
		if (m_responds)
			return IOWrappers::write_param(mshl,output,t,true);
		else
			return true;
	}

	bool read_response(Marshaller_Base* mshl, OOCore::InputStream* input)
	{
		if (m_responds)
			return IOWrappers::read_param(mshl,input,t,true);
		else
			return true;
	}

private:
	T t;
	bool m_responds;
};

class Marshaller_Base
{
public:
	template <class T> Marshalled_Param_Holder<T>* param(unsigned int index)
	{
		return static_cast<Marshalled_Param_Holder<T>*>(m_params[index]);
	}

	int create_proxy(const OOObj::guid_t& iid, const OOObj::cookie_t& key, OOObj::Object** ppVal);
	int create_stub(const OOObj::guid_t& iid, OOObj::Object* pObj, OOCore::OutputStream* output);
	
	int output_response(OOCore::OutputStream* output);

protected:
	Marshaller_Base(OOCore::ProxyStubManager* manager, bool failed = false);
	virtual ~Marshaller_Base();

	bool m_failed;

	int input_response(OOCore::InputStream* input);
	int send_and_recv(OOCore::OutputStream* output, OOObj::uint32_t trans_id, OOCore::InputStream** input);
	
	template <class T>
	Marshalled_Param_Holder<T>* pack_param(const T& val, bool responds)
	{
		Marshalled_Param_Holder<T>* p;
		ACE_NEW_RETURN(p,Marshalled_Param_Holder<T>(val,responds),0);
		m_params.push_back(p);
		return p;
	}

	size_t param_size();

private:
	OOCore::Object_Ptr<OOCore::ProxyStubManager> m_manager;
	ACE_Vector<Marshalled_Param_Holder_Base*,16> m_params;
};

};

#endif // OOCORE_MARSHALLER_H_INCLUDED_
