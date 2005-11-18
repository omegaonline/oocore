#ifndef OOCORE_OOCORE_H_INCLUDED_
#define OOCORE_OOCORE_H_INCLUDED_

#include "./Object.h"

namespace OOCore
{
	class InputStream : public OOObj::Object
	{
	public:
		virtual int ReadBoolean(OOObj::bool_t& val) = 0;
		virtual int ReadChar(OOObj::char_t& val) = 0;
		virtual int ReadByte(OOObj::byte_t& val) = 0;
		virtual int ReadShort(OOObj::int16_t& val) = 0;
		virtual int ReadUShort(OOObj::uint16_t& val) = 0;
		virtual int ReadLong(OOObj::int32_t& val) = 0;
		virtual int ReadULong(OOObj::uint32_t& val) = 0;
		virtual int ReadLongLong(OOObj::int64_t& val) = 0;
		virtual int ReadULongLong(OOObj::uint64_t& val) = 0;
		virtual int ReadFloat(OOObj::real4_t& val) = 0;
		virtual int ReadDouble(OOObj::real8_t& val) = 0;
		virtual int ReadCookie(OOObj::cookie_t& val) = 0;
		virtual int ReadGuid(OOObj::guid_t& val) = 0;
		virtual int ReadBytes(OOObj::byte_t* val, OOObj::uint32_t len) = 0;

		DECLARE_IID(OOCore_Export);
	};

	class OutputStream : public OOObj::Object
	{
	public:
		virtual int Append(OutputStream* add) = 0;

		virtual int WriteBoolean(OOObj::bool_t val) = 0;
		virtual int WriteChar(OOObj::char_t val) = 0;
		virtual int WriteByte(OOObj::byte_t val) = 0;
		virtual int WriteShort(OOObj::int16_t val) = 0;
		virtual int WriteUShort(OOObj::uint16_t val) = 0;
		virtual int WriteLong(OOObj::int32_t val) = 0;
		virtual int WriteULong(OOObj::uint32_t val) = 0;
		virtual int WriteLongLong(OOObj::int64_t val) = 0;
		virtual int WriteULongLong(OOObj::uint64_t val) = 0;
		virtual int WriteFloat(OOObj::real4_t val) = 0;
		virtual int WriteDouble(OOObj::real8_t val) = 0;
		virtual int WriteCookie(const OOObj::cookie_t& val) = 0;
		virtual int WriteGuid(const OOObj::guid_t& val) = 0;
		virtual int WriteBytes(const OOObj::byte_t* val, OOObj::uint32_t len) = 0;

		DECLARE_IID(OOCore_Export);
	};

	class Stub : public OOObj::Object
	{
	public:
		virtual int Invoke(OOObj::uint32_t method, OOObj::int32_t& ret_code, InputStream* input, OutputStream* output) = 0;

		DECLARE_IID(OOCore_Export);
	};
	
	class Transport : public OOObj::Object
	{
	public:
		virtual int CreateOutputStream(OutputStream** ppStream) = 0;
		virtual int Send(OutputStream* output) = 0;

		DECLARE_IID(OOCore_Export);
	};

	class ProxyStubManager : public OOObj::Object
	{
	public:
		virtual int CreateProxy(const OOObj::guid_t& iid, const OOObj::cookie_t& key, OOObj::Object** ppVal) = 0;
		virtual int CreateStub(const OOObj::guid_t& iid, OOObj::Object* pObj, OutputStream* output) = 0;
		virtual int CreateRequest(const OOObj::cookie_t& proxy_key, OOObj::uint32_t method, OOObj::bool_t sync, OOObj::uint32_t* trans_id, OutputStream** output) = 0;
		virtual int CancelRequest(OOObj::uint32_t trans_id) = 0;
		virtual int SendAndReceive(OutputStream* output, OOObj::uint32_t trans_id, InputStream** input) = 0;
		
		DECLARE_IID(OOCore_Export);
	};

	class RemoteObjectFactory : public OOObj::Object
	{
	public:
		virtual OOObj::int32_t CreateObject(const OOObj::string_t class_name, const OOObj::guid_t& iid, OOObj::Object** ppVal) = 0;
		virtual OOObj::int32_t SetReverse(RemoteObjectFactory* pRemote) = 0;

		DECLARE_IID(OOCore_Export);
	};

	class Server : public OOObj::Object
	{
	public:
		virtual OOObj::int32_t Stop(OOObj::bool_t force, OOObj::uint16_t* remaining) = 0;
		virtual OOObj::int32_t StopPending(OOObj::bool_t* pending) = 0;
		virtual OOObj::int32_t StayAlive() = 0;

		DECLARE_IID(OOCore_Export);
	};
	
	typedef OOCore_Export int (*CreateProxy_Function)(ProxyStubManager* manager, const OOObj::guid_t& iid, const OOObj::cookie_t& key, OOObj::Object** proxy);
	typedef OOCore_Export int (*CreateStub_Function)(ProxyStubManager* manager, const OOObj::guid_t& iid, OOObj::Object* obj, Stub** stub);

	OOCore_Export int LaunchServer();
};

#endif // OOCORE_OOCORE_H_INCLUDED_
