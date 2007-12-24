///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
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

#ifndef OOCORE_BUILD_LIBRARY
#define OOCORE_BUILD_LIBRARY
#endif

// Pre-include config...
#include <OOCore/config-guess.h>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#pragma warning(disable : 4355)
#endif

/////////////////////////////////////////////////
// Include ACE components

#include <ace/CDR_Stream.h>
#include <ace/Codecs.h>
#include <ace/Countdown_Time.h>
#include <ace/DLL.h>
#include <ace/DLL_Manager.h>
#include <ace/Encoding_Converter.h>
#include <ace/Encoding_Converter_Factory.h>
#include <ace/SOCK_Connector.h>
#include <ace/Message_Queue.h>
#include <ace/NT_Service.h>
#include <ace/OS.h>
#include <ace/Proactor.h>
#include <ace/Process.h>
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/RW_Thread_Mutex.h>
#include <ace/Singleton.h>
#include <ace/SString.h>
#include <ace/TSS_T.h>
#include <ace/UUID.h>

#include <ace/SPIPE_Connector.h>
#include <ace/UNIX_Addr.h>

#if !defined(ACE_HAS_WCHAR)
#error OmegaOnline requires has wchar_t support!
#endif

#if !defined(ACE_USES_WCHAR)
#error OmegaOnline requires uses wchar_t support!
#endif

// End of ACE includes
/////////////////////////////////////////////////

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

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

#include <OOCore/Remoting.h>
#include <OTL/OTL.h>

// End of OOCore/OTL includes
/////////////////////////////////////////////////

/////////////////////////////////////////////////
// Include Windows components
#ifdef OMEGA_WIN32

// We use the unicode CRT
#define _UNICODE
#include <shlobj.h>
#include <shlwapi.h>

#endif
// End of Windows includes
/////////////////////////////////////////////////

#define OOCORE_THROW_ERRNO(error) \
	OMEGA_THROW(Omega::string_t(ACE_OS::strerror(error),false))

#define OOCORE_THROW_LASTERROR() \
	OOCORE_THROW_ERRNO(ACE_OS::last_error())

#define OOCORE_GUARD(MUTEX, OBJ, LOCK) \
	ACE_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OOCORE_THROW_LASTERROR();

#define OOCORE_READ_GUARD(MUTEX,OBJ,LOCK) \
	ACE_Read_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OOCORE_THROW_LASTERROR();

#define OOCORE_WRITE_GUARD(MUTEX,OBJ,LOCK) \
	ACE_Write_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OOCORE_THROW_LASTERROR();

#endif // OOCORE_LOCAL_MACROS_H_INCLUDED_

