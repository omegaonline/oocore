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

/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_ROOT_H_INCLUDED_
#define OOSERVER_ROOT_H_INCLUDED_

//////////////////////////////////////////////
// Bring in the config guesser

#include <OOCore/config-guess.h>

/////////////////////////////////////////////////
// Include ACE components

#if defined(_MSC_VER)
#pragma warning(push)
#ifndef _DEBUG
// Optimization sometimes re-orders things causing this error
#pragma warning(disable : 4702)
#endif
#if _MSC_VER >= 1400
#pragma warning(disable : 4996) // 'function' was declared deprecated 
#endif
#endif

#include <ace/ARGV.h>
#include <ace/Asynch_Acceptor.h>
#include <ace/CDR_Stream.h>
#include <ace/Condition_Thread_Mutex.h>
#include <ace/Countdown_Time.h>
#include <ace/Event.h>
#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/Message_Queue.h>
#include <ace/NT_Service.h>
#include <ace/OS.h>
#include <ace/Proactor.h>
#include <ace/Singleton.h>
#include <ace/SOCK_Acceptor.h>
#include <ace/SOCK_Connector.h>

#include <ace/Reactor.h>
#include <ace/SPIPE_Acceptor.h>
#include <ace/SPIPE_Connector.h>
#include <ace/UNIX_Addr.h>

#if defined(ACE_NLOGGING)
#error You must not define ACE_NLOGGING, we use it!
#endif

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
// Include STL components

#include <list>
#include <map>
#include <set>

// End of STL includes
//////////////////////////////////////////////

#if defined(OMEGA_WIN32)

// For the Windows path functions
#include <shlwapi.h>
#include <shlobj.h>

#endif

#if defined(OMEGA_WIN32)
	typedef HANDLE user_id_type;
#else
	typedef uid_t user_id_type;
#endif

#ifdef OMEGA_DEBUG
void AttachDebugger(pid_t pid);
#endif

#if !defined(OMEGA_WIN32)
bool IsDebuggerPresent();
#endif

#endif // OOSERVER_ROOT_H_INCLUDED_
