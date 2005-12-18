#ifndef OOCORE_OOCORE_H_INCLUDED_
#define OOCORE_OOCORE_H_INCLUDED_

#include "./Object.h"

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

	class Transport : public OOObject::Object
	{
	public:
		virtual int CreateOutputStream(OutputStream** ppStream) = 0;
		virtual int Send(OutputStream* output) = 0;

		DECLARE_IID(OOCore);
	};

	enum Marshall_Flags
	{
		ASYNC = 0,
		SYNC = 1
	};

	class Stub : public OOObject::Object
	{
	public:
		virtual int Invoke(Marshall_Flags flags, OOObject::uint16_t wait_secs, InputStream* input, OutputStream* output) = 0;

		DECLARE_IID(OOCore);
	};

	class ProxyStubManager : public OOObject::Object
	{
	public:
		virtual int CreateProxy(const OOObject::guid_t& iid, const OOObject::cookie_t& key, OOObject::Object** ppVal) = 0;
		virtual int CreateStub(const OOObject::guid_t& iid, OOObject::Object* pObj, OOObject::cookie_t* key) = 0;
		virtual int ReleaseProxy(const OOObject::cookie_t& key) = 0;
		virtual int ReleaseStub(const OOObject::cookie_t& key) = 0;
		virtual int CreateRequest(Marshall_Flags flags, const OOObject::cookie_t& proxy_key, OOObject::uint32_t* trans_id, OutputStream** output) = 0;
		virtual int CancelRequest(OOObject::uint32_t trans_id) = 0;
		virtual int SendAndReceive(Marshall_Flags flags, OOObject::uint16_t wait_secs, OutputStream* output, OOObject::uint32_t trans_id, InputStream** input) = 0;
		
		DECLARE_IID(OOCore);
	};

	class ObjectFactory : public OOObject::Object
	{
	public:
		virtual OOObject::int32_t CreateObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal) = 0;

		DECLARE_IID(OOCore);
	};

	class Protocol : public OOObject::Object
	{
	public:
		//virtual OOObject::int32_t CreateObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal) = 0;

		DECLARE_IID(OOCore);
	};

	typedef int (*CreateProxy_Function)(ProxyStubManager* manager, const OOObject::guid_t& iid, const OOObject::cookie_t& key, OOObject::Object** proxy);
	typedef int (*CreateStub_Function)(ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::cookie_t& key, Stub** stub);
	typedef int (*RegisterLib_Function)(bool bRegister);

	extern "C"
	{
		OOCore_Export int RegisterProxyStub(const OOObject::guid_t& iid, const char* dll_name);
		OOCore_Export int UnregisterProxyStub(const OOObject::guid_t& iid, const char* dll_name);
		OOCore_Export OOObject::int32_t AddObjectFactory(const OOObject::guid_t& clsid, ObjectFactory* pFactory);
		OOCore_Export OOObject::int32_t RemoveObjectFactory(const OOObject::guid_t& clsid);
		OOCore_Export OOObject::int32_t AddProtocol(const OOObject::char_t* name, OOCore::Protocol* protocol);
		OOCore_Export OOObject::int32_t RemoveProtocol(const OOObject::char_t* name);
	}
};

#endif // OOCORE_OOCORE_H_INCLUDED_
