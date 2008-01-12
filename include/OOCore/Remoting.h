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

#ifndef OOCORE_REMOTING_H_INCLUDED_
#define OOCORE_REMOTING_H_INCLUDED_

#include <OOCore/OOCore.h>

namespace Omega
{
	namespace Remoting
	{
		interface IChannel : public IObject
		{
			virtual Serialize::IFormattedStream* CreateOutputStream() = 0;
			virtual IException* SendAndReceive(MethodAttributes_t attribs, Serialize::IFormattedStream* pSend, Serialize::IFormattedStream*& pRecv, uint16_t timeout) = 0;
		};

		enum MarshalFlags
		{
			same = 0,               // Objects are in the same context
			apartment = 1,          // Objects share address space, but not thread
			inter_process = 2,      // Objects share user id, but not address space
			inter_user = 3,         // Objects share machine, but not user id or address space
			another_machine = 4     // Objects on separate machines and share nothing
		};
		typedef uint16_t MarshalFlags_t;

		interface IObjectManager : public IObject
		{
			virtual void Connect(IChannel* pChannel, MarshalFlags_t marshal_flags) = 0;
			virtual void Invoke(Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut) = 0;
			virtual void Disconnect() = 0;
			virtual void CreateRemoteInstance(const guid_t& oid, const guid_t& iid, IObject* pOuter, IObject*& pObject) = 0;
			virtual void MarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject* pObject) = 0;
			virtual void ReleaseMarshalData(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject* pObject) = 0;
			virtual void UnmarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject*& pObject) = 0;
		};

		interface IMarshal : public IObject
		{
			virtual guid_t GetUnmarshalFactoryOID(const guid_t& iid, MarshalFlags_t flags) = 0;
			virtual void MarshalInterface(IObjectManager* pObjectManager, Serialize::IFormattedStream* pStream, const guid_t& iid, MarshalFlags_t flags) = 0;
			virtual void ReleaseMarshalData(IObjectManager* pObjectManager, Serialize::IFormattedStream* pStream, const guid_t& iid, MarshalFlags_t flags) = 0;
		};

		interface IMarshalFactory : public IObject
		{
			virtual void UnmarshalInterface(IObjectManager* pObjectManager, Serialize::IFormattedStream* pStream, const guid_t& iid, MarshalFlags_t flags, IObject*& pObject) = 0;
		};

		interface IInterProcessService : public IObject
		{
			virtual Registry::IRegistryKey* GetRegistry() = 0;
			virtual Activation::IRunningObjectTable* GetRunningObjectTable() = 0;
		};

		// {63EB243E-6AE3-43bd-B073-764E096775F8}
		OMEGA_DECLARE_OID(OID_StdObjectManager);

		// {7E9E22E8-C0B0-43f9-9575-BFB1665CAE4A}
		OMEGA_DECLARE_OID(OID_InterProcessService);
	}
}

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Omega::Remoting, IChannel, "{F18430B0-8AC5-4b57-9B66-56B3BE867C24}",

	OMEGA_METHOD(Serialize::IFormattedStream*,CreateOutputStream,0,())
	OMEGA_METHOD(IException*,SendAndReceive,4,((in),Remoting::MethodAttributes_t,attribs,(in),Serialize::IFormattedStream*,pSend,(out),Serialize::IFormattedStream*&,pRecv,(in),uint16_t,timeout))
)

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Omega::Remoting, IObjectManager, "{0A6F7B1B-26A0-403c-AC80-ADFADA83615D}",

	OMEGA_METHOD_VOID(Connect,2,((in),Remoting::IChannel*,pChannel,(in),Omega::Remoting::MarshalFlags_t,flags))
	OMEGA_METHOD_VOID(Invoke,2,((in),Serialize::IFormattedStream*,pParamsIn,(in),Serialize::IFormattedStream*,pParamsOut))
	OMEGA_METHOD_VOID(Disconnect,0,())
	OMEGA_METHOD_VOID(CreateRemoteInstance,4,((in),const guid_t&,oid,(in),const guid_t&,iid,(in),IObject*,pOuter,(out)(iid_is(iid)),IObject*&,pObject))
	OMEGA_METHOD_VOID(MarshalInterface,3,((in),Serialize::IFormattedStream*,pStream,(in),const guid_t&,iid,(in)(iid_is(iid)),IObject*,pObject))
	OMEGA_METHOD_VOID(ReleaseMarshalData,3,((in),Serialize::IFormattedStream*,pStream,(in),const guid_t&,iid,(in)(iid_is(iid)),IObject*,pObject))
	OMEGA_METHOD_VOID(UnmarshalInterface,3,((in),Serialize::IFormattedStream*,pStream,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
)

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Omega::Remoting, IMarshal, "{5EE81A3F-88AA-47ee-9CAA-CECC8BE8F4C4}",

	OMEGA_METHOD(guid_t,GetUnmarshalFactoryOID,2,((in),const guid_t&,iid,(in),Omega::Remoting::MarshalFlags_t,flags))
	OMEGA_METHOD_VOID(MarshalInterface,4,((in),Remoting::IObjectManager*,pObjectManager,(in),Serialize::IFormattedStream*,pStream,(in),const guid_t&,iid,(in),Omega::Remoting::MarshalFlags_t,flags))
	OMEGA_METHOD_VOID(ReleaseMarshalData,4,((in),Remoting::IObjectManager*,pObjectManager,(in),Serialize::IFormattedStream*,pStream,(in),const guid_t&,iid,(in),Omega::Remoting::MarshalFlags_t,flags))
)

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Omega::Remoting, IMarshalFactory, "{68C779B3-72E7-4c09-92F0-118A01AF224D}",

	OMEGA_METHOD_VOID(UnmarshalInterface,5,((in),Remoting::IObjectManager*,pObjectManager,(in),Serialize::IFormattedStream*,pStream,(in),const guid_t&,iid,(in),Omega::Remoting::MarshalFlags_t,flags,(out)(iid_is(iid)),IObject*&,pObject))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Remoting, IInterProcessService, "{70F6D098-6E53-4e8d-BF21-9EA359DC4FF8}",

	OMEGA_METHOD(Registry::IRegistryKey*,GetRegistry,0,())
	OMEGA_METHOD(Activation::IRunningObjectTable*,GetRunningObjectTable,0,())
)

#endif // OOCORE_REMOTING_H_INCLUDED_
