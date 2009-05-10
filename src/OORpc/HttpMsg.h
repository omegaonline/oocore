///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OORpc, the Omega Online RPC library.
//
// OORpc is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OORpc is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OORpc.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OORPC_HTTP_MESSAGE_H_INCLUDED_
#define OORPC_HTTP_MESSAGE_H_INCLUDED_

namespace Rpc
{
	interface IHttpMsg : public Omega::IObject
	{
		virtual void WriteSubObject(const char* sz, size_t len) = 0;
		virtual void ReadSubObject(void* str) = 0;
		virtual void GetContent(void* str) = 0;
	};
}

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Rpc, IHttpMsg, "{65285EA5-0433-4088-AA07-A8489F611332}",

	// Methods
	OMEGA_METHOD_VOID(WriteSubObject,2,((in),const char*,sz,(in),size_t,len))
	OMEGA_METHOD_VOID(ReadSubObject,1,((in),void*,str))
	OMEGA_METHOD_VOID(GetContent,1,((in),void*,str))
)

namespace Rpc
{
	// {B264BBE2-2DFF-45be-ADB4-1A80E6576A63}
	OMEGA_EXPORT_OID(OID_HttpOutputMsgMarshalFactory);

	// {C94CCB71-6345-40b1-B588-8EE91089B7AD}
	OMEGA_EXPORT_OID(OID_HttpOutputMsg);

	class HttpMsgBase :
		public Omega::Remoting::IMessage,
		public Omega::Remoting::IMarshal,
		public IHttpMsg
	{
	public:
		HttpMsgBase();
		virtual ~HttpMsgBase();

	protected:
		OOBase::Buffer* m_mb;
		bool               m_bFirstItem;

		bool IsWhitespace(char c);
		void CheckChar(char c);
		char PeekNextChar();
		void SkipWhitespace();
		Omega::string_t ReadString();
		Omega::uint64_t ReadUInt();
		Omega::int64_t ReadInt();
		Omega::float8_t ReadDouble();
		void ParseName(const wchar_t* pszName);
		void SkipValue(bool bNamed);
		void SkipCont(char term, bool bNamed);
			
	// IMarshal members
	public: 
		Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			return OID_HttpOutputMsgMarshalFactory;
		}

		void MarshalInterface(Omega::Remoting::IObjectManager*, Omega::Remoting::IMessage* pMessage, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t);
		void ReleaseMarshalData(Omega::Remoting::IObjectManager*, Omega::Remoting::IMessage* pMessage, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t);

	// Some IHttpMsg members
	public:
		void ReadSubObject(void* str);
		void GetContent(void* str);
		
	// Some IMessage members
	public:
		size_t ReadBooleans(const wchar_t* pszName, size_t count, Omega::bool_t* arr);
		size_t ReadBytes(const wchar_t* pszName, size_t count, Omega::byte_t* arr);
		size_t ReadInt16s(const wchar_t* pszName, size_t count, Omega::int16_t* arr);
		size_t ReadUInt16s(const wchar_t* pszName, size_t count, Omega::uint16_t* arr);
		size_t ReadInt32s(const wchar_t* pszName, size_t count, Omega::int32_t* arr);
		size_t ReadUInt32s(const wchar_t* pszName, size_t count, Omega::uint32_t* arr);
		size_t ReadInt64s(const wchar_t* pszName, size_t count, Omega::int64_t* arr);
		size_t ReadUInt64s(const wchar_t* pszName, size_t count, Omega::uint64_t* arr);
		size_t ReadFloat4s(const wchar_t* pszName, size_t count, Omega::float4_t* arr);
		size_t ReadFloat8s(const wchar_t* pszName, size_t count, Omega::float8_t* arr);
		size_t ReadStrings(const wchar_t* pszName, size_t count, Omega::string_t* arr);
		size_t ReadGuids(const wchar_t* pszName, size_t count, Omega::guid_t* arr);
		void ReadStructStart(const wchar_t* pszName, const wchar_t* pszType);
		void ReadStructEnd(const wchar_t* pszName);
	};

	class HttpOutputMsg :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactory<HttpOutputMsg,&OID_HttpOutputMsg,0,Omega::Activation::InProcess>,
		public HttpMsgBase
	{
	public:
		BEGIN_INTERFACE_MAP(HttpOutputMsg)
			INTERFACE_ENTRY(Omega::Remoting::IMessage)
			INTERFACE_ENTRY(Omega::Remoting::IMarshal)
			INTERFACE_ENTRY(IHttpMsg)
		END_INTERFACE_MAP()

	private:
		void grow_mb(size_t cbBytes);
		void WriteString(std::string str);
		void WriteName(const Omega::string_t& strName);
		void WriteInt(const Omega::int64_t& v);
		void WriteUInt(const Omega::uint64_t& v);
		void WriteDouble(const double& v);

	// IHttpMsg
	public:
		void WriteSubObject(const char* sz, size_t len);		

	// IMessage members
	public:
		void WriteBooleans(const wchar_t* pszName, size_t count, const Omega::bool_t* arr);
		void WriteBytes(const wchar_t* pszName, size_t count, const Omega::byte_t* arr);
		void WriteInt16s(const wchar_t* pszName, size_t count, const Omega::int16_t* arr);
		void WriteUInt16s(const wchar_t* pszName, size_t count, const Omega::uint16_t* arr);
		void WriteInt32s(const wchar_t* pszName, size_t count, const Omega::int32_t* arr);
		void WriteUInt32s(const wchar_t* pszName, size_t count, const Omega::uint32_t* arr);
		void WriteInt64s(const wchar_t* pszName, size_t count, const Omega::int64_t* arr);
		void WriteUInt64s(const wchar_t* pszName, size_t count, const Omega::uint64_t* arr);
		void WriteFloat4s(const wchar_t* pszName, size_t count, const Omega::float4_t* arr);
		void WriteFloat8s(const wchar_t* pszName, size_t count, const Omega::float8_t* arr);
		void WriteStrings(const wchar_t* pszName, size_t count, const Omega::string_t* arr);
		void WriteGuids(const wchar_t* pszName, size_t count, const Omega::guid_t* arr);
		void WriteStructStart(const wchar_t* pszName, const wchar_t* pszType);
		void WriteStructEnd(const wchar_t* pszName);		
	};

	class HttpInputMsg :
		public OTL::ObjectBase,
		public HttpMsgBase
	{
	public:
		void init(const ACE_Message_Block* mb);
		void init(const std::string& strText);

		void skip_leader();
		bool more_exists();
		
		BEGIN_INTERFACE_MAP(HttpInputMsg)
			INTERFACE_ENTRY(Omega::Remoting::IMessage)
			INTERFACE_ENTRY(Omega::Remoting::IMarshal)
			INTERFACE_ENTRY(IHttpMsg)
		END_INTERFACE_MAP()

	// IHttpMsg
	public:
		void WriteSubObject(const char*, size_t)
			{ OMEGA_THROW(EACCES); }

	// IMessage members
	public:
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
		void WriteFloat4s(const wchar_t*, size_t, const Omega::float4_t*)
			{ OMEGA_THROW(EACCES); }
		void WriteFloat8s(const wchar_t*, size_t, const Omega::float8_t*)
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

	class HttpOutputMsgMarshalFactory :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactorySingleton<HttpOutputMsgMarshalFactory,&OID_HttpOutputMsgMarshalFactory,0,Omega::Activation::InProcess>,
		public Omega::Remoting::IMarshalFactory
	{
	public:
		BEGIN_INTERFACE_MAP(HttpOutputMsgMarshalFactory)
			INTERFACE_ENTRY(Omega::Remoting::IMarshalFactory)
		END_INTERFACE_MAP()

	// IMarshalFactory members
	public:
		void UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags, Omega::IObject*& pObject);
	};
}

#endif // OORPC_HTTP_MESSAGE_H_INCLUDED_
