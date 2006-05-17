#ifndef OOCORE_OBJECT_H_INCLUDED_
#define OOCORE_OBJECT_H_INCLUDED_

#include <ace/OS.h>
#include <ace/CDR_Stream.h>

#include "./OOCore_export.h"

// In order to link properly, we avoid __declspec(selectany) because it is just not portable
// So, #include the relevant classes, having #define IID_LINK_HERE first in ONE souce file only
//
// E.g source1.c
//
// #define IID_LINK_HERE
// #include "./OOObject.h"
//
// and your project should now link!

#define HAS_IID					static const OOObject::guid_t IID;

#if (defined(IID_LINK_HERE))
#define DECLARE_IID(type, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        const OOObject::guid_t type::IID = \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#define DECLARE_OID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        const OOObject::guid_t OID_##name = \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#else
#define DECLARE_IID(type, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) 
#define DECLARE_OID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
		extern const OOObject::guid_t OID_##name
#endif

namespace OOObject
{
	typedef ACE_CDR::Boolean			bool_t;
	typedef ACE_CDR::Char				char_t;
	typedef ACE_CDR::Octet				byte_t;
    typedef ACE_CDR::Short				int16_t;
	typedef ACE_CDR::UShort				uint16_t;
	typedef ACE_CDR::Long				int32_t;
	typedef ACE_CDR::ULong				uint32_t;
	typedef ACE_CDR::LongLong			int64_t;
	typedef ACE_CDR::ULongLong			uint64_t;
	typedef ACE_CDR::Float				real4_t;
	typedef ACE_CDR::Double				real8_t;
	
	struct OOCore_Export guid_t
	{
		uint32_t	Data1;
		uint16_t	Data2;
		uint16_t	Data3;
		byte_t		Data4[8];

		bool operator==(const OOObject::guid_t& rhs) const;
		bool operator<(const OOObject::guid_t& rhs) const;

		static const guid_t NIL;
	};

	class Object
	{
	public:
		virtual int32_t AddRef() = 0;
		virtual int32_t Release() = 0;
		virtual int32_t QueryInterface(const guid_t& iid, Object** ppVal) = 0;

		HAS_IID;
	};

    // API functions
	OOCore_Export int Init(uint16_t threads = 1);
	OOCore_Export void Term();	
	OOCore_Export void* Alloc(const size_t size);
	OOCore_Export void Free(void* p);

	class ObjectFactory : public Object
	{
	public:
		enum Flags
		{
			IN_PROCESS = 1,
			OUT_OF_PROCESS = 2,
			ANY = 3,
		};
		typedef uint16_t Flags_t;

		virtual int32_t CreateObject(Object* pOuter, const guid_t& iid, Object** ppVal) = 0;

		HAS_IID;
	};

	OOCore_Export int32_t GetObjectFactory(const guid_t& oid, ObjectFactory::Flags_t flags, const guid_t& iid, Object** ppVal);
	OOCore_Export int32_t CreateObject(const guid_t& oid, ObjectFactory::Flags_t flags, Object* pOuter, const guid_t& iid, Object** ppVal);
	OOCore_Export int32_t GetTypeInfo(const guid_t& interface_id, const guid_t& iid, Object** ppVal);
	
	class RunningObjectTable : public Object
	{
	public:
		virtual int32_t GetObject(const guid_t& oid, Object** ppObj) = 0;

		HAS_IID;
	};

	// ROT Helpers
	OOCore_Export int GetRunningObjectTable(RunningObjectTable** ppROT);
		
	class InputStream : public Object
	{
	public:
		virtual int ReadBoolean(bool_t& val) = 0;
		virtual int ReadChar(char_t& val) = 0;
		virtual int ReadByte(byte_t& val) = 0;
		virtual int ReadShort(int16_t& val) = 0;
		virtual int ReadUShort(uint16_t& val) = 0;
		virtual int ReadLong(int32_t& val) = 0;
		virtual int ReadULong(uint32_t& val) = 0;
		virtual int ReadLongLong(int64_t& val) = 0;
		virtual int ReadULongLong(uint64_t& val) = 0;
		virtual int ReadFloat(real4_t& val) = 0;
		virtual int ReadDouble(real8_t& val) = 0;
		
		HAS_IID;
	};

	class OutputStream : public Object
	{
	public:
		virtual int WriteBoolean(bool_t val) = 0;
		virtual int WriteChar(char_t val) = 0;
		virtual int WriteByte(byte_t val) = 0;
		virtual int WriteShort(int16_t val) = 0;
		virtual int WriteUShort(uint16_t val) = 0;
		virtual int WriteLong(int32_t val) = 0;
		virtual int WriteULong(uint32_t val) = 0;
		virtual int WriteLongLong(int64_t val) = 0;
		virtual int WriteULongLong(uint64_t val) = 0;
		virtual int WriteFloat(real4_t val) = 0;
		virtual int WriteDouble(real8_t val) = 0;
				
		HAS_IID;
	};

	class Transport : public Object
	{
	public:
		virtual int CreateOutputStream(OutputStream** ppStream) = 0;
		virtual int Send(OutputStream* output) = 0;
		virtual int Close() = 0;
		
		HAS_IID;
	};

	/*class Transport : public Channel
	{
	public:
		virtual int32_t ActivateObject(const guid_t& oid, Object* pOuter, const guid_t& iid, Object** ppVal) = 0;

		HAS_IID;
	};*/

	class TypeInfo : public Object
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

		virtual int GetInterfaceInfo(const OOObject::char_t** type_name, size_t* method_count) = 0;
		virtual int GetMethodInfo(size_t method, const OOObject::char_t** method_name, size_t* param_count, Method_Attributes_t* attributes, OOObject::uint16_t* wait_secs) = 0;
		virtual int GetParamInfo(size_t method, size_t param, const OOObject::char_t** param_name, TypeInfo::Type_t* type) = 0;

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

		HAS_IID;
	};

	class Stub : public Object
	{
	public:
		virtual int Invoke(uint32_t method, TypeInfo::Method_Attributes_t flags, uint16_t wait_secs, InputStream* input, OutputStream* output) = 0;
		virtual int GetObject(Object** ppVal) = 0;

		HAS_IID;
	};

	class ObjectManager : public Object
	{
	};

	class ProxyStubManager : public Object
	{
	public:
		virtual int CreateProxy(const guid_t& iid, const uint32_t& key, Object** ppVal) = 0;
		virtual int CreateStub(const guid_t& iid, Object* pObj, uint32_t* key) = 0;
		//virtual int ReleaseProxy(const uint32_t& key) = 0;
		//virtual int ReleaseStub(const uint32_t& key) = 0;
		virtual int CreateRequest(uint32_t method, TypeInfo::Method_Attributes_t flags, const uint32_t& proxy_key, uint32_t* trans_id, OutputStream** output) = 0;
		virtual int CancelRequest(uint32_t trans_id) = 0;
		virtual int32_t SendAndReceive(TypeInfo::Method_Attributes_t flags, uint16_t wait_secs, OutputStream* output, uint32_t trans_id, InputStream** input) = 0;
		
		HAS_IID;
	};

	class Proxy : public Object
	{
	public:
		virtual int GetManager(ProxyStubManager** ppManager) = 0;
		virtual int GetKey(uint32_t* proxy_key) = 0;

		HAS_IID;
	};

	class Protocol : public Object
	{
	public:
		virtual int32_t Connect(const char_t* remote_addr, Transport** ppTransport) = 0;

		HAS_IID;
	};

	class RegistryKey : public Object
	{
	public:
		virtual int QueryValue(const char_t* name, char_t* value, size_t* size) = 0;
		virtual int RemoveValue(const char_t* name) = 0;
		virtual int SetValue(const char_t* name, const char_t* value) = 0;

		HAS_IID;
	};

	OOCore_Export int OpenRegistryKey(const char_t* key, OOObject::bool_t create, OOObject::RegistryKey** ppRegKey);	
};

#endif // OOCORE_OBJECT_H_INCLUDED_
