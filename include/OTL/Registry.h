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
				ObjectPtrBase<Omega::Registry::IKey>(pKey)
		{ }

		template <typename I>
		ObjectPtr(I* pObject) :
				ObjectPtrBase<Omega::Registry::IKey>(NULL)
		{
			if (pObject)
				this->m_ptr = static_cast<Omega::Registry::IKey*>(pObject->QueryInterface(OMEGA_GUIDOF(Omega::Registry::IKey)));
		}

		ObjectPtr(const ObjectPtr<Omega::Registry::IKey>& rhs) :
				ObjectPtrBase<Omega::Registry::IKey>(rhs)
		{ }

		template <typename I>
		ObjectPtr(const ObjectPtr<I>& rhs) :
				ObjectPtrBase<Omega::Registry::IKey>(NULL)
		{
			if (rhs)
				this->m_ptr = static_cast<Omega::Registry::IKey*>(rhs->QueryInterface(OMEGA_GUIDOF(Omega::Registry::IKey)));
		}
		
		ObjectPtr(const wchar_t* key, Omega::Registry::IKey::OpenFlags_t flags = Omega::Registry::IKey::OpenExisting) :
				ObjectPtrBase<Omega::Registry::IKey>(NULL)
		{
			this->m_ptr = static_cast<Omega::Registry::IKey*>(Omega::CreateInstance(Omega::Registry::OID_Registry,Omega::Activation::Any,NULL,OMEGA_GUIDOF(Omega::Registry::IKey)));
			if (key)
				Attach(this->m_ptr->OpenSubKey(Omega::string_t(key,-1,false),flags));
		}

		ObjectPtr(const Omega::string_t& key, Omega::Registry::IKey::OpenFlags_t flags = Omega::Registry::IKey::OpenExisting) :
				ObjectPtrBase<Omega::Registry::IKey>(NULL)
		{
			this->m_ptr = static_cast<Omega::Registry::IKey*>(Omega::CreateInstance(Omega::Registry::OID_Registry,Omega::Activation::Any,NULL,OMEGA_GUIDOF(Omega::Registry::IKey)));
			if (!key.IsEmpty())
				Attach(this->m_ptr->OpenSubKey(key,flags));
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
	};
}

#endif // OTL_REGISTRY_H_INCLUDED_
