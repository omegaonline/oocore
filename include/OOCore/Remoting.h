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
		enum MarshalFlags
		{
			same = 0,               // Objects are in the same context
			apartment = 1,          // Objects share address space, but not thread
			inter_process = 2,      // Objects share user id, but not address space
			inter_user = 3,         // Objects share machine, but not user id or address space
			another_machine = 4     // Objects on separate machines and share nothing
		};
		typedef uint16_t MarshalFlags_t;

		interface IChannel : public IObject
		{
			virtual IO::IFormattedStream* CreateOutputStream() = 0;
			virtual IException* SendAndReceive(MethodAttributes_t attribs, IO::IFormattedStream* pSend, IO::IFormattedStream*& pRecv, uint16_t timeout = 0) = 0;
			virtual MarshalFlags_t GetMarshalFlags() = 0;
			virtual uint32_t GetSource() = 0;
		};

		interface ICallContext : public IObject
		{
			virtual void Deadline(uint64_t& secs, int32_t& usecs) = 0;
			virtual uint32_t Timeout() = 0;
			virtual bool_t HasTimedOut() = 0;
			virtual uint32_t SourceId() = 0;
			virtual MarshalFlags_t SourceType() = 0;
		};

		inline ICallContext* GetCallContext();

		interface IObjectManager : public IObject
		{
			virtual void Connect(IChannel* pChannel) = 0;
			virtual void Invoke(IO::IFormattedStream* pParamsIn, IO::IFormattedStream* pParamsOut, uint64_t deadline_secs, int32_t deadline_usecs) = 0;
			virtual void Disconnect() = 0;
			virtual void CreateRemoteInstance(const guid_t& oid, const guid_t& iid, IObject* pOuter, IObject*& pObject) = 0;
			virtual void MarshalInterface(IO::IFormattedStream* pStream, const guid_t& iid, IObject* pObject) = 0;
			virtual void ReleaseMarshalData(IO::IFormattedStream* pStream, const guid_t& iid, IObject* pObject) = 0;
			virtual void UnmarshalInterface(IO::IFormattedStream* pStream, const guid_t& iid, IObject*& pObject) = 0;
		};

		interface IMarshal : public IObject
		{
			virtual guid_t GetUnmarshalFactoryOID(const guid_t& iid, MarshalFlags_t flags) = 0;
			virtual void MarshalInterface(IObjectManager* pObjectManager, IO::IFormattedStream* pStream, const guid_t& iid, MarshalFlags_t flags) = 0;
			virtual void ReleaseMarshalData(IObjectManager* pObjectManager, IO::IFormattedStream* pStream, const guid_t& iid, MarshalFlags_t flags) = 0;
		};

		interface IMarshalFactory : public IObject
		{
			virtual void UnmarshalInterface(IObjectManager* pObjectManager, IO::IFormattedStream* pStream, const guid_t& iid, MarshalFlags_t flags, IObject*& pObject) = 0;
		};

		interface IInterProcessService : public IObject
		{
			virtual Registry::IRegistryKey* GetRegistry() = 0;
			virtual Activation::IRunningObjectTable* GetRunningObjectTable() = 0;
			virtual bool_t ExecProcess(const string_t& strProcess, bool_t bPublic) = 0;
			virtual IO::IStream* OpenStream(const string_t& strEndPoint, IO::IAsyncStreamCallback* pCallback) = 0;
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

	OMEGA_METHOD(IO::IFormattedStream*,CreateOutputStream,0,())
	OMEGA_METHOD(IException*,SendAndReceive,4,((in),Remoting::MethodAttributes_t,attribs,(in),IO::IFormattedStream*,pSend,(out),IO::IFormattedStream*&,pRecv,(in),uint16_t,timeout))
	OMEGA_METHOD(Remoting::MarshalFlags_t,GetMarshalFlags,0,())
	OMEGA_METHOD(uint32_t,GetSource,0,())
)

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Omega::Remoting, ICallContext, "{05340979-0CEA-48f6-91C9-2FE13F8546E0}",

	OMEGA_METHOD_VOID(Deadline,2,((out),uint64_t&,secs,(out),int32_t&,usecs))
	OMEGA_METHOD(uint32_t,Timeout,0,())
	OMEGA_METHOD(bool_t,HasTimedOut,0,())
	OMEGA_METHOD(uint32_t,SourceId,0,())
	OMEGA_METHOD(Remoting::MarshalFlags_t,SourceType,0,())
)

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Omega::Remoting, IObjectManager, "{0A6F7B1B-26A0-403c-AC80-ADFADA83615D}",

	OMEGA_METHOD_VOID(Connect,1,((in),Remoting::IChannel*,pChannel))
	OMEGA_METHOD_VOID(Invoke,4,((in),IO::IFormattedStream*,pParamsIn,(in),IO::IFormattedStream*,pParamsOut,(in),uint64_t,deadline_secs,(in),int32_t,deadline_usecs))
	OMEGA_METHOD_VOID(Disconnect,0,())
	OMEGA_METHOD_VOID(CreateRemoteInstance,4,((in),const guid_t&,oid,(in),const guid_t&,iid,(in),IObject*,pOuter,(out)(iid_is(iid)),IObject*&,pObject))
	OMEGA_METHOD_VOID(MarshalInterface,3,((in),IO::IFormattedStream*,pStream,(in),const guid_t&,iid,(in)(iid_is(iid)),IObject*,pObject))
	OMEGA_METHOD_VOID(ReleaseMarshalData,3,((in),IO::IFormattedStream*,pStream,(in),const guid_t&,iid,(in)(iid_is(iid)),IObject*,pObject))
	OMEGA_METHOD_VOID(UnmarshalInterface,3,((in),IO::IFormattedStream*,pStream,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
)

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Omega::Remoting, IMarshal, "{5EE81A3F-88AA-47ee-9CAA-CECC8BE8F4C4}",

	OMEGA_METHOD(guid_t,GetUnmarshalFactoryOID,2,((in),const guid_t&,iid,(in),Remoting::MarshalFlags_t,flags))
	OMEGA_METHOD_VOID(MarshalInterface,4,((in),Remoting::IObjectManager*,pObjectManager,(in),IO::IFormattedStream*,pStream,(in),const guid_t&,iid,(in),Remoting::MarshalFlags_t,flags))
	OMEGA_METHOD_VOID(ReleaseMarshalData,4,((in),Remoting::IObjectManager*,pObjectManager,(in),IO::IFormattedStream*,pStream,(in),const guid_t&,iid,(in),Remoting::MarshalFlags_t,flags))
)

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Omega::Remoting, IMarshalFactory, "{68C779B3-72E7-4c09-92F0-118A01AF224D}",

	OMEGA_METHOD_VOID(UnmarshalInterface,5,((in),Remoting::IObjectManager*,pObjectManager,(in),IO::IFormattedStream*,pStream,(in),const guid_t&,iid,(in),Remoting::MarshalFlags_t,flags,(out)(iid_is(iid)),IObject*&,pObject))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Remoting, IInterProcessService, "{70F6D098-6E53-4e8d-BF21-9EA359DC4FF8}",

	OMEGA_METHOD(Registry::IRegistryKey*,GetRegistry,0,())
	OMEGA_METHOD(Activation::IRunningObjectTable*,GetRunningObjectTable,0,())
	OMEGA_METHOD(bool_t,ExecProcess,2,((in),const string_t&,strProcess,(in),bool_t,bPublic))
	OMEGA_METHOD(IO::IStream*,OpenStream,2,((in),const string_t&,strEndPoint,(in),IO::IAsyncStreamCallback*,pCallback))
)

OMEGA_EXPORTED_FUNCTION(Omega::Remoting::ICallContext*,Remoting_GetCallContext,0,())
Omega::Remoting::ICallContext* Omega::Remoting::GetCallContext()
{
	return Remoting_GetCallContext();
}

#endif // OOCORE_REMOTING_H_INCLUDED_
