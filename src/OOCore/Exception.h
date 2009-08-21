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
	// {35F2702C-0A1B-4962-A012-F6BBBF4B0732}
	extern "C" const Omega::guid_t OID_SystemExceptionMarshalFactory;

	class SystemException :
		public OTL::ExceptionAutoMarshalImpl<Omega::ISystemException, &OID_SystemExceptionMarshalFactory>
	{
		typedef OTL::ExceptionAutoMarshalImpl<Omega::ISystemException, &OID_SystemExceptionMarshalFactory> baseClass;
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
		public OTL::ExceptionMarshalFactoryImpl<SystemException>
	{
	};

	// {1E127359-1542-4329-8E30-FED8FF810960}
	extern "C" const Omega::guid_t OID_NoInterfaceExceptionMarshalFactory;

	class NoInterfaceException :
		public OTL::ExceptionAutoMarshalImpl<Omega::INoInterfaceException, &OID_NoInterfaceExceptionMarshalFactory>
	{
		typedef OTL::ExceptionAutoMarshalImpl<Omega::INoInterfaceException, &OID_NoInterfaceExceptionMarshalFactory> baseClass;
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
		public OTL::ExceptionMarshalFactoryImpl<NoInterfaceException>
	{
	};

	// {8FA37F2C-8252-437e-9C54-F07C13152E94}
	extern "C" const Omega::guid_t OID_TimeoutExceptionMarshalFactory;

	class TimeoutException :
		public OTL::ExceptionAutoMarshalImpl<Omega::ITimeoutException, &OID_TimeoutExceptionMarshalFactory>
	{
		typedef OTL::ExceptionAutoMarshalImpl<Omega::ITimeoutException, &OID_TimeoutExceptionMarshalFactory> baseClass;
	public:
		BEGIN_INTERFACE_MAP(TimeoutException)
			INTERFACE_ENTRY_CHAIN(baseClass)
		END_INTERFACE_MAP()
	};

	class TimeoutExceptionMarshalFactoryImpl :
		public OTL::AutoObjectFactorySingleton<TimeoutExceptionMarshalFactoryImpl,&OOCore::OID_TimeoutExceptionMarshalFactory,Omega::Activation::InProcess>,
		public OTL::ExceptionMarshalFactoryImpl<TimeoutException>
	{
	};

	// {029B38C5-CC76-4d13-98A4-83A65D40710A}
	extern "C" const Omega::guid_t OID_ChannelClosedExceptionMarshalFactory;

	class ChannelClosedException :
		public OTL::ExceptionAutoMarshalImpl<Omega::Remoting::IChannelClosedException, &OID_ChannelClosedExceptionMarshalFactory>
	{
		typedef OTL::ExceptionAutoMarshalImpl<Omega::Remoting::IChannelClosedException, &OID_ChannelClosedExceptionMarshalFactory> baseClass;
	public:
		BEGIN_INTERFACE_MAP(ChannelClosedException)
			INTERFACE_ENTRY_CHAIN(baseClass)
		END_INTERFACE_MAP()
	};

	class ChannelClosedExceptionMarshalFactoryImpl :
		public OTL::AutoObjectFactorySingleton<ChannelClosedExceptionMarshalFactoryImpl,&OOCore::OID_ChannelClosedExceptionMarshalFactory,Omega::Activation::InProcess>,
		public OTL::ExceptionMarshalFactoryImpl<ChannelClosedException>
	{
	};
}

#endif // OOCORE_EXCEPTION_H_INCLUDED_
