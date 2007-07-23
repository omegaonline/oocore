#ifndef OOSERVER_USER_REGISTRY_H_INCLUDED_
#define OOSERVER_USER_REGISTRY_H_INCLUDED_

namespace User
{
	class Manager;

	namespace Registry
	{
		class RootKey : 
			public OTL::ObjectBase,
			public Omega::Registry::IRegistryKey
		{
		public:
			void Init(Manager* pManager, const Omega::string_t& strKey);
			void EnumSubKeys(std::set<Omega::string_t>& setStrings);

			BEGIN_INTERFACE_MAP(RootKey)
				INTERFACE_ENTRY(Omega::Registry::IRegistryKey)
			END_INTERFACE_MAP()

		private:
			Manager*        m_pManager;
			Omega::string_t m_strKey;

			Omega::string_t FullKeyPath(const Omega::string_t& strSub);

		// IRegistry members
		public:
			Omega::bool_t IsSubKey(const Omega::string_t& strSubKey);
			Omega::bool_t IsValue(const Omega::string_t& strName);
			Omega::string_t GetStringValue(const Omega::string_t& strName);
			Omega::uint32_t GetUIntValue(const Omega::string_t& strName);
			void GetBinaryValue(const Omega::string_t& strName, Omega::uint32_t& cbLen, Omega::byte_t* pBuffer);
			void SetStringValue(const Omega::string_t& strName, const Omega::string_t& strValue);
			void SetUIntValue(const Omega::string_t& strName, Omega::uint32_t uValue);
			void SetBinaryValue(const Omega::string_t& strName, Omega::uint32_t cbLen, const Omega::byte_t* val);
			Omega::Registry::ValueType_t GetValueType(const Omega::string_t& strName);
			Omega::Registry::IRegistryKey* OpenSubKey(const Omega::string_t& strSubKey, Omega::Registry::IRegistryKey::OpenFlags_t flags = OpenExisting);
			Omega::IEnumString* EnumSubKeys();
			Omega::IEnumString* EnumValues();
			void DeleteKey(const Omega::string_t& strSubKey);
			void DeleteValue(const Omega::string_t& strName);
		};

		class UserKey : 
			public OTL::ObjectBase,
			public Omega::Registry::IRegistryKey
		{
		public:
			UserKey();

			void Init(ACE_Configuration_Heap* pRegistry, const ACE_Configuration_Section_Key& strKey, ACE_RW_Thread_Mutex* pLock, const ACE_WString& strKeyName);

			BEGIN_INTERFACE_MAP(UserKey)
				INTERFACE_ENTRY(Omega::Registry::IRegistryKey)
			END_INTERFACE_MAP()

		private:
			ACE_Configuration_Heap*        m_pRegistry;
			ACE_Configuration_Section_Key  m_key;
			ACE_RW_Thread_Mutex*           m_pLock;
			ACE_WString                    m_strKeyName;

			Omega::string_t FullKeyPath(const Omega::string_t& strSub);
			
		// IRegistry members
		public:
			Omega::bool_t IsSubKey(const Omega::string_t& strSubKey);
			Omega::bool_t IsValue(const Omega::string_t& strName);
			Omega::string_t GetStringValue(const Omega::string_t& strName);
			Omega::uint32_t GetUIntValue(const Omega::string_t& strName);
			void GetBinaryValue(const Omega::string_t& strName, Omega::uint32_t& cbLen, Omega::byte_t* pBuffer);
			void SetStringValue(const Omega::string_t& strName, const Omega::string_t& strValue);
			void SetUIntValue(const Omega::string_t& strName, Omega::uint32_t uValue);
			void SetBinaryValue(const Omega::string_t& strName, Omega::uint32_t cbLen, const Omega::byte_t* val);
			Omega::Registry::ValueType_t GetValueType(const Omega::string_t& strName);
			Omega::Registry::IRegistryKey* OpenSubKey(const Omega::string_t& strSubKey, Omega::Registry::IRegistryKey::OpenFlags_t flags = OpenExisting);
			Omega::IEnumString* EnumSubKeys();
			Omega::IEnumString* EnumValues();
			void DeleteKey(const Omega::string_t& strSubKey);
			void DeleteValue(const Omega::string_t& strName);
		};

		class BaseKey : 
			public OTL::ObjectBase,
			public Omega::Registry::IRegistryKey
		{
		public:
			void Init(Manager* pManager, bool bSandbox);

			BEGIN_INTERFACE_MAP(BaseKey)
				INTERFACE_ENTRY(Omega::Registry::IRegistryKey)
			END_INTERFACE_MAP()

		private:
			OTL::ObjectPtr<OTL::ObjectImpl<RootKey> >   m_ptrRoot;
			OTL::ObjectPtr<OTL::ObjectImpl<UserKey> >   m_ptrUser;
			ACE_Configuration_Heap                      m_registry;
			ACE_RW_Thread_Mutex                         m_lock;
			
			int open_registry(bool bSandbox);

		// IRegistry members
		public:
			Omega::bool_t IsSubKey(const Omega::string_t& strSubKey);
			Omega::bool_t IsValue(const Omega::string_t& strName);
			Omega::string_t GetStringValue(const Omega::string_t& strName);
			Omega::uint32_t GetUIntValue(const Omega::string_t& strName);
			void GetBinaryValue(const Omega::string_t& strName, Omega::uint32_t& cbLen, Omega::byte_t* pBuffer);
			void SetStringValue(const Omega::string_t& strName, const Omega::string_t& strValue);
			void SetUIntValue(const Omega::string_t& strName, Omega::uint32_t uValue);
			void SetBinaryValue(const Omega::string_t& strName, Omega::uint32_t cbLen, const Omega::byte_t* val);
			Omega::Registry::ValueType_t GetValueType(const Omega::string_t& strName);
			Omega::Registry::IRegistryKey* OpenSubKey(const Omega::string_t& strSubKey, Omega::Registry::IRegistryKey::OpenFlags_t flags = OpenExisting);
			Omega::IEnumString* EnumSubKeys();
			Omega::IEnumString* EnumValues();
			void DeleteKey(const Omega::string_t& strSubKey);
			void DeleteValue(const Omega::string_t& strName);
		};
	}
}

#endif // OOSERVER_USER_REGISTRY_H_INCLUDED_
