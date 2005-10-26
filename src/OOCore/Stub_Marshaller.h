//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "Object.h" instead
//
//////////////////////////////////////////////////////

#ifndef _OOCORE_STUB_MARSHALLER_H_INCLUDED_
#define _OOCORE_STUB_MARSHALLER_H_INCLUDED_

#include "./Marshaller.h"
#include "./Object_Impl.h"
#include "./Array_Marshaller.h"

#include "./OOCore_export.h"

class OOCore_Transport_Service;

class OOCore_Export OOCore_Stub_Marshaller : public OOCore_Marshaller_Base
{
	friend class OOCore_ProxyStub_Handler;

public:
	OOCore_Stub_Marshaller(OOCore_ProxyStub_Handler* handler, ACE_InputCDR* input, bool sync);
	virtual ~OOCore_Stub_Marshaller();

	template <class T> T unpack()
	{
		// We call through again because of C++ return value problems
		T t;
		if (!m_failed)
			m_failed = !unpack_i(t);
		
		return t;
	}

private:
	ACE_InputCDR* m_input;
	
	template <class T> bool unpack_i(T& val)
	{
		if (!read_param(this,*m_input,val,false))
			return false;
		
		return (pack_param(val,false) != 0);
	}

    template <class T> bool unpack_i_t(T*& val)
	{
		// Get the index
		OOObj::uint32_t index;
		if (!m_input->read_ulong(index))
			return false;

		if (index == static_cast<OOObj::uint32_t>(-1))
		{
			// Just a pointer
			T t;
			if (!read_param(this,*m_input,t,false))
				return false;

			// Put in map
			OOCore_Marshalled_Param_Holder<T>* p = pack_param(t,true);
			if (p == 0)
				return false;
			
			// Pass out address
			val = p->addr();
			return true;
		}
		else
		{
			// Array!
			if (m_params.size()<index)
				return false;

			OOCore_Array_Marshaller<T> arr(index);
			if (!arr.read(*this,*m_input,false))
				return false;

			OOCore_Marshalled_Param_Holder<OOCore_Array_Marshaller<T> >* p = pack_param(arr,true);
			if (p == 0)
				return false;
			
			// Pass out value
			val = p->value().data();
			return true;
		}
	}

	template <class T> bool unpack_i_t(T**& val)
	{
		// Get the index
		OOObj::uint32_t index;
		if (!m_input->read_ulong(index))
			return false;

		// Check the index
		if (m_params.size()<index)
			return false;

		OOCore_Array_Marshaller<T> arr(index);
		if (!arr.read(*this,*m_input,false))
			return false;

		OOCore_Marshalled_Param_Holder<OOCore_Array_Marshaller<T> >* p = pack_param(arr,true);
		if (p == 0)
			return false;
		
		// Pass out value
		val = p->value().dataref();
		return true;
	}

	bool unpack_i(ACE_Active_Map_Manager_Key*& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::Boolean*& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::Char*& val) { return unpack_i_t(val); }
	//bool unpack_i(ACE_CDR::Octet*& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::Short*& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::UShort*& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::Long*& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::ULong*& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::LongLong*& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::ULongLong*& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::Float*& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::Double*& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::Boolean**& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::Char**& val) { return unpack_i_t(val); }
	//bool unpack_i(ACE_CDR::Octet**& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::Short**& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::UShort**& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::Long**& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::ULong**& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::LongLong**& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::ULongLong**& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::Float**& val) { return unpack_i_t(val); }
	bool unpack_i(ACE_CDR::Double**& val) { return unpack_i_t(val); }

	template <class T> bool unpack_i(T*& val)
	{
		// If you get a compliler warning on the line below, then you are trying to marshall an illegal type
		return unpack_object_p(reinterpret_cast<OOObj::Object*&>(val),T::IID);
	}

	template <class T> bool unpack_i(T**& val)
	{
		// If you get a compliler warning on the line below, then you are trying to marshall an illegal type
		return unpack_object_pp(reinterpret_cast<OOObj::Object**&>(val),T::IID);
	}
	
	bool unpack_object_p(OOObj::Object*& val, const OOObj::GUID& iid);
	bool unpack_object_pp(OOObj::Object**& val, const OOObj::GUID& iid);
	bool unpack_i(const ACE_CDR::Char*& val);
};

template <> inline const OOObj::GUID& OOCore_Stub_Marshaller::unpack()
{
	OOObj::GUID t;
	if (!m_failed)
		m_failed = !(*m_input >> t);

	if (!m_failed)
	{
		OOCore_Marshalled_Param_Holder<OOObj::GUID>* p = pack_param(t,false);
		if (p==0)
			m_failed = true;
		else
			return p->value();	
	}
	return OOObj::GUID::GUID_NIL;
}

#endif // _OOCORE_STUB_MARSHALLER_H_INCLUDED_
