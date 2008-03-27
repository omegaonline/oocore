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
	template <typename E, const Omega::guid_t* pOID>
	class ExceptionAutoMarshalImpl :
		public OTL::ExceptionImpl<E>,
		public Omega::Remoting::IMarshal
	{
	public:

		BEGIN_INTERFACE_MAP(ExceptionAutoMarshalImpl)
			INTERFACE_ENTRY_FUNCTION(Omega::Remoting::IMarshal,ExceptionAutoMarshalImpl::QIMarshal)
			INTERFACE_ENTRY_CHAIN(OTL::ExceptionImpl<E>)
		END_INTERFACE_MAP()

		virtual void UnmarshalInterface(Omega::Remoting::IObjectManager* pManager, Omega::IO::IFormattedStream* pStream, Omega::Remoting::MarshalFlags_t)
		{
			this->m_strDesc = pStream->ReadString();
			this->m_strSource = pStream->ReadString();
			Omega::IObject* pE = 0;
			pManager->UnmarshalInterface(pStream,OMEGA_UUIDOF(Omega::IException),pE);
			this->m_ptrCause.Attach(static_cast<Omega::IException*>(pE));
		}

	private:
		Omega::IObject* QIMarshal(const Omega::guid_t&)
		{
			if (pOID == 0)
				return 0;
			else
			{
				Omega::IObject* pRet = static_cast<Omega::Remoting::IMarshal*>(this);
				pRet->AddRef();
				return pRet;
			}
		}

	// IMarshal members
	public:
		virtual Omega::guid_t GetUnmarshalFactoryOID(const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			if (pOID)
				return *pOID;
			else
				return Omega::guid_t::Null();
		}

		virtual void MarshalInterface(Omega::Remoting::IObjectManager* pManager, Omega::IO::IFormattedStream* pStream, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			pStream->WriteString(this->m_strDesc);
			pStream->WriteString(this->m_strSource);
			pManager->MarshalInterface(pStream,OMEGA_UUIDOF(Omega::IException),this->m_ptrCause);
		}

		virtual void ReleaseMarshalData(Omega::Remoting::IObjectManager* pManager, Omega::IO::IFormattedStream* pStream, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
		{
			pStream->ReadString();
			pStream->ReadString();
			pManager->ReleaseMarshalData(pStream,OMEGA_UUIDOF(Omega::IException),this->m_ptrCause);
		}
	};

	template <class E>
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
		virtual void UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::IO::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags, Omega::IObject*& pObject)
		{
			OTL::ObjectPtr<OTL::ObjectImpl<E> > ptrE = OTL::ObjectImpl<E>::CreateInstancePtr();
			ptrE->UnmarshalInterface(pObjectManager,pStream,flags);
			pObject = ptrE->QueryInterface(iid);
		}
	};

	// {35F2702C-0A1B-4962-A012-F6BBBF4B0732}
	OMEGA_DECLARE_OID(OID_SystemExceptionMarshalFactory);

	// {1E127359-1542-4329-8E30-FED8FF810960}
	OMEGA_DECLARE_OID(OID_NoInterfaceExceptionMarshalFactory);

	class SystemException :
		public ExceptionAutoMarshalImpl<Omega::ISystemException, &OID_SystemExceptionMarshalFactory>
	{
		typedef ExceptionAutoMarshalImpl<Omega::ISystemException, &OID_SystemExceptionMarshalFactory> baseClass;
	public:
		Omega::uint32_t m_errno;

		BEGIN_INTERFACE_MAP(SystemException)
			INTERFACE_ENTRY_CHAIN(baseClass)
		END_INTERFACE_MAP()

		virtual void UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::IO::IFormattedStream* pStream, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::UnmarshalInterface(pObjectManager,pStream,flags);
			m_errno = pStream->ReadInt32();
		}

		virtual void MarshalInterface(Omega::Remoting::IObjectManager* pManager, Omega::IO::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::MarshalInterface(pManager,pStream,iid,flags);
			pStream->WriteUInt32(m_errno);
		}

		virtual void ReleaseMarshalData(Omega::Remoting::IObjectManager* pManager, Omega::IO::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
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
		public ExceptionMarshalFactoryImpl<SystemException>
	{
	};

	class NoInterfaceException :
		public ExceptionAutoMarshalImpl<Omega::INoInterfaceException, &OID_NoInterfaceExceptionMarshalFactory>
	{
		typedef ExceptionAutoMarshalImpl<Omega::INoInterfaceException, &OID_NoInterfaceExceptionMarshalFactory> baseClass;
	public:
		Omega::guid_t m_iid;

		BEGIN_INTERFACE_MAP(NoInterfaceException)
			INTERFACE_ENTRY_CHAIN(baseClass)
		END_INTERFACE_MAP()

		virtual void UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::IO::IFormattedStream* pStream, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::UnmarshalInterface(pObjectManager,pStream,flags);
			m_iid = pStream->ReadGuid();
		}

		virtual void MarshalInterface(Omega::Remoting::IObjectManager* pManager, Omega::IO::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
		{
			baseClass::MarshalInterface(pManager,pStream,iid,flags);
			pStream->WriteGuid(m_iid);
		}

		virtual void ReleaseMarshalData(Omega::Remoting::IObjectManager* pManager, Omega::IO::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags)
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
		public ExceptionMarshalFactoryImpl<NoInterfaceException>
	{
	};
}

#endif // OOCORE_EXCEPTION_H_INCLUDED_
