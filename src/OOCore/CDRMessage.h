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
	interface IMessageBlockHolder : public Omega::IObject
	{
		virtual const void* GetMessageBlock() = 0;
		virtual void* GetOutputCDR() = 0;
		virtual void* GetInputCDR() = 0;
	};

	// {1455FCD0-A49B-4f2a-94A5-222949957123}
	OMEGA_DECLARE_OID(OID_OutputCDRMarshalFactory);
}

OMEGA_DEFINE_INTERFACE_LOCAL
(
	OOCore, IMessageBlockHolder, "{5251283B-95C8-4e5b-9136-5DDCBE636A4E}",

	// Methods
	OMEGA_METHOD(const void*,GetMessageBlock,0,())
	OMEGA_METHOD(void*,GetOutputCDR,0,())
	OMEGA_METHOD(void*,GetInputCDR,0,())
)

// Some macros to help

#define OOCORE_DEFINE_MESSAGE_READ(input,name,type,ace_type) \
	size_t name(const wchar_t*,size_t count, Omega::type* arr) \
	{ \
		if (count == 1) \
		{ \
			if (!input OMEGA_CONCAT(read_,ace_type)(arr[0])) \
				OMEGA_THROW(ACE_OS::last_error()); \
		} \
		else \
		{ \
			if (count > (size_t)-1 / sizeof(Omega::type)) \
				OMEGA_THROW(E2BIG); \
			if (count > (ACE_CDR::ULong)-1) \
				OMEGA_THROW(E2BIG); \
			if (!input OMEGA_CONCAT(OMEGA_CONCAT_R(read_,ace_type),_array)(arr,static_cast<ACE_CDR::ULong>(count))) \
				OMEGA_THROW(ACE_OS::last_error()); \
		} \
		return count; \
	}

#define OOCORE_DEFINE_MESSAGE_WRITE(name,type,ace_type) \
	void name(const wchar_t*,size_t count, const Omega::type* arr) \
	{ \
		if (count == 1) \
		{ \
			if (!OMEGA_CONCAT(write_,ace_type)(arr[0])) \
				OMEGA_THROW(ACE_OS::last_error()); \
		} \
		else \
		{ \
			if (count > (ACE_CDR::ULong)-1) \
				OMEGA_THROW(E2BIG); \
			if (!OMEGA_CONCAT(OMEGA_CONCAT_R(write_,ace_type),_array)(arr,static_cast<ACE_CDR::ULong>(count))) \
				OMEGA_THROW(ACE_OS::last_error()); \
		} \
	}

namespace OOCore
{
	class OutputCDR :
		public OTL::ObjectBase,
		public ACE_OutputCDR,
		public OOCore::IMessageBlockHolder,
		public Omega::Remoting::IMessage,
		public Omega::Remoting::IMarshal
	{
	public:
		OutputCDR() : m_pInput(0)
		{ }

		virtual ~OutputCDR()
		{
			if (m_pInput)
				delete m_pInput;
		}

		BEGIN_INTERFACE_MAP(OutputCDR)
			INTERFACE_ENTRY(Omega::Remoting::IMessage)
			INTERFACE_ENTRY(Omega::Remoting::IMarshal)
			INTERFACE_ENTRY(OOCore::IMessageBlockHolder)
		END_INTERFACE_MAP()

	private:
		ACE_InputCDR* m_pInput;

		ACE_InputCDR& get_input()
		{
			if (!m_pInput)
				OMEGA_NEW(m_pInput,ACE_InputCDR(*this));

			return *m_pInput;
		}

		Omega::uint16_t ReadUInt16()
		{ 
			Omega::uint16_t val; 
			if (!get_input().read_ushort(val)) 
				OMEGA_THROW(ACE_OS::last_error()); 
			return val; 
		}

		Omega::uint32_t ReadUInt32()
		{ 
			Omega::uint32_t val; 
			if (!get_input().read_ulong(val)) 
				OMEGA_THROW(ACE_OS::last_error()); 
			return val;
		}

		Omega::string_t ReadString()
		{ 
			ACE_CString val; 
			if (!get_input().read_string(val)) 
				OMEGA_THROW(ACE_OS::last_error()); 

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
				OMEGA_THROW(EIO);
			return g;
		}

		void WriteUInt16(Omega::uint16_t val)
		{ 
			if (!write_ushort(val)) 
				OMEGA_THROW(ACE_OS::last_error()); 
		}
		
		void WriteUInt32(Omega::uint32_t val)
		{ 
			if (!write_ulong(val)) 
				OMEGA_THROW(ACE_OS::last_error()); 
		}

		void WriteString(const Omega::string_t& val)
		{ 
			if (!write_string(val.ToUTF8().c_str())) 
				OMEGA_THROW(ACE_OS::last_error()); 
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
			ACE_CDR::ULong cb = static_cast<ACE_CDR::ULong>(cbBytes);
			if (!get_input().read_octet_array(val,cb)) 
				OMEGA_THROW(ACE_OS::last_error());
		}

		void WriteBytes(const Omega::uint64_t& cbBytes, const Omega::byte_t* val)
		{ 
			ACE_CDR::ULong cb = static_cast<ACE_CDR::ULong>(cbBytes);
			if (!write_octet_array(val,cb)) 
				OMEGA_THROW(ACE_OS::last_error());
		}

	// IMessageBlockHolder
	public:
		const void* GetMessageBlock()
		{
			return begin();
		}

		void* GetOutputCDR()
		{
			return static_cast<ACE_OutputCDR*>(this);
		}

		void* GetInputCDR()
		{
			get_input();
			return m_pInput;
		}

	// IMarshal members
	public: 
		Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			return OID_OutputCDRMarshalFactory;
		}

		void MarshalInterface(Omega::Remoting::IObjectManager*, Omega::Remoting::IMessage* pMessage, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			OTL::ObjectPtr<IMessageBlockHolder> ptrMB(pMessage);
			if (ptrMB)
			{
				ACE_OutputCDR* pOutput = static_cast<ACE_OutputCDR*>(ptrMB->GetOutputCDR());

				if (total_length() > (ACE_CDR::ULong)-1)
					OMEGA_THROW(E2BIG);

				pOutput->write_ulong((ACE_CDR::ULong)total_length());
				pOutput->align_write_ptr(ACE_CDR::MAX_ALIGNMENT);

				pOutput->write_octet_array_mb(begin());
			}
			else
			{
				Omega::uint64_t sz = total_length();
				pMessage->WriteUInt64s(L"length",1,&sz);
				pMessage->WriteBytes(L"data",static_cast<size_t>(sz),(Omega::byte_t*)buffer());
			}
		}

		void ReleaseMarshalData(Omega::Remoting::IObjectManager*, Omega::Remoting::IMessage* pMessage, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			OTL::ObjectPtr<IMessageBlockHolder> ptrMB(pMessage);
			if (ptrMB)
			{
				ACE_InputCDR* pInput = static_cast<ACE_InputCDR*>(ptrMB->GetInputCDR());

				ACE_CDR::ULong len = 0;
				pInput->read_ulong(len);

				pInput->align_read_ptr(ACE_CDR::MAX_ALIGNMENT);
				pInput->skip_bytes((ACE_CDR::ULong)len);
			}
			else
			{
				Omega::uint64_t sz;
				pMessage->ReadUInt64s(L"length",1,&sz);
				Omega::byte_t* szBuf;
				OMEGA_NEW(szBuf,Omega::byte_t[static_cast<size_t>(sz)]);
				pMessage->ReadBytes(L"data",static_cast<size_t>(sz),szBuf);
				delete [] szBuf;
			}
		}

	// IMessage members
	public:
		OOCORE_DEFINE_MESSAGE_READ(get_input().,ReadBooleans,bool_t,boolean)
		OOCORE_DEFINE_MESSAGE_READ(get_input().,ReadBytes,byte_t,octet)
		OOCORE_DEFINE_MESSAGE_READ(get_input().,ReadInt16s,int16_t,short)
		OOCORE_DEFINE_MESSAGE_READ(get_input().,ReadUInt16s,uint16_t,ushort)
		OOCORE_DEFINE_MESSAGE_READ(get_input().,ReadInt32s,int32_t,long)
		OOCORE_DEFINE_MESSAGE_READ(get_input().,ReadUInt32s,uint32_t,ulong)
		OOCORE_DEFINE_MESSAGE_READ(get_input().,ReadInt64s,int64_t,longlong)
		OOCORE_DEFINE_MESSAGE_READ(get_input().,ReadUInt64s,uint64_t,ulonglong)
		
		size_t ReadStrings(const wchar_t*, size_t count, Omega::string_t* arr)
		{ 
			if (count > (size_t)-1 / sizeof(Omega::string_t))
				OMEGA_THROW(E2BIG);
			
			for (size_t i=0;i<count;++i)
				arr[i] = ReadString();

			return count;
		}

		size_t ReadGuids(const wchar_t*, size_t count, Omega::guid_t* arr)
		{
			if (count > (size_t)-1 / sizeof(Omega::guid_t))
				OMEGA_THROW(E2BIG);
			
			for (size_t i=0;i<count;++i)
				arr[i] = ReadGuid();

			return count;
		}

		void ReadStructStart(const wchar_t*, const wchar_t*)
			{ /* NOP */	}

		void ReadStructEnd(const wchar_t*)
			{ /* NOP */	}

		OOCORE_DEFINE_MESSAGE_WRITE(WriteBooleans,bool_t,boolean)
		OOCORE_DEFINE_MESSAGE_WRITE(WriteBytes,byte_t,octet)
		OOCORE_DEFINE_MESSAGE_WRITE(WriteInt16s,int16_t,short)
		OOCORE_DEFINE_MESSAGE_WRITE(WriteUInt16s,uint16_t,ushort)
		OOCORE_DEFINE_MESSAGE_WRITE(WriteInt32s,int32_t,long)
		OOCORE_DEFINE_MESSAGE_WRITE(WriteUInt32s,uint32_t,ulong)
		OOCORE_DEFINE_MESSAGE_WRITE(WriteInt64s,int64_t,longlong)
		OOCORE_DEFINE_MESSAGE_WRITE(WriteUInt64s,uint64_t,ulonglong)

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

	class InputCDR :
		public OTL::ObjectBase,
		public ACE_InputCDR,
		public Omega::Remoting::IMessage,
		public Omega::Remoting::IMarshal,
		public OOCore::IMessageBlockHolder
	{
	public:
		InputCDR() : ACE_InputCDR(size_t(0))
		{}

		void init(const ACE_InputCDR& i)
		{
			*static_cast<ACE_InputCDR*>(this) = i;
		}

		BEGIN_INTERFACE_MAP(InputCDR)
			INTERFACE_ENTRY(Omega::Remoting::IMessage)
			INTERFACE_ENTRY(Omega::Remoting::IMarshal)
			INTERFACE_ENTRY(OOCore::IMessageBlockHolder)
		END_INTERFACE_MAP()

	// IMarshal members
	public: 
		Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			return OID_OutputCDRMarshalFactory;
		}

		void MarshalInterface(Omega::Remoting::IObjectManager*, Omega::Remoting::IMessage* pMessage, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			OTL::ObjectPtr<IMessageBlockHolder> ptrMB(pMessage);
			if (ptrMB)
			{
				if (length() > (ACE_CDR::ULong)-1)
					OMEGA_THROW(E2BIG);

				ACE_OutputCDR* pOutput = static_cast<ACE_OutputCDR*>(ptrMB->GetOutputCDR());
				pOutput->write_ulong((ACE_CDR::ULong)length());
				pOutput->align_write_ptr(ACE_CDR::MAX_ALIGNMENT);
				pOutput->write_octet_array_mb(start());
			}
			else
			{
				Omega::uint64_t sz = this->length();
				pMessage->WriteUInt64s(L"length",1,&sz);
				pMessage->WriteBytes(L"data",static_cast<size_t>(sz),(Omega::byte_t*)(this->rd_ptr()));
			}
		}

		void ReleaseMarshalData(Omega::Remoting::IObjectManager*, Omega::Remoting::IMessage* pMessage, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			OTL::ObjectPtr<IMessageBlockHolder> ptrMB(pMessage);
			if (ptrMB)
			{
				ACE_InputCDR* pInput = static_cast<ACE_InputCDR*>(ptrMB->GetInputCDR());

				ACE_CDR::ULong len = 0;
				pInput->read_ulong(len);
				pInput->align_read_ptr(ACE_CDR::MAX_ALIGNMENT);
				pInput->skip_bytes((ACE_CDR::ULong)len);
			}
			else
			{
				Omega::uint64_t sz;
				pMessage->ReadUInt64s(L"length",1,&sz);
				Omega::byte_t* szBuf;
				OMEGA_NEW(szBuf,Omega::byte_t[static_cast<size_t>(sz)]);
				pMessage->ReadBytes(L"data",static_cast<size_t>(sz),szBuf);
				delete [] szBuf;
			}
		}

	private:
		Omega::uint16_t ReadUInt16()
		{ 
			Omega::uint16_t val; 
			if (!this->read_ushort(val)) 
				OMEGA_THROW(ACE_OS::last_error()); 
			return val; 
		}

		Omega::uint32_t ReadUInt32()
		{ 
			Omega::uint32_t val; 
			if (!this->read_ulong(val)) 
				OMEGA_THROW(ACE_OS::last_error()); 
			return val;
		}

		Omega::string_t ReadString()
		{ 
			ACE_CString val; 
			if (!read_string(val)) 
				OMEGA_THROW(ACE_OS::last_error()); 

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
				OMEGA_THROW(EIO);
			return g;
		}

		void ReadBytes(Omega::uint64_t& cbBytes, Omega::byte_t* val)
		{
			ACE_CDR::ULong cb = static_cast<ACE_CDR::ULong>(cbBytes);
			if (!read_octet_array(val,cb)) 
				OMEGA_THROW(ACE_OS::last_error());
		}

	// IMessageBlockHolder
	public:
		const void* GetMessageBlock()
			{ OMEGA_THROW(EACCES); }
		void* GetOutputCDR()
			{ OMEGA_THROW(EACCES); }

		void* GetInputCDR()
		{
			return static_cast<ACE_InputCDR*>(this);
		}

	// IMessage members
	public:
		OOCORE_DEFINE_MESSAGE_READ(this->,ReadBooleans,bool_t,boolean)
		OOCORE_DEFINE_MESSAGE_READ(this->,ReadBytes,byte_t,octet)
		OOCORE_DEFINE_MESSAGE_READ(this->,ReadInt16s,int16_t,short)
		OOCORE_DEFINE_MESSAGE_READ(this->,ReadUInt16s,uint16_t,ushort)
		OOCORE_DEFINE_MESSAGE_READ(this->,ReadInt32s,int32_t,long)
		OOCORE_DEFINE_MESSAGE_READ(this->,ReadUInt32s,uint32_t,ulong)
		OOCORE_DEFINE_MESSAGE_READ(this->,ReadInt64s,int64_t,longlong)
		OOCORE_DEFINE_MESSAGE_READ(this->,ReadUInt64s,uint64_t,ulonglong)
		
		size_t ReadStrings(const wchar_t*, size_t count, Omega::string_t* arr)
		{ 
			if (count > (size_t)-1 / sizeof(Omega::string_t))
				OMEGA_THROW(E2BIG);
			
			for (size_t i=0;i<count;++i)
				arr[i] = ReadString();

			return count;
		}

		size_t ReadGuids(const wchar_t*, size_t count, Omega::guid_t* arr)
		{
			if (count > (size_t)-1 / sizeof(Omega::guid_t))
				OMEGA_THROW(E2BIG);
			
			for (size_t i=0;i<count;++i)
				arr[i] = ReadGuid();

			return count;
		}

		void ReadStructStart(const wchar_t*, const wchar_t*)
			{ /* NOP */	}

		void ReadStructEnd(const wchar_t*)
			{ /* NOP */	}

		void WriteBooleans(const wchar_t*, size_t, const Omega::bool_t*)
			{ OMEGA_THROW(EACCES); }
		void WriteBytes(const wchar_t*, size_t, const Omega::byte_t*)
			{ OMEGA_THROW(EACCES); }
		void WriteInt16s(const wchar_t*, size_t, const Omega::int16_t*)
			{ OMEGA_THROW(EACCES); }
		void WriteUInt16s(const wchar_t*, size_t, const Omega::uint16_t*)
			{ OMEGA_THROW(EACCES); }
		void WriteInt32s(const wchar_t*, size_t, const Omega::int32_t*)
			{ OMEGA_THROW(EACCES); }
		void WriteUInt32s(const wchar_t*, size_t, const Omega::uint32_t*)
			{ OMEGA_THROW(EACCES); }
		void WriteInt64s(const wchar_t*, size_t, const Omega::int64_t*)
			{ OMEGA_THROW(EACCES); }
		void WriteUInt64s(const wchar_t*, size_t, const Omega::uint64_t*)
			{ OMEGA_THROW(EACCES); }
		void WriteStrings(const wchar_t*, size_t, const Omega::string_t*)
			{ OMEGA_THROW(EACCES); }
		void WriteGuids(const wchar_t*, size_t, const Omega::guid_t*)
			{ OMEGA_THROW(EACCES); }
		void WriteStructStart(const wchar_t*, const wchar_t*)
			{ OMEGA_THROW(EACCES); }
		void WriteStructEnd(const wchar_t*)
			{ OMEGA_THROW(EACCES); }
	};
}

#endif // OOCORE_CDR_MESSAGE_H_INCLUDED_
