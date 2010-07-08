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

#include "../../include/Omega/Remoting.h"

namespace OOCore
{
	// {1455FCD0-A49B-4f2a-94A5-222949957123}
	extern "C" const Omega::guid_t OID_CDRMessageMarshalFactory;
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

		template <typename T>
		T read()
		{
			T retval;
			if (!m_stream.read(retval))
				OMEGA_THROW(m_stream.last_error());
			return retval;
		}

		template <typename T>
		void write(T val)
		{
			if (!m_stream.write(val))
				OMEGA_THROW(m_stream.last_error());
		}

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
				OMEGA_THROW("Message too long to marshal");

			Omega::uint32_t len = static_cast<Omega::uint32_t>(m_stream.buffer()->length());
			pMessage->WriteValue(L"length",len);
			pMessage->WriteBytes(L"data",len,reinterpret_cast<const Omega::byte_t*>(m_stream.buffer()->rd_ptr()));
		}

		void ReleaseMarshalData(Omega::Remoting::IMarshaller*, Omega::Remoting::IMessage* pMessage, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			Omega::uint32_t len = pMessage->ReadValue(L"length").cast<Omega::uint32_t>();
			OOBase::SmartPtr<Omega::byte_t,OOBase::ArrayDestructor<Omega::byte_t> > szBuf = 0;
			OMEGA_NEW(szBuf,Omega::byte_t[len]);
			pMessage->ReadBytes(L"data",len,szBuf);
		}

	// IMessage members
	public:
		void ReadBytes(const Omega::string_t&, Omega::uint32_t count, Omega::byte_t* val)
		{
			Omega::uint32_t actual = read<Omega::uint32_t>();
			if (actual > count)
				OMEGA_THROW("Over-read on memory message");

			size_t read = m_stream.read_bytes(val,actual);
			if (read != actual)
				OMEGA_THROW("Under-read on memory message");
		}

		void WriteBytes(const Omega::string_t&, Omega::uint32_t count, const Omega::byte_t* val)
		{
			if (!m_stream.write(count) ||
					!m_stream.write_bytes(val,count))
			{
				OMEGA_THROW(m_stream.last_error());
			}
		}

		Omega::any_t ReadValue(const Omega::string_t&)
		{
			Omega::TypeInfo::Type_t type = read<Omega::TypeInfo::Type_t>();
			switch (static_cast<Omega::TypeInfo::Type>(type))
			{
			case Omega::TypeInfo::typeBool:
				return read<Omega::bool_t>();
			case Omega::TypeInfo::typeByte:
				return read<Omega::byte_t>();
			case Omega::TypeInfo::typeInt16:
				return read<Omega::int16_t>();
			case Omega::TypeInfo::typeUInt16:
				return read<Omega::uint16_t>();
			case Omega::TypeInfo::typeInt32:
				return read<Omega::int32_t>();
			case Omega::TypeInfo::typeUInt32:
				return read<Omega::uint32_t>();
			case Omega::TypeInfo::typeInt64:
				return read<Omega::int64_t>();
			case Omega::TypeInfo::typeUInt64:
				return read<Omega::uint64_t>();
			case Omega::TypeInfo::typeFloat4:
				return read<Omega::float4_t>();
			case Omega::TypeInfo::typeFloat8:
				return read<Omega::float8_t>();
			case Omega::TypeInfo::typeString:
				return Omega::string_t(read<std::string>().c_str(),true);

			case Omega::TypeInfo::typeGuid:
				{
					Omega::byte_t is_null = read<Omega::byte_t>();
					if (is_null == 0)
						return Omega::guid_t::Null();
					else
					{
						Omega::guid_t g;
						g.Data1 = read<Omega::uint32_t>();
						g.Data2 = read<Omega::int16_t>();
						g.Data3 = read<Omega::int16_t>();
						if (m_stream.read_bytes(g.Data4,sizeof(g.Data4)) != sizeof(g.Data4))
							OMEGA_THROW(m_stream.last_error());
						return g;
					}
				}

			case Omega::TypeInfo::typeVoid:
				return Omega::any_t();

			default:
				OMEGA_THROW("Invalid any_t type");
			}
		}

		void WriteValue(const Omega::string_t&, const Omega::any_t& value)
		{
			Omega::TypeInfo::Type_t type = static_cast<Omega::TypeInfo::Type_t>(value.GetType());
			write(type);

			switch (value.GetType())
			{
			case Omega::TypeInfo::typeBool:
				return write(value.cast<Omega::bool_t>());
			case Omega::TypeInfo::typeByte:
				return write(value.cast<Omega::byte_t>());
			case Omega::TypeInfo::typeInt16:
				return write(value.cast<Omega::int16_t>());
			case Omega::TypeInfo::typeUInt16:
				return write(value.cast<Omega::uint16_t>());
			case Omega::TypeInfo::typeInt32:
				return write(value.cast<Omega::int32_t>());
			case Omega::TypeInfo::typeUInt32:
				return write(value.cast<Omega::uint32_t>());
			case Omega::TypeInfo::typeInt64:
				return write(value.cast<const Omega::int64_t&>());
			case Omega::TypeInfo::typeUInt64:
				return write(value.cast<const Omega::uint64_t&>());
			case Omega::TypeInfo::typeFloat4:
				return write(value.cast<Omega::float4_t>());
			case Omega::TypeInfo::typeFloat8:
				return write(value.cast<const Omega::float8_t&>());
			case Omega::TypeInfo::typeString:
				return write(value.cast<const Omega::string_t&>().ToUTF8());

			case Omega::TypeInfo::typeGuid:
				{
					const Omega::guid_t& g = value.cast<const Omega::guid_t&>();
					if (g == Omega::guid_t::Null())
						write(Omega::byte_t(0));
					else
					{
						write(Omega::byte_t(1));

						write(g.Data1);
						write(g.Data2);
						write(g.Data3);
						if (!m_stream.write_bytes(g.Data4,sizeof(g.Data4)))
							OMEGA_THROW(m_stream.last_error());
					}
				}
				break;

			case Omega::TypeInfo::typeVoid:
				return;

			default:
				OMEGA_THROW("Invalid any_t type");
			}
		}

		void ReadStructStart(const Omega::string_t&, const Omega::string_t&)
		{ /* NOP */ }

		void ReadStructEnd()
		{ /* NOP */ }

		void WriteStructStart(const Omega::string_t&, const Omega::string_t&)
		{ /* NOP */ }

		void WriteStructEnd()
		{ /* NOP */ }

		Omega::uint32_t ReadArrayStart(const Omega::string_t&)
		{
			return read<Omega::uint32_t>();
		}

		void ReadArrayEnd()
		{ /* NOP */ }

		void WriteArrayStart(const Omega::string_t&, Omega::uint32_t count)
		{
			write(count);
		}

		void WriteArrayEnd()
		{ /* NOP */ }
	};
}

#endif // OOCORE_CDR_MESSAGE_H_INCLUDED_
