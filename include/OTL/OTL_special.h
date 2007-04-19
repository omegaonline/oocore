#ifndef OTL_OTL_SPECIAL_H_INCLUDED_
#define OTL_OTL_SPECIAL_H_INCLUDED_

// Specialisations of ObjectPtr

namespace OTL
{
	template <>
	class ObjectPtr<Omega::Registry::IRegistryKey> : public ObjectPtrBase<Omega::Registry::IRegistryKey>
	{
	public:
		ObjectPtr<Omega::Registry::IRegistryKey>(Omega::Registry::IRegistryKey* pKey = 0) :
		  ObjectPtrBase<Omega::Registry::IRegistryKey>(pKey)
		{ }

		ObjectPtr(const ObjectPtr<Omega::Registry::IRegistryKey>& rhs) :
		  ObjectPtrBase<Omega::Registry::IRegistryKey>(rhs)
		{ }

		ObjectPtr(const Omega::string_t& key, Omega::Registry::IRegistryKey::OpenFlags_t flags = Omega::Registry::IRegistryKey::OpenExisting) :
		  ObjectPtrBase<Omega::Registry::IRegistryKey>(0)
		{
			Attach(Omega::Registry::IRegistryKey::OpenKey(key,flags));
		}

		ObjectPtr<Omega::Registry::IRegistryKey> OpenSubKey(const Omega::string_t& key, Omega::Registry::IRegistryKey::OpenFlags_t flags = Omega::Registry::IRegistryKey::OpenExisting)
		{
			ObjectPtr<Omega::Registry::IRegistryKey> sub_key;
			sub_key.Attach(m_ptr.value()->OpenSubKey(key,flags));
			return sub_key;
		}

		ObjectPtr<Omega::IEnumString> EnumSubKeys()
		{
			ObjectPtr<Omega::IEnumString> en;
			en.Attach(m_ptr.value()->EnumSubKeys());
			return en;
		}

		ObjectPtr<Omega::IEnumString> EnumValues()
		{
			ObjectPtr<Omega::IEnumString> en;
			en.Attach(m_ptr.value()->EnumValues());
			return en;
		}
	};

}

#endif // OTL_OTL_SPECIAL_H_INCLUDED_
