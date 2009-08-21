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

#ifndef OTL_EXCEPTION_H_INCLUDED_
#define OTL_EXCEPTION_H_INCLUDED_

#include "OTL.h"
#include <OOCore/Remoting.h>

namespace OTL
{
	template <typename E>
	class ExceptionImpl :
		public ObjectBase,
		public E
	{
	public:
		ObjectPtr<Omega::IException>	m_ptrCause;
		Omega::string_t					m_strDesc;
		Omega::string_t					m_strSource;

		BEGIN_INTERFACE_MAP(ExceptionImpl)
			INTERFACE_ENTRY(Omega::IException)
			INTERFACE_ENTRY(E)
		END_INTERFACE_MAP()

	// IException members
	public:
		virtual Omega::guid_t GetThrownIID()
		{
			return OMEGA_GUIDOF(E);
		}
		virtual Omega::IException* GetCause()
		{
			return m_ptrCause.AddRef();
		}
		virtual Omega::string_t GetDescription()
		{
			return m_strDesc;
		}
		virtual Omega::string_t GetSource()
		{
			return m_strSource;
		}
	};

	template <typename E>
	class ExceptionMarshalFactoryImpl :
		public ObjectBase,
		public Omega::Remoting::IMarshalFactory
	{
	public:
		BEGIN_INTERFACE_MAP(ExceptionMarshalFactoryImpl)
			INTERFACE_ENTRY(Omega::Remoting::IMarshalFactory)
		END_INTERFACE_MAP()

	// IMarshalFactory members
	public:
		virtual void UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags, Omega::IObject*& pObject)
		{
			ObjectPtr<ObjectImpl<E> > ptrE = ObjectImpl<E>::CreateInstancePtr();
			ptrE->UnmarshalInterface(pObjectManager,pMessage,flags);
			pObject = ptrE->QueryInterface(iid);
		}
	};

	template <typename E, const Omega::guid_t* pOID>
	class ExceptionAutoMarshalImpl :
		public OTL::ExceptionImpl<E>,
		public Omega::Remoting::IMarshal
	{
	public:
		BEGIN_INTERFACE_MAP(ExceptionAutoMarshalImpl)
			INTERFACE_ENTRY_FUNCTION(Omega::Remoting::IMarshal,&ExceptionAutoMarshalImpl::QIMarshal)
			INTERFACE_ENTRY_CHAIN(OTL::ExceptionImpl<E>)
		END_INTERFACE_MAP()

		virtual void UnmarshalInterface(Omega::Remoting::IObjectManager* pManager, Omega::Remoting::IMessage* pMessage, Omega::Remoting::MarshalFlags_t)
		{
			if (pMessage->ReadStrings(L"m_strDesc",1,&this->m_strDesc) != 1)
				OMEGA_THROW(L"Unexpected end of message");

			if (pMessage->ReadStrings(L"m_strSource",1,&this->m_strSource) != 1)
				OMEGA_THROW(L"Unexpected end of message");

			Omega::guid_t actual_iid = OMEGA_GUIDOF(Omega::IException);
			IObject* pUI = 0;
			pManager->UnmarshalInterface(L"m_ptrCause",pMessage,actual_iid,pUI);
			this->m_ptrCause.Attach(static_cast<Omega::IException*>(pUI));
		}

	private:
		Omega::IObject* QIMarshal(const Omega::guid_t&)
		{
			Omega::IObject* pRet = static_cast<Omega::Remoting::IMarshal*>(this);
			pRet->AddRef();
			return pRet;
		}

	// IMarshal members
	public:
		virtual Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			return *pOID;
		}

		virtual void MarshalInterface(Omega::Remoting::IObjectManager* pManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			pMessage->WriteStrings(L"m_strDesc",1,&this->m_strDesc);
			pMessage->WriteStrings(L"m_strSource",1,&this->m_strSource);
			pManager->MarshalInterface(L"m_ptrCause",pMessage,OMEGA_GUIDOF(Omega::IException),this->m_ptrCause);
		}

		virtual void ReleaseMarshalData(Omega::Remoting::IObjectManager* pManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			Omega::string_t s;
			if (pMessage->ReadStrings(L"m_strDesc",1,&s) != 1)
				OMEGA_THROW(L"Unexpected end of message");

			if (pMessage->ReadStrings(L"m_strSource",1,&s) != 1)
				OMEGA_THROW(L"Unexpected end of message");

			pManager->ReleaseMarshalData(L"m_ptrCause",pMessage,OMEGA_GUIDOF(Omega::IException),this->m_ptrCause);
		}
	};
}

#endif // OTL_EXCEPTION_H_INCLUDED_
