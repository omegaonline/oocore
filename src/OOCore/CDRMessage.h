///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOCORE_CDR_MESSAGE_H_INCLUDED_
#define OOCORE_CDR_MESSAGE_H_INCLUDED_

namespace OOCore
{
	interface ICDRStreamHolder : public Omega::IObject
	{
		virtual void* GetCDRStream() = 0;
	};

	// {1455FCD0-A49B-4f2a-94A5-222949957123}
	OMEGA_DECLARE_OID(OID_CDRMessageMarshalFactory);
}

OMEGA_DEFINE_INTERFACE_LOCAL
(
	OOCore, ICDRStreamHolder, "{5251283B-95C8-4e5b-9136-5DDCBE636A4E}",

	// Methods
	OMEGA_METHOD(void*,GetCDRStream,0,())
)

// Some macros to help

#define OOCORE_DEFINE_MESSAGE_READ(name,type) \
	size_t name(const wchar_t*,size_t count, Omega::type* arr) \
	{ \
		if (count > (size_t)-1 / sizeof(Omega::type)) \
			OMEGA_THROW(L"Overflow"); \
		size_t i; \
		for (i=0;i<count;++i) \
		{ \
			if (!m_stream.read(arr[i])) \
				break; \
		} \
		return i; \
	}

#define OOCORE_DEFINE_MESSAGE_WRITE(name,type) \
	void name(const wchar_t*,size_t count, const Omega::type* arr) \
	{ \
		if (count > (size_t)-1 / sizeof(Omega::type)) \
			OMEGA_THROW(L"Overflow"); \
		for (size_t i=0;i<count;++i) \
		{ \
			if (!m_stream.write(arr[i])) \
				OMEGA_THROW(m_stream.last_error()); \
		} \
	}

namespace OOCore
{
	class CDRMessage :
		public OTL::ObjectBase,
		public OOCore::ICDRStreamHolder,
		public Omega::Remoting::IMessage,
		public Omega::Remoting::IMarshal
	{
	public:
		CDRMessage()
		{
		}

		virtual ~CDRMessage()
		{
		}

		void init(const OOBase::CDRStream& stream)
		{
			m_stream = stream;
		}
		
		BEGIN_INTERFACE_MAP(CDRMessage)
			INTERFACE_ENTRY(Omega::Remoting::IMessage)
			INTERFACE_ENTRY(Omega::Remoting::IMarshal)
			INTERFACE_ENTRY(OOCore::ICDRStreamHolder)
		END_INTERFACE_MAP()

	private:
		OOBase::CDRStream m_stream;

		Omega::uint16_t ReadUInt16()
		{ 
			Omega::uint16_t val; 
			if (!m_stream.read(val)) 
				OMEGA_THROW(m_stream.last_error()); 
			return val; 
		}

		Omega::uint32_t ReadUInt32()
		{ 
			Omega::uint32_t val; 
			if (!m_stream.read(val)) 
				OMEGA_THROW(m_stream.last_error());
			return val;
		}

		Omega::string_t ReadString()
		{ 
			std::string val; 
			if (!m_stream.read(val)) 
				OMEGA_THROW(m_stream.last_error());

			return Omega::string_t(val.c_str(),true); 
		}

		Omega::guid_t ReadGuid()
		{
			Omega::guid_t g;
			g.Data1 = ReadUInt32();
			g.Data2 = ReadUInt16();
			g.Data3 = ReadUInt16();
			Omega::uint64_t bytes = 8;
			ReadBytes(bytes,g.Data4);
			if (bytes != 8)
				OMEGA_THROW(m_stream.last_error());
			return g;
		}

		void WriteUInt16(Omega::uint16_t val)
		{ 
			if (!m_stream.write(val)) 
				OMEGA_THROW(m_stream.last_error());
		}
		
		void WriteUInt32(Omega::uint32_t val)
		{ 
			if (!m_stream.write(val)) 
				OMEGA_THROW(m_stream.last_error()); 
		}

		void WriteString(const Omega::string_t& val)
		{ 
			if (!m_stream.write(val.ToUTF8())) 
				OMEGA_THROW(m_stream.last_error()); 
		}

		void WriteGuid(const Omega::guid_t& val)
		{
			WriteUInt32(val.Data1);
			WriteUInt16(val.Data2);
			WriteUInt16(val.Data3);
			WriteBytes((size_t)8,val.Data4);
		}

		void ReadBytes(Omega::uint64_t& cbBytes, Omega::byte_t* val)
		{
			cbBytes = m_stream.read_bytes(val,static_cast<size_t>(cbBytes));
		}

		void WriteBytes(const Omega::uint64_t& cbBytes, const Omega::byte_t* val)
		{ 
			m_stream.write_bytes(val,static_cast<size_t>(cbBytes));
		}

	// ICDRStreamHolder
	public:
		void* GetCDRStream()
		{
			return &m_stream;
		}

	// IMarshal members
	public: 
		Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			return OID_CDRMessageMarshalFactory;
		}

		void MarshalInterface(Omega::Remoting::IObjectManager*, Omega::Remoting::IMessage* pMessage, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			Omega::uint64_t sz = m_stream.buffer()->length();
			pMessage->WriteUInt64s(L"length",1,&sz);
			pMessage->WriteBytes(L"data",static_cast<size_t>(sz),reinterpret_cast<const Omega::byte_t*>(m_stream.buffer()->rd_ptr()));
		}

		void ReleaseMarshalData(Omega::Remoting::IObjectManager*, Omega::Remoting::IMessage* pMessage, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			Omega::uint64_t sz;
			pMessage->ReadUInt64s(L"length",1,&sz);
			if (sz > (size_t)-1)
				OMEGA_THROW(L"Overflow"); \
			size_t len = static_cast<size_t>(sz);
			OOBase::SmartPtr<Omega::byte_t,OOBase::ArrayDestructor<Omega::byte_t> > szBuf = 0;
			OMEGA_NEW(szBuf,Omega::byte_t[len]);
			pMessage->ReadBytes(L"data",len,szBuf.value());
		}

	// IMessage members
	public:
		OOCORE_DEFINE_MESSAGE_READ(ReadBooleans,bool_t)
		OOCORE_DEFINE_MESSAGE_READ(ReadBytes,byte_t)
		OOCORE_DEFINE_MESSAGE_READ(ReadInt16s,int16_t)
		OOCORE_DEFINE_MESSAGE_READ(ReadUInt16s,uint16_t)
		OOCORE_DEFINE_MESSAGE_READ(ReadInt32s,int32_t)
		OOCORE_DEFINE_MESSAGE_READ(ReadUInt32s,uint32_t)
		OOCORE_DEFINE_MESSAGE_READ(ReadInt64s,int64_t)
		OOCORE_DEFINE_MESSAGE_READ(ReadUInt64s,uint64_t)
		OOCORE_DEFINE_MESSAGE_READ(ReadFloat4s,float4_t)
		OOCORE_DEFINE_MESSAGE_READ(ReadFloat8s,float8_t)
		
		size_t ReadStrings(const wchar_t*, size_t count, Omega::string_t* arr)
		{ 
			if (count > (size_t)-1 / sizeof(Omega::string_t))
				OMEGA_THROW(L"Overflow"); \
			
			for (size_t i=0;i<count;++i)
				arr[i] = ReadString();

			return count;
		}

		size_t ReadGuids(const wchar_t*, size_t count, Omega::guid_t* arr)
		{
			if (count > (size_t)-1 / sizeof(Omega::guid_t))
				OMEGA_THROW(L"Overflow"); \
			
			for (size_t i=0;i<count;++i)
				arr[i] = ReadGuid();

			return count;
		}

		void ReadStructStart(const wchar_t*, const wchar_t*)
			{ /* NOP */	}

		void ReadStructEnd(const wchar_t*)
			{ /* NOP */	}

		OOCORE_DEFINE_MESSAGE_WRITE(WriteBooleans,bool_t)
		OOCORE_DEFINE_MESSAGE_WRITE(WriteBytes,byte_t)
		OOCORE_DEFINE_MESSAGE_WRITE(WriteInt16s,int16_t)
		OOCORE_DEFINE_MESSAGE_WRITE(WriteUInt16s,uint16_t)
		OOCORE_DEFINE_MESSAGE_WRITE(WriteInt32s,int32_t)
		OOCORE_DEFINE_MESSAGE_WRITE(WriteUInt32s,uint32_t)
		OOCORE_DEFINE_MESSAGE_WRITE(WriteInt64s,int64_t)
		OOCORE_DEFINE_MESSAGE_WRITE(WriteUInt64s,uint64_t)
		OOCORE_DEFINE_MESSAGE_WRITE(WriteFloat4s,float4_t)
		OOCORE_DEFINE_MESSAGE_WRITE(WriteFloat8s,float8_t)

		void WriteStrings(const wchar_t*, size_t count, const Omega::string_t* arr)
		{ 
			for (size_t i=0;i<count;++i)
				WriteString(arr[i]);
		}

		void WriteGuids(const wchar_t*, size_t count, const Omega::guid_t* arr)
		{
			for (size_t i=0;i<count;++i)
				WriteGuid(arr[i]);
		}

		void WriteStructStart(const wchar_t*, const wchar_t*)
			{ /* NOP */	}

		void WriteStructEnd(const wchar_t*)
			{ /* NOP */	}
	};
}

#endif // OOCORE_CDR_MESSAGE_H_INCLUDED_
