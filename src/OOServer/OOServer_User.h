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

#if defined(HAVE_CONFIG_H)
// Autoconf
#include "config-autoconf.h"
#endif

#include <OOCore/config-guess.h>

/////////////////////////////////////////////////
// Include ACE components

#if defined(OMEGA_WIN32) && !defined(WIN32)
#define WIN32
#endif

#if defined(_MSC_VER)
#pragma warning(push)
#ifndef _DEBUG
// Optimization sometimes re-orders things causing this error
#pragma warning(disable : 4702)
#endif
#if (_MSC_VER == 1310)
#pragma warning(disable : 4244) // 'argument' : conversion from 't1' to 't2', possible loss of data
#endif
#if (_MSC_VER >= 1400)
#pragma warning(disable : 4996) // 'function' was declared deprecated
#endif
#endif

#include <ace/Asynch_Connector.h>
#include <ace/CDR_Stream.h>
#include <ace/Connector.h>
#include <ace/Countdown_Time.h>
#include <ace/Event.h>
#include <ace/Get_Opt.h>
#include <ace/Message_Queue.h>
#include <ace/OS.h>
#include <ace/Proactor.h>
#include <ace/Process.h>
#include <ace/SOCK_Connector.h>

#include <ace/SOCK_Acceptor.h>
#include <ace/SPIPE_Acceptor.h>
#include <ace/SPIPE_Connector.h>
#include <ace/UNIX_Addr.h>

#if !defined(ACE_HAS_WCHAR)
#error Omega Online requires ACE_HAS_WCHAR support!
#endif

#if defined(ACE_WIN32)
#if ((defined(UNICODE) || defined(_UNICODE)) && !defined(ACE_USES_WCHAR)) || (!defined(UNICODE) && !defined(_UNICODE) && defined(ACE_USES_WCHAR))
#error You cannot mix and match UNICODE and ACE_USES_WCHAR!
#endif
#endif

// Define a macro to hold the ACE version
#define OMEGA_ACE_VERSION(x,y,z) \
	((x * 10000) + (y * 100) + z)

#define OMEGA_ACE_VERSION_CURRENT() OMEGA_ACE_VERSION(ACE_MAJOR_VERSION,ACE_MINOR_VERSION,ACE_BETA_VERSION)

#if defined(_MSC_VER)
#pragma warning(pop)

#endif

// End of ACE includes
/////////////////////////////////////////////////

//////////////////////////////////////////////
// Include STL components

#include <set>
#include <sstream>

// End of STL includes
//////////////////////////////////////////////

/////////////////////////////////////////////////
// Include OOCore/OTL components

#include <OOCore/Remoting.h>
#include <OOCore/Service.h>
#include <OTL/OTL.h>

// End of OOCore/OTL includes
/////////////////////////////////////////////////

/////////////////////////////////////////////////
// Include Windows components
#if defined(OMEGA_WIN32)

#include <shlobj.h>
#include <shlwapi.h>

#endif
// End of Windows includes
/////////////////////////////////////////////////

#define OOSERVER_GUARD(MUTEX,OBJ,LOCK) \
	ACE_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OMEGA_THROW(ACE_OS::last_error());

#define OOSERVER_READ_GUARD(MUTEX,OBJ,LOCK) \
	ACE_Read_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OMEGA_THROW(ACE_OS::last_error());

#define OOSERVER_WRITE_GUARD(MUTEX,OBJ,LOCK) \
	ACE_Write_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OMEGA_THROW(ACE_OS::last_error());

#ifdef OMEGA_DEBUG
void AttachDebugger(pid_t pid);
#endif

#if !defined(OMEGA_WIN32)
int IsDebuggerPresent();
#endif
