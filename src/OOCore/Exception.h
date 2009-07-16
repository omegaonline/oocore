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

#ifndef OOCORE_EXCEPTION_H_INCLUDED_
#define OOCORE_EXCEPTION_H_INCLUDED_

#include <OTL/Exception.h>

namespace OOCore
{
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

	template <typename E>
	class ExceptionMarshalFactoryImpl :
		public OTL::ObjectBase,
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
			OTL::ObjectPtr<OTL::ObjectImpl<E> > ptrE = OTL::ObjectImpl<E>::CreateInstancePtr();
			ptrE->UnmarshalInterface(pObjectManager,pMessage,flags);

			pObject = ptrE->QueryInterface(iid);
			if (!pObject)
				throw Omega::INoInterfaceException::Create(iid);
		}
	};

	// {35F2702C-0A1B-4962-A012-F6BBBF4B0732}
	extern "C" const Omega::guid_t OID_SystemExceptionMarshalFactory;

	class SystemException :
		public ExceptionAutoMarshalImpl<Omega::ISystemException, &OID_SystemExceptionMarshalFactory>
	{
		typedef ExceptionAutoMarshalImpl<Omega::ISystemException, &OID_SystemExceptionMarshalFactory> baseClass;
	public:
		Omega::uint32_t m_errno;

		BEGIN_INTERFACE_MAP(SystemException)
			INTERFACE_ENTRY_CHAIN(baseClass)
		END_INTERFACE_MAP()

		virtual void UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::UnmarshalInterface(pObjectManager,pMessage,flags);
			if (pMessage->ReadUInt32s(L"m_errno",1,&m_errno) != 1)
				OMEGA_THROW(L"Unexpected end of message");
		}

		virtual void MarshalInterface(Omega::Remoting::IObjectManager* pManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::MarshalInterface(pManager,pMessage,iid,flags);
			pMessage->WriteUInt32s(L"m_errno",1,&m_errno);
		}

		virtual void ReleaseMarshalData(Omega::Remoting::IObjectManager* pManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::ReleaseMarshalData(pManager,pMessage,iid,flags);
			Omega::uint32_t e;
			if (pMessage->ReadUInt32s(L"m_errno",1,&e) != 1)
				OMEGA_THROW(L"Unexpected end of message");
		}

	// ISystemException memebers
	public:
		virtual Omega::uint32_t GetErrorCode()
		{
			return m_errno;
		}
	};

	class SystemExceptionMarshalFactoryImpl :
		public OTL::AutoObjectFactorySingleton<SystemExceptionMarshalFactoryImpl,&OOCore::OID_SystemExceptionMarshalFactory,Omega::Activation::InProcess>,
		public ExceptionMarshalFactoryImpl<SystemException>
	{
	};

	// {1E127359-1542-4329-8E30-FED8FF810960}
	extern "C" const Omega::guid_t OID_NoInterfaceExceptionMarshalFactory;

	class NoInterfaceException :
		public ExceptionAutoMarshalImpl<Omega::INoInterfaceException, &OID_NoInterfaceExceptionMarshalFactory>
	{
		typedef ExceptionAutoMarshalImpl<Omega::INoInterfaceException, &OID_NoInterfaceExceptionMarshalFactory> baseClass;
	public:
		Omega::guid_t m_iid;

		BEGIN_INTERFACE_MAP(NoInterfaceException)
			INTERFACE_ENTRY_CHAIN(baseClass)
		END_INTERFACE_MAP()

		virtual void UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Remoting::IMessage* pMessage, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::UnmarshalInterface(pObjectManager,pMessage,flags);
			if (pMessage->ReadGuids(L"m_iid",1,&m_iid) != 1)
				OMEGA_THROW(L"Unexpected end of message");
		}

		virtual void MarshalInterface(Omega::Remoting::IObjectManager* pManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::MarshalInterface(pManager,pMessage,iid,flags);
			pMessage->WriteGuids(L"m_iid",1,&m_iid);
		}

		virtual void ReleaseMarshalData(Omega::Remoting::IObjectManager* pManager, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::ReleaseMarshalData(pManager,pMessage,iid,flags);
			Omega::guid_t g;
			if (pMessage->ReadGuids(L"m_iid",1,&g) != 1)
				OMEGA_THROW(L"Unexpected end of message");
		}

	// INoInterfaceException members
	public:
		inline Omega::guid_t GetUnsupportedIID()
		{
			return m_iid;
		}
	};

	class NoInterfaceExceptionMarshalFactoryImpl :
		public OTL::AutoObjectFactorySingleton<NoInterfaceExceptionMarshalFactoryImpl,&OOCore::OID_NoInterfaceExceptionMarshalFactory,Omega::Activation::InProcess>,
		public ExceptionMarshalFactoryImpl<NoInterfaceException>
	{
	};

	// {8FA37F2C-8252-437e-9C54-F07C13152E94}
	extern "C" const Omega::guid_t OID_TimeoutExceptionMarshalFactory;

	class TimeoutException :
		public ExceptionAutoMarshalImpl<Omega::ITimeoutException, &OID_TimeoutExceptionMarshalFactory>
	{
		typedef ExceptionAutoMarshalImpl<Omega::ITimeoutException, &OID_TimeoutExceptionMarshalFactory> baseClass;
	public:
		BEGIN_INTERFACE_MAP(TimeoutException)
			INTERFACE_ENTRY_CHAIN(baseClass)
		END_INTERFACE_MAP()
	};

	class TimeoutExceptionMarshalFactoryImpl :
		public OTL::AutoObjectFactorySingleton<TimeoutExceptionMarshalFactoryImpl,&OOCore::OID_TimeoutExceptionMarshalFactory,Omega::Activation::InProcess>,
		public ExceptionMarshalFactoryImpl<TimeoutException>
	{
	};

	// {029B38C5-CC76-4d13-98A4-83A65D40710A}
	extern "C" const Omega::guid_t OID_ChannelClosedExceptionMarshalFactory;

	class ChannelClosedException :
		public ExceptionAutoMarshalImpl<Omega::Remoting::IChannelClosedException, &OID_ChannelClosedExceptionMarshalFactory>
	{
		typedef ExceptionAutoMarshalImpl<Omega::Remoting::IChannelClosedException, &OID_ChannelClosedExceptionMarshalFactory> baseClass;
	public:
		BEGIN_INTERFACE_MAP(ChannelClosedException)
			INTERFACE_ENTRY_CHAIN(baseClass)
		END_INTERFACE_MAP()
	};

	class ChannelClosedExceptionMarshalFactoryImpl :
		public OTL::AutoObjectFactorySingleton<ChannelClosedExceptionMarshalFactoryImpl,&OOCore::OID_ChannelClosedExceptionMarshalFactory,Omega::Activation::InProcess>,
		public ExceptionMarshalFactoryImpl<ChannelClosedException>
	{
	};
}

#endif // OOCORE_EXCEPTION_H_INCLUDED_
