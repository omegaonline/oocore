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

#ifndef OOCORE_LOCAL_MACROS_H_INCLUDED_
#define OOCORE_LOCAL_MACROS_H_INCLUDED_

#include "../OOBase/Singleton.h"
#include "../OOBase/TLSSingleton.h"
#include "../OOBase/SmartPtr.h"
#include "../OOBase/CDRStream.h"
#include "../OOBase/TimeVal.h"
#include "../OOBase/Thread.h"
#include "../OOBase/Socket.h"
#include "../OOBase/Queue.h"
#include "../OOBase/DLL.h"
#include "../OOBase/Win32.h"

//////////////////////////////////////////////
// Set up the export macros for OOCORE
#define OMEGA_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	OMEGA_LOCAL_FUNCTION_VOID(name,param_count,params)

#define OMEGA_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	OMEGA_LOCAL_FUNCTION(ret_type,name,param_count,params)

#define OMEGA_INTERNAL \
	OMEGA_EXPORT

#define OMEGA_DECLARE_OID(name) \
	OMEGA_EXPORT_OID(name)

/////////////////////////////////////////////////
// Include OOCore/OTL components

#include <OOCore/OOCore.h>
#include <OOCore/Remoting.h>
#include <OOCore/Apartment.h>
#include <OTL/OTL.h>
#include "../Common/Server.h"

#include "../Common/Version.h"

// End of OOCore/OTL includes
/////////////////////////////////////////////////

/////////////////////////////////////////////////
// Some helpers and globals

namespace OOCore
{
	OTL::ObjectPtr<Omega::System::IInterProcessService> GetInterProcessService();
	bool HostedByOOServer();

	inline Omega::bool_t ReadBoolean(const wchar_t* name, Omega::Remoting::IMessage* pMsg)
	{
		Omega::bool_t val;
		if (pMsg->ReadBooleans(name,1,&val) != 1)
			OMEGA_THROW(L"Unexpected end of message");
		return val;
	}

	inline Omega::byte_t ReadByte(const wchar_t* name, Omega::Remoting::IMessage* pMsg)
	{
		Omega::byte_t val;
		if (pMsg->ReadBytes(name,1,&val) != 1)
			OMEGA_THROW(L"Unexpected end of message");
		return val;
	}

	inline Omega::uint16_t ReadUInt16(const wchar_t* name, Omega::Remoting::IMessage* pMsg)
	{
		Omega::uint16_t val;
		if (pMsg->ReadUInt16s(name,1,&val) != 1)
			OMEGA_THROW(L"Unexpected end of message");
		return val;
	}

	inline Omega::uint32_t ReadUInt32(const wchar_t* name, Omega::Remoting::IMessage* pMsg)
	{
		Omega::uint32_t val;
		if (pMsg->ReadUInt32s(name,1,&val) != 1)
			OMEGA_THROW(L"Unexpected end of message");
		return val;
	}

	inline Omega::guid_t ReadGuid(const wchar_t* name, Omega::Remoting::IMessage* pMsg)
	{
		Omega::guid_t val;
		if (pMsg->ReadGuids(name,1,&val) != 1)
			OMEGA_THROW(L"Unexpected end of message");
		return val;
	}

	inline void WriteBoolean(const wchar_t* name, Omega::Remoting::IMessage* pMsg, Omega::bool_t val)
	{
		pMsg->WriteBooleans(name,1,&val);
	}

	inline void WriteByte(const wchar_t* name, Omega::Remoting::IMessage* pMsg, Omega::byte_t val)
	{
		pMsg->WriteBytes(name,1,&val);
	}

	inline void WriteUInt16(const wchar_t* name, Omega::Remoting::IMessage* pMsg, Omega::uint16_t val)
	{
		pMsg->WriteUInt16s(name,1,&val);
	}

	inline void WriteUInt32(const wchar_t* name, Omega::Remoting::IMessage* pMsg, Omega::uint32_t val)
	{
		pMsg->WriteUInt32s(name,1,&val);
	}

	inline void WriteGuid(const wchar_t* name, Omega::Remoting::IMessage* pMsg, Omega::guid_t val)
	{
		pMsg->WriteGuids(name,1,&val);
	}
}

#endif // OOCORE_LOCAL_MACROS_H_INCLUDED_
