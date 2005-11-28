//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "ProxyStub.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_STUB_MARSHALLER_H_INCLUDED_
#define OOCORE_STUB_MARSHALLER_H_INCLUDED_

#include "./Array_Marshaller.h"
#include "./String_Marshalling.h"

namespace Marshall_A
{

class Stub_Marshaller : public Marshaller_Base
{
public:
	Stub_Marshaller(OOCore::ProxyStubManager* manager, OOCore::InputStream* input);
	virtual ~Stub_Marshaller();

	template <class T> T unpack()
	{
		// We call through again because of C++ return value problems
		T t;
		if (!m_failed)
			m_failed = !unpack_i(t);
		
		return t;
	}

private:
	OOCore::Object_Ptr<OOCore::InputStream> m_input;
	
	template <class T> bool unpack_i(T& val)
	{
		if (!IOWrappers::read_param(this,m_input,val,false))
			return false;
		
		return (pack_param(val,false) != 0);
	}

    template <class T> bool unpack_i_t(T*& val)
	{
		// Get the index
		OOObj::uint32_t index;
		if (m_input->ReadULong(index) != 0)
			return false;

		if (index == static_cast<OOObj::uint32_t>(-1))
		{
			// Just a pointer
			T t;
			if (!IOWrappers::read_param(this,m_input,t,false))
				return false;

			// Put in map
			Marshalled_Param_Holder<T>* p = pack_param(t,true);
			if (p == 0)
				return false;
			
			// Pass out address
			val = p->addr();
			return true;
		}
		else
		{
			// Array!
			if (param_size()<index)
				return false;

			Array_Marshaller<T> arr(index);
			if (!arr.read(*this,m_input,false))
				return false;

			Marshalled_Param_Holder<Array_Marshaller<T> >* p = pack_param(arr,true);
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
		if (m_input->ReadULong(index) != 0)
			return false;

		// Check the index
		if (param_size()<index)
			return false;

		Array_Marshaller<T> arr(index);
		if (!arr.read(*this,m_input,false))
			return false;

		Marshalled_Param_Holder<Array_Marshaller<T> >* p = pack_param(arr,true);
		if (p == 0)
			return false;
		
		// Pass out value
		val = p->value().dataref();
		return true;
	}

	bool unpack_i(OOObj::cookie_t*& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::bool_t*& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::char_t*& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::int16_t*& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::uint16_t*& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::int32_t*& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::uint32_t*& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::int64_t*& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::uint64_t*& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::real4_t*& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::real8_t*& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::bool_t**& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::char_t**& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::int16_t**& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::uint16_t**& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::int32_t**& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::uint32_t**& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::int64_t**& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::uint64_t**& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::real4_t**& val) { return unpack_i_t(val); }
	bool unpack_i(OOObj::real8_t**& val) { return unpack_i_t(val); }

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
	
	bool unpack_object_p(OOObj::Object*& val, const OOObj::guid_t& iid);
	bool unpack_object_pp(OOObj::Object**& val, const OOObj::guid_t& iid);
};

template <> inline const OOObj::guid_t& Stub_Marshaller::unpack()
{
	OOObj::guid_t t;
	if (!m_failed)
		m_failed = (m_input->ReadGuid(t)==0 ? false : true);

	if (!m_failed)
	{
		Marshalled_Param_Holder<OOObj::guid_t>* p = pack_param(t,false);
		if (p==0)
			m_failed = true;
		else
			return p->value();	
	}
	return OOObj::guid_t::NIL;
}

template <> inline const OOObj::char_t*& Stub_Marshaller::unpack()
{
	static const OOObj::char_t* null_string = 0;

	OOObj::uint32_t len;
	if (m_input->ReadULong(len) != 0)
	{
		m_failed = true;
		return null_string;
	}

	OOObj::byte_t* buf=0;
	ACE_NEW_NORETURN(buf,OOObj::byte_t[len+1]);
	if (buf==0)
	{
		m_failed = true;
		return null_string;
	}

	StringHolder str(reinterpret_cast<OOObj::char_t*>(buf));
	
	if (m_input->ReadBytes(buf,len))
	{
		m_failed = true;
		return null_string;
	}
	buf[len] = 0;

	Marshalled_Param_Holder<StringHolder>* p = pack_param(str,false);
	if (p == 0)
	{
		m_failed = true;
		return null_string;
	}
	
	// Pass out value
	return p->value().const_ref();
}

template <> inline const OOObj::char_t* Stub_Marshaller::unpack()
{
	static const OOObj::char_t* null_string = 0;

	OOObj::uint32_t len;
	if (m_input->ReadULong(len) != 0)
	{
		m_failed = true;
		return null_string;
	}

	OOObj::byte_t* buf=0;
	ACE_NEW_NORETURN(buf,OOObj::byte_t[len+1]);
	if (buf==0)
	{
		m_failed = true;
		return null_string;
	}

	StringHolder str(reinterpret_cast<OOObj::char_t*>(buf));
	
	if (m_input->ReadBytes(buf,len))
	{
		m_failed = true;
		return null_string;
	}
	buf[len] = 0;

	Marshalled_Param_Holder<StringHolder>* p = pack_param(str,false);
	if (p == 0)
	{
		m_failed = true;
		return null_string;
	}
	
	// Pass out value
	return p->value().const_ref();
}

};

#endif // OOCORE_STUB_MARSHALLER_H_INCLUDED_
