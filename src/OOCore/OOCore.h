#ifndef OOCORE_OOCORE_H_INCLUDED_
#define OOCORE_OOCORE_H_INCLUDED_

#include <ace/Active_Map_Manager.h>

#include "./Object.h"

#ifdef _DEBUG
#define DEFAULT_WAIT	30
#else
#define DEFAULT_WAIT	5
#endif

namespace OOCore
{
	class InputStream : public OOObject::Object
	{
	public:
		virtual int ReadBoolean(OOObject::bool_t& val) = 0;
		virtual int ReadChar(OOObject::char_t& val) = 0;
		virtual int ReadByte(OOObject::byte_t& val) = 0;
		virtual int ReadShort(OOObject::int16_t& val) = 0;
		virtual int ReadUShort(OOObject::uint16_t& val) = 0;
		virtual int ReadLong(OOObject::int32_t& val) = 0;
		virtual int ReadULong(OOObject::uint32_t& val) = 0;
		virtual int ReadLongLong(OOObject::int64_t& val) = 0;
		virtual int ReadULongLong(OOObject::uint64_t& val) = 0;
		virtual int ReadFloat(OOObject::real4_t& val) = 0;
		virtual int ReadDouble(OOObject::real8_t& val) = 0;
		
		DECLARE_IID(OOCore);
	};

	class OutputStream : public OOObject::Object
	{
	public:
		virtual int WriteBoolean(OOObject::bool_t val) = 0;
		virtual int WriteChar(OOObject::char_t val) = 0;
		virtual int WriteByte(OOObject::byte_t val) = 0;
		virtual int WriteShort(OOObject::int16_t val) = 0;
		virtual int WriteUShort(OOObject::uint16_t val) = 0;
		virtual int WriteLong(OOObject::int32_t val) = 0;
		virtual int WriteULong(OOObject::uint32_t val) = 0;
		virtual int WriteLongLong(OOObject::int64_t val) = 0;
		virtual int WriteULongLong(OOObject::uint64_t val) = 0;
		virtual int WriteFloat(OOObject::real4_t val) = 0;
		virtual int WriteDouble(OOObject::real8_t val) = 0;
				
		DECLARE_IID(OOCore);
	};

	class Channel : public OOObject::Object
	{
	public:
		virtual int CreateOutputStream(OutputStream** ppStream) = 0;
		virtual int Send(OutputStream* output) = 0;
		
		DECLARE_IID(OOCore);
	};

	class Transport : public OOObject::Object
	{
	public:
		virtual OOObject::int32_t CreateObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal) = 0;

		DECLARE_IID(OOCore);
	};

	class TypeInfo : public OOObject::Object
	{
	public:
		enum Type
		{
			// Basic types
			bool_t = 0,
			char_t = 1,
			byte_t = 2,
			int16_t = 3,
			uint16_t = 4,
			int32_t = 5,
			uint32_t = 6,
			int64_t = 7,
			uint64_t = 8,
			real4_t = 9,
			real8_t = 0xA,
			guid_t = 0xB,
			Object = 0xC,
			TYPE_MASK = 0xF,

			// Attribute modifiers
			in = 0x100,
			out = 0x200,
			in_out = 0x300,
			array = 0x400,
			string = 0x800,
			ATTR_MASK = 0xF00,

			// C++ modifiers (not hugely useful)
			cpp_const = 0x1000,
			cpp_ref = 0x2000,
			CPP_MASK = 0x3000
		};
		typedef OOObject::uint16_t Type_t;

		enum Method_Attributes
		{
			sync_method = 0,
			async_method = 1
		};
		typedef OOObject::uint16_t Method_Attributes_t;

		virtual int GetMetaInfo(const OOObject::char_t** type_name, size_t* method_count) = 0;
		virtual int GetMethodInfo(size_t method, const OOObject::char_t** method_name, size_t* param_count, Method_Attributes_t* attributes, OOObject::uint16_t* wait_secs) = 0;
		virtual int GetParamInfo(size_t method, size_t param, const OOObject::char_t** param_name, Type_t* type) = 0;

		struct Param_Attrib_Data_t
		{
			bool	is_index;
			union
			{
				size_t	index;
				const OOObject::guid_t* iid;
			};
		};
		virtual int GetParamAttributeData(size_t method, size_t param, Param_Attrib_Data_t* data) = 0;

		DECLARE_IID(OOCore);
	};

	class Stub : public OOObject::Object
	{
	public:
		virtual int Invoke(TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, InputStream* input, OutputStream* output) = 0;
		virtual int GetObject(OOObject::Object** ppVal) = 0;

		DECLARE_IID(OOCore);
	};

	class ProxyStubManager : public OOObject::Object
	{
	public:
		typedef ACE_Active_Map_Manager_Key	cookie_t;

		virtual int CreateProxy(const OOObject::guid_t& iid, const OOCore::ProxyStubManager::cookie_t& key, OOObject::Object** ppVal) = 0;
		virtual int CreateStub(const OOObject::guid_t& iid, OOObject::Object* pObj, OOCore::ProxyStubManager::cookie_t* key) = 0;
		virtual int ReleaseProxy(const OOCore::ProxyStubManager::cookie_t& key) = 0;
		virtual int ReleaseStub(const OOCore::ProxyStubManager::cookie_t& key) = 0;
		virtual int CreateRequest(TypeInfo::Method_Attributes_t flags, const OOCore::ProxyStubManager::cookie_t& proxy_key, OOObject::uint32_t* trans_id, OutputStream** output) = 0;
		virtual int CancelRequest(OOObject::uint32_t trans_id) = 0;
		virtual int SendAndReceive(TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, OutputStream* output, OOObject::uint32_t trans_id, InputStream** input) = 0;
		
		DECLARE_IID(OOCore);
	};

	class Proxy : public OOObject::Object
	{
	public:
		virtual int GetManager(ProxyStubManager** ppManager) = 0;
		virtual int GetKey(OOCore::ProxyStubManager::cookie_t* proxy_key) = 0;

		DECLARE_IID(OOCore);
	};

	class ObjectFactory : public OOObject::Object
	{
	public:
		enum Flags
		{
			LOCAL_ONLY = 1,
			REMOTE_ONLY = 2,
			USAGE_ANY = 3
		};
		typedef OOObject::uint16_t Flags_t;

		virtual OOObject::int32_t CreateObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal) = 0;

		DECLARE_IID(OOCore);
	};

	class Protocol : public OOObject::Object
	{
	public:
		virtual OOObject::int32_t Connect(const OOObject::char_t* remote_addr, Transport** ppTransport) = 0;

		DECLARE_IID(OOCore);
	};

	typedef int (*CreateProxy_Function)(ProxyStubManager* manager, const OOObject::guid_t& iid, const OOCore::ProxyStubManager::cookie_t& key, OOObject::Object** proxy);
	typedef int (*CreateStub_Function)(ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOCore::ProxyStubManager::cookie_t& key, Stub** stub);
	typedef int (*RegisterLib_Function)(bool bRegister);
	typedef int (*GetTypeInfo_Function)(const OOObject::guid_t& iid, TypeInfo** typeinfo);

	OOCore_Export int RegisterProxyStub(const OOObject::guid_t& iid, const char* dll_name);
	OOCore_Export int UnregisterProxyStub(const OOObject::guid_t& iid, const char* dll_name);
    OOCore_Export OOObject::int32_t AddObjectFactory(ObjectFactory::Flags_t flags, const OOObject::guid_t& clsid, ObjectFactory* pFactory);
	OOCore_Export OOObject::int32_t RemoveObjectFactory(const OOObject::guid_t& clsid);
	OOCore_Export OOObject::int32_t RegisterProtocol(const OOObject::char_t* name, OOCore::Protocol* protocol);
	OOCore_Export OOObject::int32_t UnregisterProtocol(const OOObject::char_t* name);
	OOCore_Export int CreateProxy(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, const OOCore::ProxyStubManager::cookie_t& key, OOObject::Object** proxy);
	OOCore_Export int CreateStub(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOCore::ProxyStubManager::cookie_t& key, OOCore::Stub** stub);
	OOCore_Export int GetTypeInfo(const OOObject::guid_t& iid, OOCore::TypeInfo** type_info);
};

#endif // OOCORE_OOCORE_H_INCLUDED_
