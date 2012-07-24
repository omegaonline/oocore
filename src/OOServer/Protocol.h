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

		// Root -> Sandbox
		Root_StartService = 0x40,
		Root_StopAllServices,

		// User -> Root
		User_NotifyStarted = 0x80,
		User_GetConfigArg,

		Service_Start = 0x90,
		Service_Stop,
		Service_IsRunning,
		Service_ListRunning,

		Registry_OpenKey = 0xA0,
		Registry_DeleteSubKey,
		Registry_EnumSubKeys,
		Registry_ValueExists,
		Registry_GetValue,
		Registry_SetValue,
		Registry_EnumValues,
		Registry_DeleteValue
	};
	typedef Omega::uint16_t RootOpCode_t;

	enum RootErrCode
	{
		// These must match Db::hive_errors
		Ok = 0,
		Errored,
		NotFound,
		AlreadyExists,
		ReadOnlyHive,
		NoRead,
		NoWrite,
		ProtectedKey,
		BadName,
		Linked
	};
	typedef Omega::uint16_t RootErrCode_t;
}

#endif // OOSERVER_PROTOCOL_H_INCLUDED_
