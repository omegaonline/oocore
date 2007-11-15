///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 1999 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

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
