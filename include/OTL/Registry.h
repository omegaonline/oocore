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
		ObjectPtr(Omega::Registry::IKey* pKey = NULL) :
				ObjectPtrBase<Omega::Registry::IKey>(pKey,false)
		{ }

		ObjectPtr(const ObjectPtr<Omega::Registry::IKey>& rhs) :
				ObjectPtrBase<Omega::Registry::IKey>(rhs.m_ptr,true)
		{ }

		ObjectPtr(const char* key, Omega::Registry::IKey::OpenFlags_t flags = Omega::Registry::IKey::OpenExisting) :
				ObjectPtrBase<Omega::Registry::IKey>(Omega::Registry::OID_Registry,Omega::Activation::Default,NULL)
		{
			if (key && key[0] != '\0')
				replace(this->m_ptr->OpenSubKey(key,flags),false);
			else
				replace(NULL,false);
		}

		ObjectPtr(const Omega::string_t& key, Omega::Registry::IKey::OpenFlags_t flags = Omega::Registry::IKey::OpenExisting) :
				ObjectPtrBase<Omega::Registry::IKey>(Omega::Registry::OID_Registry,Omega::Activation::Default,NULL)
		{
			if (!key.IsEmpty())
				replace(this->m_ptr->OpenSubKey(key,flags),false);
			else
				replace(NULL,false);
		}

		ObjectPtr& operator = (const ObjectPtr<Omega::Registry::IKey>& rhs)
		{
			if (this != &rhs)
				replace(rhs.m_ptr,true);

			return *this;
		}

		ObjectPtr& operator = (Omega::Registry::IKey* obj)
		{
			replace(obj,false);
			return *this;
		}
	};
}

#endif // OTL_REGISTRY_H_INCLUDED_
