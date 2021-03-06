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
	extern const Omega::guid_t OID_CDRMessageMarshalFactory;
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

		virtual ~CDRMessage()
		{
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

		void write(const Omega::string_t& strText)
		{
			if (!m_stream.write(strText.c_str(),strText.Length()))
				OMEGA_THROW(m_stream.last_error());
		}

	public:
		OOBase::CDRStream* GetCDRStream()
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
			pMessage->WriteValue(Omega::string_t::constant("length"),len);
			pMessage->WriteBytes(Omega::string_t::constant("data"),len,reinterpret_cast<const Omega::byte_t*>(m_stream.buffer()->rd_ptr()));
		}

		void ReleaseMarshalData(Omega::Remoting::IMarshaller*, Omega::Remoting::IMessage* pMessage, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			Omega::uint32_t len = pMessage->ReadValue(Omega::string_t::constant("length")).cast<Omega::uint32_t>();
			if (len <= 256)
			{
				Omega::byte_t szBuf[256];
				pMessage->ReadBytes(Omega::string_t::constant("data"),len,szBuf);
			}
			else
			{
				Omega::byte_t* szBuf = static_cast<Omega::byte_t*>(::Omega::System::Allocate(len));
				try
				{
					pMessage->ReadBytes(Omega::string_t::constant("data"),len,szBuf);
				}
				catch (...)
				{
					::Omega::System::Free(szBuf);
					throw;
				}
				::Omega::System::Free(szBuf);
			}
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
				{
					OOBase::StackAllocator<256> allocator;
					OOBase::LocalString str(allocator);
					if (!m_stream.read_string(str))
						OMEGA_THROW(m_stream.last_error());

					return Omega::any_t(str.c_str());
				}

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
			default:
				return Omega::any_t();
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
				return write(value.cast<Omega::int64_t>());
			case Omega::TypeInfo::typeUInt64:
				return write(value.cast<Omega::uint64_t>());
			case Omega::TypeInfo::typeFloat4:
				return write(value.cast<Omega::float4_t>());
			case Omega::TypeInfo::typeFloat8:
				return write(value.cast<Omega::float8_t>());
			case Omega::TypeInfo::typeString:
				return write(value.cast<Omega::string_t>());

			case Omega::TypeInfo::typeGuid:
				{
					Omega::guid_t g = value.cast<Omega::guid_t>();
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
			default:
				break;
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
