///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
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

#ifndef OOCORE_SERVER_H_INCLUDED_
#define OOCORE_SERVER_H_INCLUDED_

namespace OOCore
{
	interface IInterProcessService : public Omega::IObject
	{
		virtual Omega::Registry::IKey* GetRegistry() = 0;
		virtual Omega::Activation::IRunningObjectTable* GetRunningObjectTable() = 0;
		virtual void LaunchObjectApp(const Omega::guid_t& oid, const Omega::guid_t& iid, Omega::Activation::Flags_t flags, Omega::uint32_t envc, const Omega::string_t* envp, Omega::IObject*& pObject) = 0;
		virtual Omega::bool_t HandleRequest(Omega::uint32_t millisecs) = 0;
		virtual Omega::Remoting::IChannel* OpenRemoteChannel(const Omega::string_t& strEndpoint) = 0;
		virtual Omega::Remoting::IChannelSink* OpenServerSink(const Omega::guid_t& message_oid, Omega::Remoting::IChannelSink* pSink) = 0;
	};

	interface ISurrogate : public Omega::IObject
	{
		virtual void CreateInstance(const Omega::guid_t& oid, const Omega::guid_t& iid, Omega::Activation::Flags_t flags, Omega::IObject*& pObject) = 0;
	};

	// {7E9E22E8-C0B0-43F9-9575-BFB1665CAE4A}
	extern "C" const Omega::guid_t OID_InterProcessService;

	// {D063D32C-FB9A-004A-D2E5-BB5451808FF5}
	extern "C" const Omega::guid_t OID_Surrogate;

	// {22DC1376-4905-D9DD-1B63-2096C487E5A3}
	extern "C" const Omega::guid_t OID_SingleSurrogate;
}

OMEGA_DEFINE_INTERFACE
(
	OOCore, IInterProcessService, "{70F6D098-6E53-4E8D-BF21-9EA359DC4FF8}",

	OMEGA_METHOD(Registry::IKey*,GetRegistry,0,())
	OMEGA_METHOD(Activation::IRunningObjectTable*,GetRunningObjectTable,0,())
	OMEGA_METHOD_VOID(LaunchObjectApp,6,((in),const guid_t&,oid,(in),const guid_t&,iid,(in),Activation::Flags_t,flags,(in),uint32_t,envc,(in)(size_is(envc)),const Omega::string_t*,envp,(out)(iid_is(iid)),IObject*&,pObject))
	OMEGA_METHOD(bool_t,HandleRequest,1,((in),uint32_t,millisecs))
	OMEGA_METHOD(Remoting::IChannel*,OpenRemoteChannel,1,((in),const string_t&,strEndpoint))
	OMEGA_METHOD(Remoting::IChannelSink*,OpenServerSink,2,((in),const guid_t&,message_oid,(in),Remoting::IChannelSink*,pSink))
)

OMEGA_DEFINE_INTERFACE
(
	OOCore, ISurrogate, "{1EE9F564-853C-104F-AC73-EA8E76DCADCE}",

	OMEGA_METHOD_VOID(CreateInstance,4,((in),const guid_t&,oid,(in),const guid_t&,iid,(in),Activation::Flags_t,flags,(out)(iid_is(iid)),IObject*&,pObject))
)

OOCORE_EXPORTED_FUNCTION_VOID(OOCore_ServerInit,0,());
OOCORE_EXPORTED_FUNCTION(Omega::uint32_t,OOCore_RegisterIPS,1,((in),Omega::IObject*,pIPS));
OOCORE_EXPORTED_FUNCTION_VOID(OOCore_RevokeIPS,1,((in),Omega::uint32_t,nCookie));
OOCORE_EXPORTED_FUNCTION_VOID(OOCore_RespondException,2,((in),Omega::Remoting::IMessage*,pMessage,(in),Omega::IException*,pException));
OOCORE_EXPORTED_FUNCTION(Omega::Remoting::IObjectManager*,OOCore_CreateStdObjectManager,0,());

#endif // OOCORE_SERVER_H_INCLUDED_
