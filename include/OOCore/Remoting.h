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

#ifndef OOCORE_REMOTING_H_INCLUDED_
#define OOCORE_REMOTING_H_INCLUDED_

#include "OOCore.h"

namespace Omega
{
	namespace Remoting
	{
		enum MarshalFlags
		{
			Same = 0,              ///< Objects are in the same context
			Apartment = 1,         ///< Objects share address space, but not thread
			InterProcess = 2,      ///< Objects share user id, but not address space
			InterUser = 3,         ///< Objects share machine, but not user id or address space
			RemoteMachine = 4      ///< Objects on separate machines and share nothing
		};
		typedef uint16_t MarshalFlags_t;

		interface IChannelBase : public IObject
		{
			virtual IMessage* CreateMessage() = 0;
			virtual IException* SendAndReceive(TypeInfo::MethodAttributes_t attribs, IMessage* pSend, IMessage*& pRecv, uint32_t timeout = 0) = 0;
			virtual MarshalFlags_t GetMarshalFlags() = 0;
			virtual uint32_t GetSource() = 0;
			virtual bool_t IsConnected() = 0;
		};

		interface ICallContext : public IObject
		{
			virtual uint32_t Timeout() = 0;
			virtual bool_t HasTimedOut() = 0;
			virtual uint32_t SourceId() = 0;
			virtual MarshalFlags_t SourceType() = 0;
		};

		inline ICallContext* GetCallContext();

		interface IObjectManager : public IObject
		{
			virtual void Connect(IChannelBase* pChannel) = 0;
			virtual IMessage* Invoke(IMessage* pParamsIn, uint32_t timeout) = 0;
			virtual void Shutdown() = 0;
			virtual void GetRemoteInstance(const Omega::string_t& strOID, Activation::Flags_t flags, const guid_t& iid, IObject*& pObject) = 0;
			virtual void MarshalInterface(const wchar_t* pszName, IMessage* pMessage, const guid_t& iid, IObject* pObject) = 0;
			virtual void ReleaseMarshalData(const wchar_t* pszName, IMessage* pMessage, const guid_t& iid, IObject* pObject) = 0;
			virtual void UnmarshalInterface(const wchar_t* pszName, IMessage* pMessage, const guid_t& iid, IObject*& pObject) = 0;
		};

		interface IChannel : public IChannelBase
		{
			virtual guid_t GetReflectUnmarshalFactoryOID() = 0;
			virtual void ReflectMarshal(IMessage* pMessage) = 0;
			virtual IObjectManager* GetObjectManager() = 0;
		};

		interface IChannelClosedException : public IException
		{
			inline static IChannelClosedException* Create();
		};

		interface IMarshal : public IObject
		{
			virtual guid_t GetUnmarshalFactoryOID(const guid_t& iid, MarshalFlags_t flags) = 0;
			virtual void MarshalInterface(IObjectManager* pObjectManager, IMessage* pMessage, const guid_t& iid, MarshalFlags_t flags) = 0;
			virtual void ReleaseMarshalData(IObjectManager* pObjectManager, IMessage* pMessage, const guid_t& iid, MarshalFlags_t flags) = 0;
		};

		interface IMarshalFactory : public IObject
		{
			virtual void UnmarshalInterface(IObjectManager* pObjectManager, IMessage* pMessage, const guid_t& iid, MarshalFlags_t flags, IObject*& pObject) = 0;
		};

		interface IChannelSink : public IObject
		{
			virtual void Send(TypeInfo::MethodAttributes_t attribs, IMessage* pMsg, uint32_t timeout) = 0;
			virtual void Close() = 0;

			inline static IChannelSink* OpenServerSink(const guid_t& message_oid, IChannelSink* pSink);
		};

		interface IEndpoint : public IObject
		{
			virtual string_t Canonicalise(const string_t& strEndpoint) = 0;
			virtual guid_t MessageOid() = 0;
			virtual IChannelSink* Open(const string_t& strEndpoint, IChannelSink* pSink) = 0;
		};
		
		/// {63EB243E-6AE3-43BD-B073-764E096775F8}
		OMEGA_DECLARE_OID(OID_StdObjectManager);
	}
}

#if !defined(DOXYGEN)

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Omega::Remoting, IChannelBase, "{F18430B0-8AC5-4b57-9B66-56B3BE867C24}",

	OMEGA_METHOD(Remoting::IMessage*,CreateMessage,0,())
	OMEGA_METHOD(IException*,SendAndReceive,4,((in),TypeInfo::MethodAttributes_t,attribs,(in),Remoting::IMessage*,pSend,(out),Remoting::IMessage*&,pRecv,(in),uint32_t,timeout))
	OMEGA_METHOD(Remoting::MarshalFlags_t,GetMarshalFlags,0,())
	OMEGA_METHOD(uint32_t,GetSource,0,())
	OMEGA_METHOD(bool_t,IsConnected,0,())
)

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Omega::Remoting, ICallContext, "{05340979-0CEA-48f6-91C9-2FE13F8546E0}",

	OMEGA_METHOD(uint32_t,Timeout,0,())
	OMEGA_METHOD(bool_t,HasTimedOut,0,())
	OMEGA_METHOD(uint32_t,SourceId,0,())
	OMEGA_METHOD(Remoting::MarshalFlags_t,SourceType,0,())
)

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Omega::Remoting, IObjectManager, "{0A6F7B1B-26A0-403c-AC80-ADFADA83615D}",

	OMEGA_METHOD_VOID(Connect,1,((in),Remoting::IChannelBase*,pChannel))
	OMEGA_METHOD(Remoting::IMessage*,Invoke,2,((in),Remoting::IMessage*,pParamsIn,(in),uint32_t,timeout))
	OMEGA_METHOD_VOID(Shutdown,0,())
	OMEGA_METHOD_VOID(GetRemoteInstance,4,((in),const string_t&,strOID,(in),Activation::Flags_t,flags,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
	OMEGA_METHOD_VOID(MarshalInterface,4,((in),const wchar_t*,pszName,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(in)(iid_is(iid)),IObject*,pObject))
	OMEGA_METHOD_VOID(ReleaseMarshalData,4,((in),const wchar_t*,pszName,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(in)(iid_is(iid)),IObject*,pObject))
	OMEGA_METHOD_VOID(UnmarshalInterface,4,((in),const wchar_t*,pszName,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
)

OMEGA_DEFINE_INTERFACE_DERIVED_LOCAL
(
	Omega::Remoting, IChannel, Omega::Remoting, IChannelBase, "{1F6D79E8-94E2-46ef-BAB8-E46B9EAC6B84}",

	OMEGA_METHOD(guid_t,GetReflectUnmarshalFactoryOID,0,())
	OMEGA_METHOD_VOID(ReflectMarshal,1,((in),Remoting::IMessage*,pMessage))
	OMEGA_METHOD(Remoting::IObjectManager*,GetObjectManager,0,())
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::Remoting, IChannelClosedException, Omega, IException, "{E0BB01D1-CF43-4da0-97E0-E40B66A2CFE7}",

	OMEGA_NO_METHODS()
	)

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Omega::Remoting, IMarshal, "{5EE81A3F-88AA-47ee-9CAA-CECC8BE8F4C4}",

	OMEGA_METHOD(guid_t,GetUnmarshalFactoryOID,2,((in),const guid_t&,iid,(in),Remoting::MarshalFlags_t,flags))
	OMEGA_METHOD_VOID(MarshalInterface,4,((in),Remoting::IObjectManager*,pObjectManager,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(in),Remoting::MarshalFlags_t,flags))
	OMEGA_METHOD_VOID(ReleaseMarshalData,4,((in),Remoting::IObjectManager*,pObjectManager,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(in),Remoting::MarshalFlags_t,flags))
)

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Omega::Remoting, IMarshalFactory, "{68C779B3-72E7-4c09-92F0-118A01AF224D}",

	OMEGA_METHOD_VOID(UnmarshalInterface,5,((in),Remoting::IObjectManager*,pObjectManager,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(in),Remoting::MarshalFlags_t,flags,(out)(iid_is(iid)),IObject*&,pObject))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Remoting, IChannelSink, "{C395066A-05D1-45f2-95C5-272319CF1394}",

	OMEGA_METHOD_EX_VOID(Asynchronous,0,Send,3,((in),TypeInfo::MethodAttributes_t,attribs,(in),Remoting::IMessage*,pMsg,(in),uint32_t,timeout))
	OMEGA_METHOD_VOID(Close,0,())
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Remoting, IEndpoint, "{5194AF93-D1C2-4bfa-AA12-8E6CDCF6CDBA}",

	OMEGA_METHOD(string_t,Canonicalise,1,((in),const string_t&,strEndpoint))
	OMEGA_METHOD(guid_t,MessageOid,0,())
	OMEGA_METHOD(Remoting::IChannelSink*,Open,2,((in),const string_t&,strEndpoint,(in),Remoting::IChannelSink*,pSink))
)

OMEGA_EXPORTED_FUNCTION(Omega::Remoting::IChannelClosedException*,OOCore_Remoting_IChannelClosedException_Create,0,())
Omega::Remoting::IChannelClosedException* Omega::Remoting::IChannelClosedException::Create()
{
	return OOCore_Remoting_IChannelClosedException_Create();
}

OMEGA_EXPORTED_FUNCTION(Omega::Remoting::ICallContext*,OOCore_Remoting_GetCallContext,0,())
Omega::Remoting::ICallContext* Omega::Remoting::GetCallContext()
{
	return OOCore_Remoting_GetCallContext();
}

OMEGA_EXPORTED_FUNCTION(Omega::Remoting::IChannelSink*,OOCore_Remoting_OpenServerSink,2,((in),const Omega::guid_t&,message_oid,(in),Omega::Remoting::IChannelSink*,pSink))
Omega::Remoting::IChannelSink* Omega::Remoting::IChannelSink::OpenServerSink(const guid_t& message_oid, Remoting::IChannelSink* pSink)
{
	return OOCore_Remoting_OpenServerSink(message_oid,pSink);
}

#endif // !defined(DOXYGEN)

#endif // OOCORE_REMOTING_H_INCLUDED_
