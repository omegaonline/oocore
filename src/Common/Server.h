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

namespace Omega
{
	namespace System
	{
		interface IInterProcessService : public IObject
		{
			virtual Registry::IKey* GetRegistry() = 0;
			virtual Activation::IRunningObjectTable* GetRunningObjectTable() = 0;
			virtual void LaunchObjectApp(const guid_t& oid, const guid_t& iid, IObject*& pObject) = 0;
			virtual bool_t HandleRequest(uint32_t timeout) = 0;
			virtual Remoting::IChannel* OpenRemoteChannel(const string_t& strEndpoint) = 0;
			virtual Remoting::IChannelSink* OpenServerSink(const guid_t& message_oid, Remoting::IChannelSink* pSink) = 0;
		};

		// {7E9E22E8-C0B0-43f9-9575-BFB1665CAE4A}
		extern const Omega::guid_t OID_InterProcessService;
	}
}

OMEGA_DEFINE_INTERFACE
(
	Omega::System, IInterProcessService, "{70F6D098-6E53-4e8d-BF21-9EA359DC4FF8}",

	OMEGA_METHOD(Registry::IKey*,GetRegistry,0,())
	OMEGA_METHOD(Activation::IRunningObjectTable*,GetRunningObjectTable,0,())
	OMEGA_METHOD_VOID(LaunchObjectApp,3,((in),const guid_t&,oid,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
	OMEGA_METHOD(bool_t,HandleRequest,1,((in),uint32_t,timeout))
	OMEGA_METHOD(Remoting::IChannel*,OpenRemoteChannel,1,((in),const string_t&,strEndpoint))
	OMEGA_METHOD(Remoting::IChannelSink*,OpenServerSink,2,((in),const guid_t&,message_oid,(in),Remoting::IChannelSink*,pSink))
)

#endif // OOCORE_SERVER_H_INCLUDED_
