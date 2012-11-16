///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2012 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOServer_Host.h"

using namespace Omega;
using namespace OTL;

#if !defined(ECONNREFUSED)
#define ECONNREFUSED ENOENT
#endif

void Host::StartService(System::IService* pService, const string_t& strName, const string_t& strPipe, Registry::IKey* pKey, const string_t& strSecret)
{
	OOBase::LocalString strPipe2;
	int err = strPipe2.assign(strPipe.c_str());
	if (err)
		OMEGA_THROW(err);

	if (strPipe2[0] == ' ')
		strPipe2.replace_at(0,'\0');

	// Connect up to the root process...
#if defined(NDEBUG)
	OOBase::Timeout timeout(15,0);
#else
	OOBase::Timeout timeout;
#endif

	OOBase::RefPtr<OOBase::Socket> ptrSocket;
	while (!timeout.has_expired())
	{
		ptrSocket = OOBase::Socket::connect_local(strPipe2.c_str(),err,timeout);
		if (!err || (err != ENOENT && err != ECONNREFUSED))
			break;

		// We ignore the error, and try again until we timeout
	}
	if (err)
		OMEGA_THROW(err);

	// Now send the secret back to the root, to authenticate ourselves, and signal we have started
	ptrSocket->send(strSecret.c_str(),strSecret.Length(),err);
	if (err)
		OMEGA_THROW(err);

#if defined(_WIN32)
	// Send our pid, so we can unmarshal socket handles
	DWORD pid = GetCurrentProcessId();
	ptrSocket->send(&pid,sizeof(pid),err);
	if (err)
		OMEGA_THROW(err);
#endif

	System::IService::socket_map_t socket_map;

	try
	{
		// Now loop reading socket handles from ptrSocket
		OOBase::StackAllocator<256> allocator;
		OOBase::TempPtr<char> ptrName(allocator);
		for (;;)
		{
			uint32_t len = 0;
			err = ptrSocket->recv(len);
			if (err)
				OMEGA_THROW(err);

			if (!len)
				break;

			if (!ptrName.reallocate(len))
				OMEGA_THROW(ERROR_OUTOFMEMORY);

			ptrSocket->recv(ptrName,len,true,err);
			if (err)
				OMEGA_THROW(err);

			OOBase::socket_t sock;
			err = ptrSocket->recv_socket(sock);
			if (err)
				OMEGA_THROW(err);

			socket_map.insert(System::IService::socket_map_t::value_type(string_t(ptrName,len),sock));
		}

		// Now run the service (this need not return for some time)
		pService->Run(strName,pKey,socket_map);

		// Close all remaining sockets
		for (System::IService::socket_map_t::iterator i=socket_map.begin();i != socket_map.end(); ++i)
			OOBase::Net::close_socket(i->second);
	}
	catch (...)
	{
		// Close all remaining sockets
		for (System::IService::socket_map_t::iterator i=socket_map.begin();i != socket_map.end(); ++i)
			OOBase::Net::close_socket(i->second);

		throw;
	}
}
