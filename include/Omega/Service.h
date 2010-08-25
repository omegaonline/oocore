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

#ifndef OOCORE_SERVICE_H_INCLUDED_
#define OOCORE_SERVICE_H_INCLUDED_

#include "Omega.h"

namespace Omega
{
	namespace System
	{
		interface IService : public IObject
		{
			virtual void Start(Registry::IKey* pKey) = 0;
			virtual void Stop() = 0;
		};
	}

	namespace Net
	{
		interface IAsyncSocketBase : public IObject
		{
			virtual void Recv(uint32_t lenBytes, bool_t bRecvAll) = 0;
			virtual void Send(uint32_t lenBytes, const byte_t* bytes, bool_t bReliable = true) = 0;
		};

		interface IAsyncSocketNotify : public IObject
		{
			virtual void OnRecv(IAsyncSocketBase* pSocket, uint32_t lenBytes, const byte_t* bytes, IException* pError) = 0;
			virtual void OnSent(IAsyncSocketBase* pSocket, uint32_t lenBytes, const byte_t* bytes, IException* pError) = 0;
			virtual void OnClose(IAsyncSocketBase* pSocket) = 0;
		};

		interface IAsyncSocket : public IAsyncSocketBase
		{
			enum BindFlags
			{
				Default = 0,
				IncludeSentData = 1,
				IncludeSocket = 2,
			};
			typedef uint16_t BindFlags_t;

			virtual IAsyncSocketNotify* Bind(IAsyncSocketNotify* pNotify, BindFlags_t flags = IAsyncSocket::Default) = 0;
		};
	}

	namespace System
	{
		interface INetworkService : public IService
		{
			virtual void OnAccept(Net::IAsyncSocket* pSocket) = 0;
		};
	}
}

OMEGA_DEFINE_INTERFACE
(
	Omega::System, IService, "{C5BA215C-C59C-4471-96F8-9169F122150A}",

	OMEGA_METHOD_VOID(Start,1,((in),Registry::IKey*,pKey))
	OMEGA_METHOD_VOID(Stop,0,())
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Net, IAsyncSocketBase, "{3651E79D-06FF-4E3B-BCCD-90B548452C8E}",

	OMEGA_METHOD_EX_VOID(TypeInfo::Asynchronous,0,Recv,2,((in),uint32_t,lenBytes,(in),bool_t,bRecvAll))
	OMEGA_METHOD_EX_VOID(TypeInfo::Asynchronous,0,Send,3,((in),uint32_t,lenBytes,(in)(size_is(lenBytes)),const byte_t*,bytes,(in),bool_t,bReliable))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Net, IAsyncSocketNotify, "{2FA8B1D5-06D4-477D-9DB4-6595C102E0C5}",

	OMEGA_METHOD_EX_VOID(TypeInfo::Asynchronous,0,OnRecv,4,((in),Net::IAsyncSocketBase*,pSocket,(in),uint32_t,lenBytes,(in)(size_is(lenBytes)),const byte_t*,bytes,(in),IException*,pError))
	OMEGA_METHOD_EX_VOID(TypeInfo::Asynchronous,0,OnSent,4,((in),Net::IAsyncSocketBase*,pSocket,(in),uint32_t,lenBytes,(in)(size_is(lenBytes)),const byte_t*,bytes,(in),IException*,pError))
	OMEGA_METHOD_EX_VOID(TypeInfo::Asynchronous,0,OnClose,1,((in),Net::IAsyncSocketBase*,pSocket))	
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::Net, IAsyncSocket, Omega::Net, IAsyncSocketBase, "{B77B01F1-19FD-4c5e-8E94-01CD72BF3885}",

	OMEGA_METHOD(Net::IAsyncSocketNotify*,Bind,2,((in),Net::IAsyncSocketNotify*,pNotify,(in),Net::IAsyncSocket::BindFlags_t,flags))
)

OMEGA_DEFINE_INTERFACE_DERIVED
(
	Omega::System, INetworkService, Omega::System, IService, "{443C61D0-BA08-41AC-B2B5-E1A3DCEE60BE}",

	OMEGA_METHOD_VOID(OnAccept,1,((in),Net::IAsyncSocket*,pSocket))
)

#endif // OOCORE_SERVICE_H_INCLUDED_
