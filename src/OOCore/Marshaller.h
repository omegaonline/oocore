//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "Object.h" instead
//
//////////////////////////////////////////////////////

#pragma once

#include <ace/Active_Map_Manager.h>
#include <ace/Vector_T.h>
#include <ace/CDR_Stream.h>

class OOCore_ProxyStub_Handler;
class OOCore_Marshaller_Base;

template <class T>
bool read_param(OOCore_Marshaller_Base* mshl, ACE_InputCDR& input, T& val, bool response)
{
	return (input >> val);
}

template <class T>
bool read_param(OOCore_Marshaller_Base* mshl, ACE_InputCDR& input, T* val, bool response)
{
	return (input >> (*val));
}

template <class T>
bool write_param(OOCore_Marshaller_Base* mshl, ACE_OutputCDR& output, const T& val, bool response)
{
	return (output << val);
}

template <class T>
bool write_param(OOCore_Marshaller_Base* mshl, ACE_OutputCDR& output, T* val, bool response)
{
	return (output << (*val));
}

template <class T>
bool arg_responds(const T& val)
{
	return false;
}

bool write_param(OOCore_Marshaller_Base* mshl, ACE_OutputCDR& output, const ACE_CDR::Char* val, bool response);

class OOCore_Marshalled_Param_Holder_Base
{
public:
	virtual ~OOCore_Marshalled_Param_Holder_Base()
	{ }

	virtual bool write_response(OOCore_Marshaller_Base* mshl, ACE_OutputCDR& output) = 0;
	virtual bool read_response(OOCore_Marshaller_Base* mshl, ACE_InputCDR& input) = 0;
};

template <class T>
class OOCore_Marshalled_Param_Holder :
	public OOCore_Marshalled_Param_Holder_Base
{
public:
	OOCore_Marshalled_Param_Holder(const T& tin, bool responds) :
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

	bool write_response(OOCore_Marshaller_Base* mshl, ACE_OutputCDR& output)
	{
		if (m_responds)
			return write_param(mshl,output,t,true);
		else
			return true;
	}

	bool read_response(OOCore_Marshaller_Base* mshl, ACE_InputCDR& input)
	{
		if (m_responds)
			return read_param(mshl,input,t,true);
		else
			return true;
	}

private:
	T t;
	bool m_responds;
};

class OOCore_Marshaller_Base
{
public:
	template <class T>
	OOCore_Marshalled_Param_Holder<T>* param(unsigned int index)
	{
		return static_cast<OOCore_Marshalled_Param_Holder<T>*>(m_params[index]);
	}

	void fail()
	{
		m_failed = true;
	}

	bool is_okay()
	{
		return !m_failed;
	}

	int output_response(ACE_OutputCDR& output);
	int input_response(ACE_InputCDR& input);

	template <class T>
	OOCore_Marshalled_Param_Holder<T>* pack_param(const T& val, bool responds)
	{
		OOCore_Marshalled_Param_Holder<T>* p;
		ACE_NEW_RETURN(p,OOCore_Marshalled_Param_Holder<T>(val,responds),0);
		m_params.push_back(p);
		return p;
	}

	OOCore_ProxyStub_Handler* handler() const
	{
		return m_handler;
	}

protected:
	OOCore_Marshaller_Base(OOCore_ProxyStub_Handler* handler, bool sync, bool failed = false);
	virtual ~OOCore_Marshaller_Base();

	bool m_failed;
	bool m_sync;
	OOCore_ProxyStub_Handler* m_handler;
	ACE_Vector<OOCore_Marshalled_Param_Holder_Base*,16> m_params;
};