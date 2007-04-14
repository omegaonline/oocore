#ifndef OOSERVER_USER_REGISTRY_H_INCLUDED_
#define OOSERVER_USER_REGISTRY_H_INCLUDED_

namespace User
{
class Manager;

namespace Registry
{

class Root : 
	public OTL::ObjectBase,
	public Omega::Registry::IRegistryKey
{
public:
	void Init(Manager* pManager);

	BEGIN_INTERFACE_MAP(Root)
		INTERFACE_ENTRY(Omega::Registry::IRegistryKey)
	END_INTERFACE_MAP()

private:
	Manager* m_pManager;

// IRegistry members
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

class CurrentUser : 
	public OTL::ObjectBase,
	public Omega::Registry::IRegistryKey
{
public:
	void Init(const ACE_Configuration_Section_Key& key);

	BEGIN_INTERFACE_MAP(CurrentUser)
		INTERFACE_ENTRY(Omega::Registry::IRegistryKey)
	END_INTERFACE_MAP()

private:
	ACE_Configuration_Section_Key	m_key;
	
// IRegistry members
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

class Base : 
	public OTL::ObjectBase,
	public Omega::Registry::IRegistryKey
{
public:
	void Init(Manager* pManager);

	BEGIN_INTERFACE_MAP(Base)
		INTERFACE_ENTRY(Omega::Registry::IRegistryKey)
	END_INTERFACE_MAP()

private:
	OTL::ObjectPtr<OTL::ObjectImpl<Root> > m_ptrRootReg;
	OTL::ObjectPtr<OTL::ObjectImpl<CurrentUser> > m_ptrUserReg;
	ACE_Configuration_Heap                 m_registry;
	
	int open_registry();

// IRegistry members
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

}
}

#endif // OOSERVER_USER_REGISTRY_H_INCLUDED_