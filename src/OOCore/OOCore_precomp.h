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

// Pre-include config...
#include "../Common/config-build.h"

/////////////////////////////////////////////////
// Include ACE components

#if defined(OMEGA_WIN32) && !defined(WIN32)
#define WIN32
#endif

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4355) // 'this' : used in base member initializer list
#if (_MSC_VER == 1310)
#pragma warning(disable : 4244) // 'argument' : conversion from 't1' to 't2', possible loss of data
#endif
#if (_MSC_VER >= 1400)
#pragma warning(disable : 4996) // 'function' was declared deprecated
#endif
#endif

#include <ace/CDR_Stream.h>
#include <ace/Countdown_Time.h>
#include <ace/DLL.h>
#include <ace/DLL_Manager.h>
#include <ace/Event.h>
#include <ace/Message_Queue.h>
#include <ace/NT_Service.h>
#include <ace/OS.h>
#include <ace/Proactor.h>
#include <ace/Process.h>
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/RW_Thread_Mutex.h>
#include <ace/Singleton.h>
#include <ace/SOCK_Connector.h>
#include <ace/SString.h>
#include <ace/TSS_T.h>
#include <ace/UUID.h>

#include <ace/SPIPE_Connector.h>
#include <ace/UNIX_Addr.h>

#if !defined(ACE_HAS_WCHAR)
#error Omega Online requires has wchar_t support!
#endif

#if defined(ACE_WIN32)
#if ((defined(UNICODE) || defined(_UNICODE)) && !defined(ACE_USES_WCHAR)) || (!defined(UNICODE) && !defined(_UNICODE) && defined(ACE_USES_WCHAR))
#error You cannot mix and match UNICODE and ACE_USES_WCHAR!
#endif
#endif

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

// End of ACE includes
/////////////////////////////////////////////////

//////////////////////////////////////////////
// Set up the export macros for OOCORE
#define OMEGA_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	OMEGA_LOCAL_FUNCTION_VOID(name,param_count,params)

#define OMEGA_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	OMEGA_LOCAL_FUNCTION(ret_type,name,param_count,params)

#define OMEGA_DECLARE_OID(n) \
	OMEGA_EXPORT_OID(n)

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
// Include Windows components

#ifdef OMEGA_WIN32
#include <shlobj.h>
#include <shlwapi.h>
#endif

// End of Windows includes
/////////////////////////////////////////////////

#define OOCORE_GUARD(MUTEX, OBJ, LOCK) \
	ACE_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OMEGA_THROW(ACE_OS::last_error());

#define OOCORE_READ_GUARD(MUTEX,OBJ,LOCK) \
	ACE_Read_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OMEGA_THROW(ACE_OS::last_error());

#define OOCORE_WRITE_GUARD(MUTEX,OBJ,LOCK) \
	ACE_Write_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OMEGA_THROW(ACE_OS::last_error());

namespace OOCore
{
	OTL::ObjectPtr<Omega::System::IInterProcessService> GetInterProcessService();
	bool HostedByOOServer();

	// Some helpers
	inline Omega::bool_t ReadBoolean(const wchar_t* name, Omega::Remoting::IMessage* pMsg)
	{
		Omega::bool_t val;
		if (pMsg->ReadBooleans(name,1,&val) != 1)
			OMEGA_THROW(EIO);
		return val;
	}

	inline Omega::byte_t ReadByte(const wchar_t* name, Omega::Remoting::IMessage* pMsg)
	{
		Omega::byte_t val;
		if (pMsg->ReadBytes(name,1,&val) != 1)
			OMEGA_THROW(EIO);
		return val;
	}

	inline Omega::uint16_t ReadUInt16(const wchar_t* name, Omega::Remoting::IMessage* pMsg)
	{
		Omega::uint16_t val;
		if (pMsg->ReadUInt16s(name,1,&val) != 1)
			OMEGA_THROW(EIO);
		return val;
	}

	inline Omega::uint32_t ReadUInt32(const wchar_t* name, Omega::Remoting::IMessage* pMsg)
	{
		Omega::uint32_t val;
		if (pMsg->ReadUInt32s(name,1,&val) != 1)
			OMEGA_THROW(EIO);
		return val;
	}

	inline Omega::guid_t ReadGuid(const wchar_t* name, Omega::Remoting::IMessage* pMsg)
	{
		Omega::guid_t val;
		if (pMsg->ReadGuids(name,1,&val) != 1)
			OMEGA_THROW(EIO);
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
