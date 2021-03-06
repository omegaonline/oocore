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

namespace OOCore
{
	// {35F2702C-0A1B-4962-A012-F6BBBF4B0732}
	extern const Omega::guid_t OID_SystemExceptionMarshalFactory;

	class SystemException :
			public OTL::ExceptionAutoMarshalImpl<Omega::ISystemException, &OID_SystemExceptionMarshalFactory>
	{
		typedef OTL::ExceptionAutoMarshalImpl<Omega::ISystemException, &OID_SystemExceptionMarshalFactory> baseClass;
	public:
		Omega::uint32_t m_errno;

		BEGIN_INTERFACE_MAP(SystemException)
			INTERFACE_ENTRY_CHAIN(baseClass)
		END_INTERFACE_MAP()

		virtual void UnmarshalInterface(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::UnmarshalInterface(pMarshaller,pMessage,flags);
			m_errno = pMessage->ReadValue(Omega::string_t::constant("m_errno")).cast<Omega::uint32_t>();
		}

		virtual void MarshalInterface(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::MarshalInterface(pMarshaller,pMessage,iid,flags);
			pMessage->WriteValue(Omega::string_t::constant("m_errno"),m_errno);
		}

		virtual void ReleaseMarshalData(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::ReleaseMarshalData(pMarshaller,pMessage,iid,flags);
			pMessage->ReadValue(Omega::string_t::constant("m_errno"));
		}

	// ISystemException memebers
	public:
		virtual Omega::uint32_t GetErrorCode()
		{
			return m_errno;
		}
	};

	class SystemExceptionMarshalFactoryImpl :
			public OTL::AutoObjectFactorySingleton<SystemExceptionMarshalFactoryImpl,&OID_SystemExceptionMarshalFactory,Omega::Activation::ProcessScope>,
			public OTL::ExceptionMarshalFactoryImpl<SystemException>
	{
	};

	// {47E86F31-E9E9-4667-89CA-40EB048DA2B7}
	extern const Omega::guid_t OID_InternalExceptionMarshalFactory;

	class InternalException :
			public OTL::ExceptionAutoMarshalImpl<Omega::IInternalException, &OID_InternalExceptionMarshalFactory>
	{
		typedef OTL::ExceptionAutoMarshalImpl<Omega::IInternalException, &OID_InternalExceptionMarshalFactory> baseClass;
	public:
		BEGIN_INTERFACE_MAP(InternalException)
			INTERFACE_ENTRY_CHAIN(baseClass)
		END_INTERFACE_MAP()

		virtual void UnmarshalInterface(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::UnmarshalInterface(pMarshaller,pMessage,flags);
			m_strSource = pMessage->ReadValue(Omega::string_t::constant("m_strSource")).cast<Omega::string_t>();
		}

		virtual void MarshalInterface(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::MarshalInterface(pMarshaller,pMessage,iid,flags);
			pMessage->WriteValue(Omega::string_t::constant("m_strSource"),m_strSource);
		}

		virtual void ReleaseMarshalData(Omega::Remoting::IMarshaller* pMarshaller, Omega::Remoting::IMessage* pMessage, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::ReleaseMarshalData(pMarshaller,pMessage,iid,flags);
			pMessage->ReadValue(Omega::string_t::constant("m_strSource"));
		}

		Omega::string_t m_strSource;

	// IInternalException memebers
	public:
		virtual Omega::string_t GetSource()
		{
			return m_strSource;
		}
	};

	class InternalExceptionMarshalFactoryImpl :
			public OTL::AutoObjectFactorySingleton<InternalExceptionMarshalFactoryImpl,&OID_InternalExceptionMarshalFactory,Omega::Activation::ProcessScope>,
			public OTL::ExceptionMarshalFactoryImpl<InternalException>
	{
	};

	// {1E127359-1542-4329-8E30-FED8FF810960}
	extern const Omega::guid_t OID_NotFoundExceptionMarshalFactory;

	class NotFoundException :
			public OTL::ExceptionAutoMarshalImpl<Omega::INotFoundException, &OID_NotFoundExceptionMarshalFactory>
	{
		typedef OTL::ExceptionAutoMarshalImpl<Omega::INotFoundException, &OID_NotFoundExceptionMarshalFactory> baseClass;
	public:
		BEGIN_INTERFACE_MAP(NotFoundException)
			INTERFACE_ENTRY_CHAIN(baseClass)
		END_INTERFACE_MAP()
	};

	class NotFoundExceptionMarshalFactoryImpl :
			public OTL::AutoObjectFactorySingleton<NotFoundExceptionMarshalFactoryImpl,&OID_NotFoundExceptionMarshalFactory,Omega::Activation::ProcessScope>,
			public OTL::ExceptionMarshalFactoryImpl<NotFoundException>
	{
	};

	// {BA90E55F-E0B6-0528-C45F-32DD9C3A414E}
	extern const Omega::guid_t OID_AlreadyExistsExceptionMarshalFactory;

	class AlreadyExistsException :
			public OTL::ExceptionAutoMarshalImpl<Omega::IAlreadyExistsException, &OID_AlreadyExistsExceptionMarshalFactory>
	{
		typedef OTL::ExceptionAutoMarshalImpl<Omega::IAlreadyExistsException, &OID_AlreadyExistsExceptionMarshalFactory> baseClass;
	public:
		BEGIN_INTERFACE_MAP(AlreadyExistsException)
			INTERFACE_ENTRY_CHAIN(baseClass)
		END_INTERFACE_MAP()
	};

	class AlreadyExistsExceptionMarshalFactoryImpl :
			public OTL::AutoObjectFactorySingleton<AlreadyExistsExceptionMarshalFactoryImpl,&OID_AlreadyExistsExceptionMarshalFactory,Omega::Activation::ProcessScope>,
			public OTL::ExceptionMarshalFactoryImpl<AlreadyExistsException>
	{
	};

	// {5CA887CE-648C-BBE4-9B66-14F275879CFB}
	extern const Omega::guid_t OID_AccessDeniedExceptionMarshalFactory;

	class AccessDeniedException :
			public OTL::ExceptionAutoMarshalImpl<Omega::IAccessDeniedException, &OID_AccessDeniedExceptionMarshalFactory>
	{
		typedef OTL::ExceptionAutoMarshalImpl<Omega::IAccessDeniedException, &OID_AccessDeniedExceptionMarshalFactory> baseClass;
	public:
		BEGIN_INTERFACE_MAP(AccessDeniedException)
			INTERFACE_ENTRY_CHAIN(baseClass)
		END_INTERFACE_MAP()
	};

	class AccessDeniedExceptionMarshalFactoryImpl :
			public OTL::AutoObjectFactorySingleton<AccessDeniedExceptionMarshalFactoryImpl,&OID_AccessDeniedExceptionMarshalFactory,Omega::Activation::ProcessScope>,
			public OTL::ExceptionMarshalFactoryImpl<AccessDeniedException>
	{
	};

	// {8FA37F2C-8252-437e-9C54-F07C13152E94}
	extern const Omega::guid_t OID_TimeoutExceptionMarshalFactory;

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
			public OTL::AutoObjectFactorySingleton<TimeoutExceptionMarshalFactoryImpl,&OID_TimeoutExceptionMarshalFactory,Omega::Activation::ProcessScope>,
			public OTL::ExceptionMarshalFactoryImpl<TimeoutException>
	{
	};

	// {029B38C5-CC76-4d13-98A4-83A65D40710A}
	extern const Omega::guid_t OID_ChannelClosedExceptionMarshalFactory;

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
			public OTL::AutoObjectFactorySingleton<ChannelClosedExceptionMarshalFactoryImpl,&OID_ChannelClosedExceptionMarshalFactory,Omega::Activation::ProcessScope>,
			public OTL::ExceptionMarshalFactoryImpl<ChannelClosedException>
	{
	};
}

#endif // OOCORE_EXCEPTION_H_INCLUDED_
