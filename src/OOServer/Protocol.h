///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
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

#ifndef OOSERVER_PROTOCOL_H_INCLUDED_
#define OOSERVER_PROTOCOL_H_INCLUDED_

namespace OOServer
{
	enum RootOpCode
	{
		// Root -> User
		OnSocketAccept = 1,
		OnSocketRecv,
		OnSocketSent,
		OnSocketClose,

		// User -> Root

		// Registry opcodes
		KeyExists = 0x80,
		CreateKey,
		DeleteKey,
		EnumSubKeys,
		ValueExists,
		GetValue,
		SetValue,
		EnumValues,
		DeleteValue,
		OpenMirrorKey,

		// Service opcodes
		ServicesStart = 0xa0,
		GetServiceKey,
		ListenSocket,
		SocketRecv,
		SocketSend,
		SocketClose
	};
	typedef Omega::uint16_t RootOpCode_t;
}

#endif // OOSERVER_PROTOCOL_H_INCLUDED_
