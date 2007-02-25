#ifndef OOSERVER_USER_REGISTRY_H_INCLUDED_
#define OOSERVER_USER_REGISTRY_H_INCLUDED_

class UserRegistry : 
	public OTL::ObjectBase,
	public Omega::Registry::IRegistryKey
{
public:
	


	BEGIN_INTERFACE_MAP(UserRegistry)
		INTERFACE_ENTRY(Omega::Registry::IRegistryKey)
	END_INTERFACE_MAP()

//IRegistry members
public:
	Omega::bool_t IsSubKey(const Omega::string_t& key);
	Omega::bool_t IsValue(const Omega::string_t& name);
	Omega::string_t GetStringValue(const Omega::string_t& name);
	Omega::uint32_t GetUIntValue(const Omega::string_t& name);
	void GetBinaryValue(const Omega::string_t& name, Omega::uint32_t& cbLen, Omega::byte_t* pBuffer);
	void SetStringValue(const Omega::string_t& name, const Omega::string_t& val);
	void SetUIntValue(const Omega::string_t& name, const Omega::uint32_t& val);
	void SetBinaryValue(const Omega::string_t& name, Omega::uint32_t cbLen, const Omega::byte_t* val);
	Omega::Registry::IRegistryKey::ValueType_t GetValueType(const Omega::string_t& name);
	Omega::Registry::IRegistryKey* OpenSubKey(const Omega::string_t& key, Omega::Registry::IRegistryKey::OpenFlags_t flags = OpenExisting);
	Omega::IEnumString* EnumSubKeys();
	Omega::IEnumString* EnumValues();
	void DeleteKey(const Omega::string_t& strKey);
	void DeleteValue(const Omega::string_t& strValue);
};

#endif // OOSERVER_USER_REGISTRY_H_INCLUDED_