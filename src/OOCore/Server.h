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
	namespace Remoting
	{
		interface IInterProcessService : public IObject
		{
			virtual Registry::IKey* GetRegistry() = 0;
			virtual Activation::IRunningObjectTable* GetRunningObjectTable() = 0;
			virtual void GetObject(const string_t& strProcess, bool_t bSandbox, const guid_t& oid, const guid_t& iid, IObject*& pObject) = 0;
			virtual IO::IStream* OpenStream(const string_t& strEndpoint, IO::IAsyncStreamNotify* pNotify) = 0;
			virtual bool_t HandleRequest(uint32_t timeout) = 0;
			virtual void GetRemoteInstance(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid, const string_t& strEndpoint, IObject*& pObject) = 0;
			virtual IChannelSink* OpenServerSink(const guid_t& message_oid, IChannelSink* pSink) = 0;
		};

		// {7E9E22E8-C0B0-43f9-9575-BFB1665CAE4A}
		OMEGA_DECLARE_OID(OID_InterProcessService);
	}
}

OMEGA_DEFINE_INTERFACE
(
	Omega::Remoting, IInterProcessService, "{70F6D098-6E53-4e8d-BF21-9EA359DC4FF8}",

	OMEGA_METHOD(Registry::IKey*,GetRegistry,0,())
	OMEGA_METHOD(Activation::IRunningObjectTable*,GetRunningObjectTable,0,())
	OMEGA_METHOD_VOID(GetObject,5,((in),const string_t&,strProcess,(in),bool_t,bSandbox,(in),const guid_t&,oid,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
	OMEGA_METHOD(IO::IStream*,OpenStream,2,((in),const string_t&,strEndpoint,(in),IO::IAsyncStreamNotify*,pNotify))
	OMEGA_METHOD(bool_t,HandleRequest,1,((in),uint32_t,timeout))
	OMEGA_METHOD_VOID(GetRemoteInstance,5,((in),const guid_t&,oid,(in),Activation::Flags_t,flags,(in),const guid_t&,iid,(in),const string_t&,strEndpoint,(out)(iid_is(iid)),IObject*&,pObject))
	OMEGA_METHOD(Remoting::IChannelSink*,OpenServerSink,2,((in),const guid_t&,message_oid,(in),Remoting::IChannelSink*,pSink))
)

#endif // OOCORE_SERVER_H_INCLUDED_
