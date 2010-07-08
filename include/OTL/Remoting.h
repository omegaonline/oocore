///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
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

#ifndef OTL_REMOTING_H_INCLUDED_
#define OTL_REMOTING_H_INCLUDED_

// Specialisations of ObjectPtr for remoting
#include "OTL.h"

#include "../Omega/Remoting.h"

namespace OTL
{
	template <>
	class ObjectPtr<Omega::Remoting::IMarshaller> : public ObjectPtrBase<Omega::Remoting::IMarshaller>
	{
	public:
		ObjectPtr(Omega::Remoting::IMarshaller* pMarshaller = 0) :
				ObjectPtrBase<Omega::Remoting::IMarshaller>(pMarshaller)
		{ }

		template <typename I>
		ObjectPtr(I* pObject) :
				ObjectPtrBase<Omega::Remoting::IMarshaller>(0)
		{
			if (pObject)
				this->m_ptr = static_cast<Omega::Remoting::IMarshaller*>(pObject->QueryInterface(OMEGA_GUIDOF(Omega::Remoting::IMarshaller)));
		}

		ObjectPtr(const ObjectPtr<Omega::Remoting::IMarshaller>& rhs) :
				ObjectPtrBase<Omega::Remoting::IMarshaller>(rhs)
		{ }

		template <typename I>
		ObjectPtr(const ObjectPtr<I>& rhs) :
				ObjectPtrBase<Omega::Remoting::IMarshaller>(0)
		{
			if (rhs)
				this->m_ptr = static_cast<Omega::Remoting::IMarshaller*>(rhs->QueryInterface(OMEGA_GUIDOF(Omega::Remoting::IMarshaller)));
		}

		ObjectPtr& operator = (const ObjectPtr<Omega::Remoting::IMarshaller>& rhs)
		{
			if (this != &rhs)
				*this = rhs.m_ptr;

			return *this;
		}

		ObjectPtr& operator = (Omega::Remoting::IMarshaller* obj)
		{
			ObjectPtrBase<Omega::Remoting::IMarshaller>::operator = (obj);
			return *this;
		}

		template <typename I>
		ObjectPtr<I> UnmarshalInterface(const Omega::string_t& strName, Omega::Remoting::IMessage* pMessage)
		{
			Omega::IObject* pObj = 0;
			m_ptr->UnmarshalInterface(strName,pMessage,OMEGA_GUIDOF(I),pObj);

			ObjectPtr<I> ptrObj;
			ptrObj.Attach(static_cast<I*>(pObj));
			return ptrObj;
		}
	};

	template <>
	class ObjectPtr<Omega::Remoting::IChannel> : public ObjectPtrBase<Omega::Remoting::IChannel>
	{
	public:
		ObjectPtr(Omega::Remoting::IChannel* pMarshaller = 0) :
				ObjectPtrBase<Omega::Remoting::IChannel>(pMarshaller)
		{ }

		template <typename I>
		ObjectPtr(I* pObject) :
				ObjectPtrBase<Omega::Remoting::IChannel>(0)
		{
			if (pObject)
				this->m_ptr = static_cast<Omega::Remoting::IChannel*>(pObject->QueryInterface(OMEGA_GUIDOF(Omega::Remoting::IChannel)));
		}

		ObjectPtr(const ObjectPtr<Omega::Remoting::IChannel>& rhs) :
				ObjectPtrBase<Omega::Remoting::IChannel>(rhs)
		{ }

		template <typename I>
		ObjectPtr(const ObjectPtr<I>& rhs) :
				ObjectPtrBase<Omega::Remoting::IChannel>(0)
		{
			if (rhs)
				this->m_ptr = static_cast<Omega::Remoting::IChannel*>(rhs->QueryInterface(OMEGA_GUIDOF(Omega::Remoting::IChannel)));
		}

		ObjectPtr& operator = (const ObjectPtr<Omega::Remoting::IChannel>& rhs)
		{
			if (this != &rhs)
				*this = rhs.m_ptr;

			return *this;
		}

		ObjectPtr& operator = (Omega::Remoting::IChannel* obj)
		{
			ObjectPtrBase<Omega::Remoting::IChannel>::operator = (obj);
			return *this;
		}

		template <typename I>
		ObjectPtr<I> GetManager()
		{
			Omega::IObject* pObj = 0;
			m_ptr->GetManager(OMEGA_GUIDOF(I),pObj);

			ObjectPtr<I> ptrObj;
			ptrObj.Attach(static_cast<I*>(pObj));
			return ptrObj;
		}
	};
}

#endif // OTL_REMOTING_H_INCLUDED_
