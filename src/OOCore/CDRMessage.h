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

#include "../include/Omega/Remoting.h"

namespace OOCore
{
	// {1455FCD0-A49B-4f2a-94A5-222949957123}
	extern "C" const Omega::guid_t OID_CDRMessageMarshalFactory;
}

// Some macros to help

#define OOCORE_DEFINE_MESSAGE_READWRITE(name,r_type,w_type) \
	r_type OMEGA_CONCAT(Read,name)(const Omega::string_t&) \
	{ \
		r_type retval; \
		if (!m_stream.read(retval)) \
			OMEGA_THROW(m_stream.last_error()); \
		return retval; \
	} \
	void OMEGA_CONCAT(Read,OMEGA_CONCAT_R(name,s))(const Omega::string_t&, Omega::uint32_t count, r_type* arr) \
	{ \
		Omega::uint32_t actual = 0; \
		if (!m_stream.read(actual)) \
			OMEGA_THROW(m_stream.last_error()); \
		if (actual > count) \
			OMEGA_THROW(L"Over-read on memory message"); \
		for (Omega::uint32_t i=0;i<actual;++i) \
		{ \
			if (!m_stream.read(arr[i])) \
				OMEGA_THROW(m_stream.last_error()); \
		} \
	} \
	void OMEGA_CONCAT(Write,name)(const Omega::string_t&, w_type val) \
	{ \
		if (!m_stream.write(val)) \
			OMEGA_THROW(m_stream.last_error()); \
	} \
	void OMEGA_CONCAT(Write,OMEGA_CONCAT_R(name,s))(const Omega::string_t&, Omega::uint32_t count, const r_type* arr) \
	{ \
		if (!m_stream.write(count)) \
			OMEGA_THROW(m_stream.last_error()); \
		for (Omega::uint32_t i=0;i<count;++i) \
		{ \
			if (!m_stream.write(arr[i])) \
				OMEGA_THROW(m_stream.last_error()); \
		} \
	}

namespace OOCore
{
	class CDRMessage :
		public OTL::ObjectBase,
		public Omega::Remoting::IMessage,
		public Omega::Remoting::IMarshal
	{
	public:
		void init(const OOBase::CDRStream& stream)
		{
			m_stream = stream;
		}

		BEGIN_INTERFACE_MAP(CDRMessage)
			INTERFACE_ENTRY(Omega::Remoting::IMessage)
			INTERFACE_ENTRY(Omega::Remoting::IMarshal)
		END_INTERFACE_MAP()

	private:
		OOBase::CDRStream m_stream;

	public:
		const OOBase::CDRStream* GetCDRStream() const
		{
			return &m_stream;
		}

	// IMarshal members
	public:
		Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			return OID_CDRMessageMarshalFactory;
		}

		void MarshalInterface(Omega::Remoting::IMarshaller*, Omega::Remoting::IMessage* pMessage, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			if (m_stream.buffer()->length() > (Omega::uint32_t)-1)
				OMEGA_THROW(L"Message too long to marshal");

			Omega::uint32_t len = static_cast<Omega::uint32_t>(m_stream.buffer()->length());
			pMessage->WriteUInt32(L"length",len);
			pMessage->WriteBytes(L"data",len,reinterpret_cast<const Omega::byte_t*>(m_stream.buffer()->rd_ptr()));
		}

		void ReleaseMarshalData(Omega::Remoting::IMarshaller*, Omega::Remoting::IMessage* pMessage, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			Omega::uint32_t len = pMessage->ReadUInt32(L"length");
			OOBase::SmartPtr<Omega::byte_t,OOBase::ArrayDestructor<Omega::byte_t> > szBuf = 0;
			OMEGA_NEW(szBuf,Omega::byte_t[len]);
			pMessage->ReadBytes(L"data",len,szBuf);
		}

	// IMessage members
	public:
		OOCORE_DEFINE_MESSAGE_READWRITE(Boolean,Omega::bool_t,Omega::bool_t)
		OOCORE_DEFINE_MESSAGE_READWRITE(Int16,Omega::int16_t,Omega::int16_t)
		OOCORE_DEFINE_MESSAGE_READWRITE(UInt16,Omega::uint16_t,Omega::uint16_t)
		OOCORE_DEFINE_MESSAGE_READWRITE(Int32,Omega::int32_t,Omega::int32_t)
		OOCORE_DEFINE_MESSAGE_READWRITE(UInt32,Omega::uint32_t,Omega::uint32_t)
		OOCORE_DEFINE_MESSAGE_READWRITE(Int64,Omega::int64_t,const Omega::int64_t&)
		OOCORE_DEFINE_MESSAGE_READWRITE(UInt64,Omega::uint64_t,const Omega::uint64_t&)
		OOCORE_DEFINE_MESSAGE_READWRITE(Float4,Omega::float4_t,Omega::float4_t)
		OOCORE_DEFINE_MESSAGE_READWRITE(Float8,Omega::float8_t,const Omega::float8_t&)

		Omega::string_t ReadString(const Omega::string_t&)
		{
			std::string val;
			if (!m_stream.read(val))
				OMEGA_THROW(m_stream.last_error());

			return Omega::string_t(val.c_str(),true);
		}

		Omega::guid_t ReadGuid(const Omega::string_t&)
		{
			Omega::guid_t g;
			g.Data1 = ReadUInt32(Omega::string_t());
			g.Data2 = ReadUInt16(Omega::string_t());
			g.Data3 = ReadUInt16(Omega::string_t());
			ReadBytes(Omega::string_t(),8,g.Data4);
			return g;
		}

		void WriteString(const Omega::string_t&, const Omega::string_t& val)
		{
			if (!m_stream.write(val.ToUTF8()))
				OMEGA_THROW(m_stream.last_error());
		}

		void WriteGuid(const Omega::string_t&, const Omega::guid_t& val)
		{
			WriteUInt32(Omega::string_t(),val.Data1);
			WriteUInt16(Omega::string_t(),val.Data2);
			WriteUInt16(Omega::string_t(),val.Data3);
			WriteBytes(Omega::string_t(),8,val.Data4);
		}

		Omega::byte_t ReadByte(const Omega::string_t&)
		{
			Omega::byte_t retval;
			if (!m_stream.read(retval))
				OMEGA_THROW(m_stream.last_error());
			return retval;
		}

		void ReadBytes(const Omega::string_t&, Omega::uint32_t count, Omega::byte_t* val)
		{
			Omega::uint32_t actual = ReadUInt32(Omega::string_t());
			if (actual > count)
				OMEGA_THROW(L"Over-read on memory message");

			size_t read = m_stream.read_bytes(val,actual);
			if (read != actual)
				OMEGA_THROW(L"Under-read on memory message");
		}

		void ReadStrings(const Omega::string_t&, Omega::uint32_t count, Omega::string_t* arr)
		{
			Omega::uint32_t actual = ReadUInt32(Omega::string_t());
			if (actual > count)
				OMEGA_THROW(L"Over-read on memory message");

			for (Omega::uint32_t i=0;i<actual;++i)
				arr[i] = ReadString(Omega::string_t());
		}

		void ReadGuids(const Omega::string_t&, Omega::uint32_t count, Omega::guid_t* arr)
		{
			Omega::uint32_t actual = ReadUInt32(Omega::string_t());
			if (actual > count)
				OMEGA_THROW(L"Over-read on memory message");

			for (Omega::uint32_t i=0;i<actual;++i)
				arr[i] = ReadGuid(Omega::string_t());
		}

		void ReadStructStart(const Omega::string_t&, const Omega::string_t&)
			{ /* NOP */	}

		void ReadStructEnd(const Omega::string_t&)
			{ /* NOP */	}

		void WriteByte(const Omega::string_t&, Omega::byte_t val)
		{
			if (!m_stream.write(val))
				OMEGA_THROW(m_stream.last_error());
		}

		void WriteBytes(const Omega::string_t&, Omega::uint32_t count, const Omega::byte_t* val)
		{
			WriteUInt32(Omega::string_t(),count);
			m_stream.write_bytes(val,count);
		}

		void WriteStrings(const Omega::string_t&, Omega::uint32_t count, const Omega::string_t* arr)
		{
			WriteUInt32(Omega::string_t(),count);
			for (Omega::uint32_t i=0;i<count;++i)
				WriteString(Omega::string_t(),arr[i]);
		}

		void WriteGuids(const Omega::string_t&, Omega::uint32_t count, const Omega::guid_t* arr)
		{
			WriteUInt32(Omega::string_t(),count);
			for (Omega::uint32_t i=0;i<count;++i)
				WriteGuid(Omega::string_t(),arr[i]);
		}

		void WriteStructStart(const Omega::string_t&, const Omega::string_t&)
			{ /* NOP */	}

		void WriteStructEnd(const Omega::string_t&)
			{ /* NOP */	}
	};
}

#endif // OOCORE_CDR_MESSAGE_H_INCLUDED_
