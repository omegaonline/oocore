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

#ifndef OMEGA_REMOTING_H_INCLUDED_
#define OMEGA_REMOTING_H_INCLUDED_

#include "Omega.h"

namespace Omega
{
	namespace Remoting
	{
		enum MarshalFlags
		{
			Same = 0,              ///< Objects are in the same context
			Compartment = 1,         ///< Objects share address space, but not thread
			InterProcess = 2,      ///< Objects share user id, but not address space
			InterUser = 3,         ///< Objects share machine, but not user id or address space
			RemoteMachine = 4      ///< Objects on separate machines and share nothing
		};
		typedef uint16_t MarshalFlags_t;

		interface IChannel : public IObject
		{
			virtual IMessage* CreateMessage() = 0;
			virtual IException* SendAndReceive(TypeInfo::MethodAttributes_t attribs, IMessage* pSend, IMessage*& pRecv, uint32_t millisecs) = 0;
			virtual MarshalFlags_t GetMarshalFlags() = 0;
			virtual uint32_t GetSource() = 0;
			virtual bool_t IsConnected() = 0;
			virtual guid_t GetReflectUnmarshalFactoryOID() = 0;
			virtual void ReflectMarshal(IMessage* pMessage) = 0;
			virtual void GetManager(const guid_t& iid, IObject*& pObject) = 0;
		};

		interface ICallContext : public IObject
		{
			virtual uint32_t Timeout() = 0;
			virtual bool_t HasTimedOut() = 0;
			virtual uint32_t SourceId() = 0;
			virtual MarshalFlags_t SourceType() = 0;
		};

		ICallContext* GetCallContext();
		bool_t IsAlive(IObject* pObject);
		uint32_t GetSource(IObject* pObject);
		IProxy* GetProxy(IObject* pObject);

		interface IObjectManager : public IObject
		{
			virtual void Connect(IChannel* pChannel) = 0;
			virtual IMessage* Invoke(IMessage* pParamsIn, uint32_t millisecs) = 0;
			virtual void Shutdown() = 0;
			virtual void GetRemoteInstance(const Omega::any_t& oid, Activation::Flags_t flags, const guid_t& iid, IObject*& pObject) = 0;
			virtual TypeInfo::IInterfaceInfo* GetInterfaceInfo(const guid_t& iid) = 0;
		};

		interface IChannelClosedException : public IException
		{
			static IChannelClosedException* Create(IException* pCause = 0);
		};

		interface IMarshal : public IObject
		{
			virtual guid_t GetUnmarshalFactoryOID(const guid_t& iid, MarshalFlags_t flags) = 0;
			virtual void MarshalInterface(IMarshaller* pMarshaller, IMessage* pMessage, const guid_t& iid, MarshalFlags_t flags) = 0;
			virtual void ReleaseMarshalData(IMarshaller* pMarshaller, IMessage* pMessage, const guid_t& iid, MarshalFlags_t flags) = 0;
		};

		interface IMarshalFactory : public IObject
		{
			virtual void UnmarshalInterface(IMarshaller* pMarshaller, IMessage* pMessage, const guid_t& iid, MarshalFlags_t flags, IObject*& pObject) = 0;
		};

		interface IChannelSink : public IObject
		{
			virtual void Send(TypeInfo::MethodAttributes_t attribs, IMessage* pMsg, uint32_t millisecs) = 0;
			virtual void Close() = 0;

			static IChannelSink* OpenServerSink(const guid_t& message_oid, IChannelSink* pSink);
		};

		interface IEndpoint : public IObject
		{
			virtual string_t Canonicalise(const string_t& strEndpoint) = 0;
			virtual guid_t MessageOid() = 0;
			virtual IChannelSink* Open(const string_t& strEndpoint, IChannelSink* pSink) = 0;
		};
	}
}

#if !defined(DOXYGEN)

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Omega::Remoting, IChannel, "{F18430B0-8AC5-4b57-9B66-56B3BE867C24}",

	OMEGA_METHOD(Remoting::IMessage*,CreateMessage,0,())
	OMEGA_METHOD(IException*,SendAndReceive,4,((in),TypeInfo::MethodAttributes_t,attribs,(in),Remoting::IMessage*,pSend,(out),Remoting::IMessage*&,pRecv,(in),uint32_t,millisecs))
	OMEGA_METHOD(Remoting::MarshalFlags_t,GetMarshalFlags,0,())
	OMEGA_METHOD(uint32_t,GetSource,0,())
	OMEGA_METHOD(bool_t,IsConnected,0,())
	OMEGA_METHOD(guid_t,GetReflectUnmarshalFactoryOID,0,())
	OMEGA_METHOD_VOID(ReflectMarshal,1,((in),Remoting::IMessage*,pMessage))
	OMEGA_METHOD_VOID(GetManager,2,((in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
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

	OMEGA_METHOD_VOID(Connect,1,((in),Remoting::IChannel*,pChannel))
	OMEGA_METHOD(Remoting::IMessage*,Invoke,2,((in),Remoting::IMessage*,pParamsIn,(in),uint32_t,timeout))
	OMEGA_METHOD_VOID(Shutdown,0,())
	OMEGA_METHOD_VOID(GetRemoteInstance,4,((in),const any_t&,oid,(in),Activation::Flags_t,flags,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
	OMEGA_METHOD(TypeInfo::IInterfaceInfo*,GetInterfaceInfo,1,((in),const guid_t&,iid))
)

OMEGA_DEFINE_INTERFACE_DERIVED_LOCAL
(
	Omega::Remoting, IChannelClosedException, Omega, IException, "{E0BB01D1-CF43-4da0-97E0-E40B66A2CFE7}",

	OMEGA_NO_METHODS()
)

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Omega::Remoting, IMarshal, "{5EE81A3F-88AA-47ee-9CAA-CECC8BE8F4C4}",

	OMEGA_METHOD(guid_t,GetUnmarshalFactoryOID,2,((in),const guid_t&,iid,(in),Remoting::MarshalFlags_t,flags))
	OMEGA_METHOD_VOID(MarshalInterface,4,((in),Remoting::IMarshaller*,pMarshaller,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(in),Remoting::MarshalFlags_t,flags))
	OMEGA_METHOD_VOID(ReleaseMarshalData,4,((in),Remoting::IMarshaller*,pMarshaller,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(in),Remoting::MarshalFlags_t,flags))
)

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Omega::Remoting, IMarshalFactory, "{68C779B3-72E7-4c09-92F0-118A01AF224D}",

	OMEGA_METHOD_VOID(UnmarshalInterface,5,((in),Remoting::IMarshaller*,pMarshaller,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(in),Remoting::MarshalFlags_t,flags,(out)(iid_is(iid)),IObject*&,pObject))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Remoting, IChannelSink, "{C395066A-05D1-45f2-95C5-272319CF1394}",

	OMEGA_METHOD_EX_VOID(TypeInfo::Asynchronous,0,Send,3,((in),TypeInfo::MethodAttributes_t,attribs,(in),Remoting::IMessage*,pMsg,(in),uint32_t,millisecs))
	OMEGA_METHOD_VOID(Close,0,())
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Remoting, IEndpoint, "{5194AF93-D1C2-4bfa-AA12-8E6CDCF6CDBA}",

	OMEGA_METHOD(string_t,Canonicalise,1,((in),const string_t&,strEndpoint))
	OMEGA_METHOD(guid_t,MessageOid,0,())
	OMEGA_METHOD(Remoting::IChannelSink*,Open,2,((in),const string_t&,strEndpoint,(in),Remoting::IChannelSink*,pSink))
)

OOCORE_EXPORTED_FUNCTION(Omega::Remoting::IProxy*,OOCore_Remoting_GetProxy,1,((in),Omega::IObject*,pObject));
inline Omega::Remoting::IProxy* Omega::Remoting::GetProxy(IObject* pObject)
{
	return OOCore_Remoting_GetProxy(pObject);
}

OOCORE_EXPORTED_FUNCTION(Omega::Remoting::IChannelClosedException*,OOCore_Remoting_IChannelClosedException_Create,1,((in),Omega::IException*,pCause))
inline Omega::Remoting::IChannelClosedException* Omega::Remoting::IChannelClosedException::Create(Omega::IException* pCause)
{
	return OOCore_Remoting_IChannelClosedException_Create(pCause);
}

OOCORE_EXPORTED_FUNCTION(Omega::Remoting::ICallContext*,OOCore_Remoting_GetCallContext,0,())
inline Omega::Remoting::ICallContext* Omega::Remoting::GetCallContext()
{
	return OOCore_Remoting_GetCallContext();
}

OOCORE_EXPORTED_FUNCTION(Omega::bool_t,OOCore_Remoting_IsAlive,1,((in),Omega::IObject*,pObject))
inline Omega::bool_t Omega::Remoting::IsAlive(Omega::IObject* pObject)
{
	return OOCore_Remoting_IsAlive(pObject);
}

OOCORE_EXPORTED_FUNCTION(Omega::uint32_t,OOCore_Remoting_GetSource,1,((in),Omega::IObject*,pObject))
inline Omega::uint32_t Omega::Remoting::GetSource(Omega::IObject* pObject)
{
	return OOCore_Remoting_GetSource(pObject);
}

OOCORE_EXPORTED_FUNCTION(Omega::Remoting::IChannelSink*,OOCore_Remoting_OpenServerSink,2,((in),const Omega::guid_t&,message_oid,(in),Omega::Remoting::IChannelSink*,pSink))
inline Omega::Remoting::IChannelSink* Omega::Remoting::IChannelSink::OpenServerSink(const guid_t& message_oid, Remoting::IChannelSink* pSink)
{
	return OOCore_Remoting_OpenServerSink(message_oid,pSink);
}

#endif // !defined(DOXYGEN)

#endif // OMEGA_REMOTING_H_INCLUDED_
