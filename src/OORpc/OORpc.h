///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OORpc, the Omega Online RPC library.
//
// OORpc is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OORpc is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OORpc.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include <OOCore/OOCore.h>

/////////////////////////////////////////////////
// Include ACE components

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

#include <ace/Codecs.h>
#include <ace/Condition_Thread_Mutex.h>
#include <ace/Event.h>
#include <ace/Init_ACE.h>
#include <ace/Message_Queue.h>
#include <ace/OS.h>
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/RW_Thread_Mutex.h>

#if !defined(ACE_HAS_WCHAR)
#error Omega Online requires ACE_HAS_WCHAR support!
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

#include <set>
#include <sstream>

// End of STL includes
//////////////////////////////////////////////

/////////////////////////////////////////////////
// Include OOCore/OTL components

#include <OOCore/Remoting.h>
#include <OTL/OTL.h>

// End of OOCore/OTL includes
/////////////////////////////////////////////////

#define OORPC_GUARD(MUTEX,OBJ,LOCK) \
	ACE_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OMEGA_THROW(ACE_OS::last_error());

#define OORPC_READ_GUARD(MUTEX,OBJ,LOCK) \
	ACE_Read_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OMEGA_THROW(ACE_OS::last_error());

#define OORPC_WRITE_GUARD(MUTEX,OBJ,LOCK) \
	ACE_Write_Guard< MUTEX > OBJ (LOCK); \
	if (OBJ.locked () == 0) OMEGA_THROW(ACE_OS::last_error());
