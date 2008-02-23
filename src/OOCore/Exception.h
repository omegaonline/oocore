///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
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

#ifndef OOCORE_EXCEPTION_H_INCLUDED_
#define OOCORE_EXCEPTION_H_INCLUDED_

namespace OOCore
{
	// {35F2702C-0A1B-4962-A012-F6BBBF4B0732}
	OMEGA_DECLARE_OID(OID_SystemExceptionMarshalFactory);

	// {1E127359-1542-4329-8E30-FED8FF810960}
	OMEGA_DECLARE_OID(OID_NoInterfaceExceptionMarshalFactory);

	class SystemException :
		public OTL::ExceptionImpl<Omega::ISystemException, &OID_SystemExceptionMarshalFactory>
	{
		typedef OTL::ExceptionImpl<Omega::ISystemException, &OID_SystemExceptionMarshalFactory> baseClass;
	public:
		Omega::uint32_t m_errno;

		BEGIN_INTERFACE_MAP(SystemException)
			INTERFACE_ENTRY_CHAIN(baseClass)
		END_INTERFACE_MAP()

		virtual void UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::UnmarshalInterface(pObjectManager,pStream,flags);
			m_errno = pStream->ReadInt32();
		}

		virtual void MarshalInterface(Omega::Remoting::IObjectManager* pManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::MarshalInterface(pManager,pStream,iid,flags);
			pStream->WriteUInt32(m_errno);
		}

		virtual void ReleaseMarshalData(Omega::Remoting::IObjectManager* pManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::ReleaseMarshalData(pManager,pStream,iid,flags);
			pStream->ReadUInt32();
		}

	// ISystemException memebers
	public:
		virtual Omega::uint32_t ErrorCode()
		{
			return m_errno;
		}
	};

	class SystemExceptionMarshalFactoryImpl :
		public OTL::AutoObjectFactoryNoAggregation<SystemExceptionMarshalFactoryImpl,&OOCore::OID_SystemExceptionMarshalFactory>,
		public OTL::ExceptionMarshalFactoryImpl<SystemException>
	{
	};

	class NoInterfaceException :
		public OTL::ExceptionImpl<Omega::INoInterfaceException, &OID_NoInterfaceExceptionMarshalFactory>
	{
		typedef OTL::ExceptionImpl<Omega::INoInterfaceException, &OID_NoInterfaceExceptionMarshalFactory> baseClass;
	public:
		Omega::guid_t m_iid;

		BEGIN_INTERFACE_MAP(NoInterfaceException)
			INTERFACE_ENTRY_CHAIN(baseClass)
		END_INTERFACE_MAP()

		virtual void UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::UnmarshalInterface(pObjectManager,pStream,flags);
			m_iid = pStream->ReadGuid();
		}

		virtual void MarshalInterface(Omega::Remoting::IObjectManager* pManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::MarshalInterface(pManager,pStream,iid,flags);
			pStream->WriteGuid(m_iid);
		}

		virtual void ReleaseMarshalData(Omega::Remoting::IObjectManager* pManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::ReleaseMarshalData(pManager,pStream,iid,flags);
			pStream->ReadGuid();
		}

	// INoInterfaceException members
	public:
		inline Omega::guid_t GetUnsupportedIID()
		{
			return m_iid;
		}
	};

	class NoInterfaceExceptionMarshalFactoryImpl :
		public OTL::AutoObjectFactoryNoAggregation<NoInterfaceExceptionMarshalFactoryImpl,&OOCore::OID_NoInterfaceExceptionMarshalFactory>,
		public OTL::ExceptionMarshalFactoryImpl<NoInterfaceException>
	{
	};
}

#endif // OOCORE_EXCEPTION_H_INCLUDED_
