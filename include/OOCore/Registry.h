#ifndef OOCORE_REGISTRY_H_INCLUDED_
#define OOCORE_REGISTRY_H_INCLUDED_

#include <OOCore/Enum.h>

namespace Omega {
namespace Registry 
{
	interface IRegistryKey : public IObject
	{
		enum ValueType
		{
			String = 0,
			UInt32 = 1,
			Binary = 2
		};

		enum OpenFlags
		{
			OpenExisting = 0,
			Create = 1,
			FailIfThere = 2
		};
		typedef uint16_t OpenFlags_t;

		virtual bool_t IsSubKey(const string_t& key) = 0;
		virtual bool_t IsValue(const string_t& name) = 0;
		virtual string_t GetStringValue(const string_t& name) = 0;
		virtual uint32_t GetUIntValue(const string_t& name) = 0;
		virtual void GetBinaryValue(const string_t& name, byte_t* pBuffer, uint32_t& cbLen) = 0;
		virtual void SetStringValue(const string_t& name, const string_t& val) = 0;
		virtual void SetUIntValue(const string_t& name, const uint32_t& val) = 0;
		virtual void SetBinaryValue(const string_t& name, const byte_t* val, uint32_t cbLen) = 0;
		virtual ValueType GetValueType(const string_t& name) = 0;
		virtual IRegistryKey* OpenSubKey(const string_t& key, OpenFlags_t flags = OpenExisting) = 0;
		virtual IEnumString* EnumSubKeys() = 0;
		virtual IEnumString* EnumValues() = 0;
		virtual void DeleteKey(const string_t& strKey) = 0;
		virtual void DeleteValue(const string_t& strValue) = 0;

		static IRegistryKey* OpenKey(const string_t& key, OpenFlags_t flags = OpenExisting);		
	};
	OMEGA_DECLARE_IID(IRegistryKey);

	interface INotFoundException : public IException
	{
		virtual string_t GetValueName() = 0;
	};
	OMEGA_DECLARE_IID(INotFoundException); 

	interface IAlreadyExistsException : public IException
	{
		virtual string_t GetKeyName() = 0;
	};
	OMEGA_DECLARE_IID(IAlreadyExistsException);

	interface IBadNameException : public IException
	{
		virtual string_t GetName() = 0;
	};
	OMEGA_DECLARE_IID(IBadNameException);

	interface IWrongValueTypeException : public IException
	{
		virtual IRegistryKey::ValueType GetValueType() = 0;
	};
	OMEGA_DECLARE_IID(IWrongValueTypeException);
}
}

OMEGA_EXPORT_INTERFACE
(
	Omega::Registry, IRegistryKey,
	0x8bc942a4, 0xf2f1, 0x4ef9, 0xbf, 0x9c, 0xd9, 0x20, 0xbe, 0x56, 0xeb, 0x9c,

	// Methods
	OMEGA_METHOD(Omega::bool_t,IsSubKey,1,((in),const Omega::string_t&,key))
	OMEGA_METHOD(Omega::bool_t,IsValue,1,((in),const Omega::string_t&,name))
	OMEGA_METHOD(Omega::string_t,GetStringValue,1,((in),const Omega::string_t&,name))
	OMEGA_METHOD(uint32_t,GetUIntValue,1,((in),const Omega::string_t&,name))
	OMEGA_METHOD_VOID(GetBinaryValue,3,((in),const Omega::string_t&,name,(out)(size_is(cbLen)),Omega::byte_t*,pBuffer,(in_out),Omega::uint32_t&,cbLen))
	OMEGA_METHOD_VOID(SetStringValue,2,((in),const Omega::string_t&,name,(in),const Omega::string_t&,val))
	OMEGA_METHOD_VOID(SetUIntValue,2,((in),const Omega::string_t&,name,(in),const Omega::uint32_t&,val))
	OMEGA_METHOD_VOID(SetBinaryValue,3,((in),const Omega::string_t&,name,(in)(size_is(cbLen)),const Omega::byte_t*,val,(in),Omega::uint32_t,cbLen))
	OMEGA_METHOD(Omega::Registry::IRegistryKey::ValueType,GetValueType,1,((in),const Omega::string_t&,name))
	OMEGA_METHOD(Omega::Registry::IRegistryKey*,OpenSubKey,2,((in),const Omega::string_t&,key,(in),Omega::Registry::IRegistryKey::OpenFlags_t,flags))
	OMEGA_METHOD(IEnumString*,EnumSubKeys,0,())
	OMEGA_METHOD(IEnumString*,EnumValues,0,())
	OMEGA_METHOD_VOID(DeleteKey,1,((in),const Omega::string_t&,strKey))
	OMEGA_METHOD_VOID(DeleteValue,1,((in),const Omega::string_t&,strValue))
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Registry, INotFoundException, Omega, IException,
	0x4ab1051, 0x81cf, 0x4c8c, 0x89, 0xd8, 0x21, 0x7f, 0x3c, 0x9a, 0x42, 0x49,

	// Methods
	OMEGA_METHOD(Omega::string_t,GetValueName,0,())
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Registry, IWrongValueTypeException, Omega, IException,
	0x96b103c, 0x360f, 0x4ada, 0x80, 0x6f, 0xd3, 0x37, 0xd6, 0x32, 0x2e, 0xb6,

	// Methods
	OMEGA_METHOD(Omega::Registry::IRegistryKey::ValueType,GetValueType,0,())
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Registry, IBadNameException, Omega, IException,
	0x18677696, 0x219c, 0x4e0a, 0xa9, 0x97, 0xc7, 0x56, 0x50, 0xe8, 0x46, 0x6e,

	// Methods
	OMEGA_METHOD(Omega::string_t,GetName,0,())
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Registry, IAlreadyExistsException, Omega, IException,
	0xee80fdeb, 0xc1ea, 0x49d1, 0x9f, 0x76, 0x9a, 0x1b, 0xb4, 0xdc, 0x97, 0x6,

	// Methods
	OMEGA_METHOD(Omega::string_t,GetKeyName,0,())
)

OOCORE_EXPORTED_FUNCTION(Omega::Registry::IRegistryKey*,IRegistryKey_OpenKey,2,((in),const Omega::string_t&,key,(in),Omega::Registry::IRegistryKey::OpenFlags_t,flags));
inline Omega::Registry::IRegistryKey* Omega::Registry::IRegistryKey::OpenKey(const Omega::string_t& key, Omega::Registry::IRegistryKey::OpenFlags_t flags)
{
	return IRegistryKey_OpenKey(key,flags);
}

#endif  // OOCORE_REGISTRY_H_INCLUDED_
