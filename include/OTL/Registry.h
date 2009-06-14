///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
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

#ifndef OTL_REGISTRY_H_INCLUDED_
#define OTL_REGISTRY_H_INCLUDED_

// Specialisations of ObjectPtr for registry access
#include "OTL.h"

namespace OTL
{
	template <>
	class ObjectPtr<Omega::Registry::IKey> : public ObjectPtrBase<Omega::Registry::IKey>
	{
	public:
		ObjectPtr(Omega::Registry::IKey* pKey = 0) :
		  ObjectPtrBase<Omega::Registry::IKey>(pKey)
		{ }

		ObjectPtr(const ObjectPtr<Omega::Registry::IKey>& rhs) :
		  ObjectPtrBase<Omega::Registry::IKey>(rhs)
		{ }

		ObjectPtr(const Omega::string_t& key, Omega::Registry::IKey::OpenFlags_t flags = Omega::Registry::IKey::OpenExisting) :
		  ObjectPtrBase<Omega::Registry::IKey>(0)
		{
			Attach(Omega::Registry::IKey::OpenKey(key,flags));
		}

		ObjectPtr& operator = (const ObjectPtr<Omega::Registry::IKey>& rhs)
		{ 
			if (this != &rhs)
				*this = rhs.m_ptr;

			return *this;
		}

		ObjectPtr& operator = (Omega::Registry::IKey* obj)
		{
			ObjectPtrBase<Omega::Registry::IKey>::operator = (obj);
			return *this;
		}

		ObjectPtr<Omega::Registry::IKey> OpenSubKey(const Omega::string_t& key, Omega::Registry::IKey::OpenFlags_t flags = Omega::Registry::IKey::OpenExisting)
		{
			ObjectPtr<Omega::Registry::IKey> sub_key;
			sub_key.Attach(m_ptr->OpenSubKey(key,flags));
			return sub_key;
		}

		ObjectPtr<Omega::IEnumString> EnumSubKeys()
		{
			ObjectPtr<Omega::IEnumString> en;
			en.Attach(m_ptr->EnumSubKeys());
			return en;
		}

		ObjectPtr<Omega::IEnumString> EnumValues()
		{
			ObjectPtr<Omega::IEnumString> en;
			en.Attach(m_ptr->EnumValues());
			return en;
		}
	};
}

#endif // OTL_REGISTRY_H_INCLUDED_
