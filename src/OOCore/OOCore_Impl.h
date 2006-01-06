//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "OOCore.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_OOCORE_IMPL_H_INCLUDED_
#define OOCORE_OOCORE_IMPL_H_INCLUDED_

#include "./OOCore_Util.h"

// This is a shoddy fixup for compilers with broken explicit template specialisation
/*#if (__GNUC__) && (__GNUC__ <= 3)
	#define EXPLICIT_TEMPLATE(m,t)	template m<t>
#else
	#define EXPLICIT_TEMPLATE(m,t)	m<t>
#endif*/

#include <functional>

namespace std
{
template<>
struct less< OOCore::ProxyStubManager::cookie_t > : public binary_function<OOCore::ProxyStubManager::cookie_t, OOCore::ProxyStubManager::cookie_t, bool> 
{
	bool operator()(const OOCore::ProxyStubManager::cookie_t& _Left, const OOCore::ProxyStubManager::cookie_t& _Right) const
	{
		return (_Left.slot_generation() <= _Right.slot_generation() &&
				_Left.slot_index() < _Right.slot_index());
	}
};
};

namespace OOCore
{
namespace Impl
{
	extern bool g_IsServer;

#ifdef ACE_WIN32
	extern HINSTANCE g_hInstance;
#endif

	// IO helpers
	class InputStream_Wrapper
	{
	public:
		InputStream_Wrapper(OOCore::InputStream* input) :
		  m_in(input)
		{ }

		InputStream_Wrapper& operator = (OOCore::InputStream* input)
		{
			m_in = input;
			return *this;
		}

		int read(OOObject::bool_t& in) { return m_in->ReadBoolean(in); }
		int read(OOObject::char_t& in) { return m_in->ReadChar(in); }
		int read(OOObject::int16_t& in) { return m_in->ReadShort(in); }
		int read(OOObject::uint16_t& in) { return m_in->ReadUShort(in); }
		int read(OOObject::int32_t& in) { return m_in->ReadLong(in); }
		int read(OOObject::uint32_t& in) { return m_in->ReadULong(in); }
		int read(OOObject::int64_t& in) { return m_in->ReadLongLong(in); }
		int read(OOObject::uint64_t& in) { return m_in->ReadULongLong(in); }
		int read(OOObject::real4_t& in) { return m_in->ReadFloat(in); }
		int read(OOObject::real8_t& in) { return m_in->ReadDouble(in); }
		
		int read(OOCore::ProxyStubManager::cookie_t& val)
		{
			OOObject::byte_t* buf;
			ACE_NEW_RETURN(buf,OOObject::byte_t[ACE_Active_Map_Manager_Key::size()],-1);
			if (read_bytes(buf,ACE_Active_Map_Manager_Key::size())!=0) 
			{
				delete [] buf;
				return -1;
			}
			
			val.decode(buf);
			delete [] buf;
			return 0;
		}

		int read(OOObject::guid_t& val) 
		{
			if (m_in->ReadULong(val.Data1) != 0) return -1;
			if (m_in->ReadUShort(val.Data2) != 0) return -1;
			if (m_in->ReadUShort(val.Data3) != 0) return -1;
			return read_bytes(val.Data4,8);
		}

		// Work around for the fact that on some platforms ACE_CDR::Boolean is typedef'd as unsigned char
		int read(ACE::If_Then_Else<(sizeof(bool)==1),OOObject::byte_t,OOObject::byte_t[2]>::result_type& in)
		{ 
			return read_byte_workaround(in); 
		}

		operator OOCore::InputStream*()
		{
			return m_in;
		}

		OOCore::InputStream* operator ->()
		{
			return m_in;
		}

	private:
		OOCore::Object_Ptr<OOCore::InputStream> m_in;

		int read_byte_workaround(OOObject::byte_t& in)
		{
			return m_in->ReadByte(in);
		}

		int read_byte_workaround(OOObject::byte_t in[2])
		{
			return read_bytes(in,2);
		}

		int read_bytes(OOObject::byte_t* b, size_t c)
		{
			for (size_t i=0;i<c;++i)
			{
				if (m_in->ReadByte(b[i]) != 0) return -1;
			}
			return 0;
		}
	};

	class OutputStream_Wrapper
	{
	public:
		OutputStream_Wrapper(OOCore::OutputStream* output) :
		  m_out(output)
		{ }

		int write(const OOObject::bool_t& out) { return m_out->WriteBoolean(out); }
		int write(const OOObject::char_t& out) { return m_out->WriteChar(out); }
		int write(const OOObject::int16_t& out) { return m_out->WriteShort(out); }
		int write(const OOObject::uint16_t& out) { return m_out->WriteUShort(out); }
		int write(const OOObject::int32_t& out) { return m_out->WriteLong(out); }
		int write(const OOObject::uint32_t& out) { return m_out->WriteULong(out); }
		int write(const OOObject::int64_t& out) { return m_out->WriteLongLong(out); }
		int write(const OOObject::uint64_t& out) { return m_out->WriteULongLong(out); }
		int write(const OOObject::real4_t& out) { return m_out->WriteFloat(out); }
		int write(const OOObject::real8_t& out) { return m_out->WriteDouble(out); }
		
		int write(const OOCore::ProxyStubManager::cookie_t& val) 
		{ 
			OOObject::byte_t* buf;
			ACE_NEW_RETURN(buf,OOObject::byte_t[ACE_Active_Map_Manager_Key::size()],-1);
			val.encode(buf);
			int ret = write_bytes(buf,ACE_Active_Map_Manager_Key::size());
			delete [] buf;
			return ret;
		}
		
		int write(const OOObject::guid_t& val)
		{
			if (m_out->WriteULong(val.Data1) != 0) return -1;
			if (m_out->WriteUShort(val.Data2) != 0) return -1;
			if (m_out->WriteUShort(val.Data3) != 0) return -1;
			return write_bytes(val.Data4,8);
		}

		// Work around for the fact that on some platforms ACE_CDR::Boolean is typedef'd as unsigned char
		int write(const ACE::If_Then_Else<(sizeof(bool)==1),OOObject::byte_t,OOObject::byte_t[2]>::result_type& out)
		{ 
			return write_byte_workaround(out); 
		}

		operator OOCore::OutputStream*()
		{
			return m_out;
		}

		OOCore::OutputStream* operator ->()
		{
			return m_out;
		}

	private:
		OOCore::Object_Ptr<OOCore::OutputStream> m_out;

		int write_byte_workaround(const OOObject::byte_t& out)
		{
			return m_out->WriteByte(out);
		}

		int write_byte_workaround(const OOObject::byte_t out[2])
		{
			return write_bytes(out,2);
		}

		int write_bytes(const OOObject::byte_t* p, size_t c)
		{
			for (size_t i=0;i<c;++i)
			{
				if (m_out->WriteByte(p[i]) != 0) return -1;
			}
			return 0;
		}
	};
};
};

#endif // OOCORE_OOCORE_IMPL_H_INCLUDED_
