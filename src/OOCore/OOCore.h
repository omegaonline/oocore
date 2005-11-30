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
		
		DECLARE_IID(OOCore_Export);
	};

	class OutputStream : public OOObject::Object
	{
	public:
		virtual int Append(OutputStream* add) = 0;

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
				
		DECLARE_IID(OOCore_Export);
	};

	class Stub : public OOObject::Object
	{
	public:
		virtual int Invoke(unsigned int method, OOObject::int32_t& ret_code, InputStream* input, OutputStream* output) = 0;

		DECLARE_IID(OOCore_Export);
	};
	
	class Transport : public OOObject::Object
	{
	public:
		virtual int CreateOutputStream(OutputStream** ppStream) = 0;
		virtual int Send(OutputStream* output) = 0;

		DECLARE_IID(OOCore_Export);
	};

	class ProxyStubManager : public OOObject::Object
	{
	public:
		virtual int CreateProxy(const OOObject::guid_t& iid, const OOObject::cookie_t& key, OOObject::Object** ppVal) = 0;
		virtual int CreateStub(const OOObject::guid_t& iid, OOObject::Object* pObj, OutputStream* output) = 0;
		virtual int CreateRequest(const OOObject::cookie_t& proxy_key, OOObject::uint32_t method, OOObject::bool_t sync, OOObject::uint32_t* trans_id, OutputStream** output) = 0;
		virtual int CancelRequest(OOObject::uint32_t trans_id) = 0;
		virtual int SendAndReceive(OutputStream* output, OOObject::uint32_t trans_id, InputStream** input) = 0;
		
		DECLARE_IID(OOCore_Export);
	};

	class RemoteObjectFactory : public OOObject::Object
	{
	public:
		virtual OOObject::int32_t CreateObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal) = 0;
		virtual OOObject::int32_t SetReverse(RemoteObjectFactory* pRemote) = 0;

		DECLARE_IID(OOCore_Export);
	};

	class Constructor : public OOObject::Object
	{
	public:
		virtual int GetTypeInfo() = 0;
		virtual int Create(const OOObject::guid_t& iid, OOObject::Object** ppVal, OOCore::InputStream* in, OOCore::OutputStream* out) = 0;

		DECLARE_IID(OOCore_Export);
	};

	class Library : public OOObject::Object
	{
	public:
		virtual int CreateProxy(ProxyStubManager* manager, const OOObject::guid_t& iid, const OOObject::cookie_t& key, OOObject::Object** proxy) = 0;
		virtual int CreateStub(ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, Stub** stub) = 0;
		virtual int GetObjectConstructor(const OOObject::guid_t& clsid, Constructor** ppConstructor) = 0;

		DECLARE_IID(OOCore_Export);
	};

	typedef OOCore_Export int (*CreateProxy_Function)(ProxyStubManager* manager, const OOObject::guid_t& iid, const OOObject::cookie_t& key, OOObject::Object** proxy);
	typedef OOCore_Export int (*CreateStub_Function)(ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, Stub** stub);

	OOCore_Export int InitAsServer();
};

#endif // OOCORE_OOCORE_H_INCLUDED_
