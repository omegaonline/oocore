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
// Version defines

#if defined(ACE_NLOGGING)
#error You must not define ACE_NLOGGING, cos we use it!
#endif

#if defined(WIN32)
	#if !defined(_WIN32_WINNT)
	#define _WIN32_WINNT 0x0500
	#elif _WIN32_WINNT < 0x0500
	#error OOServer requires _WIN32_WINNT >= 0x0500!
	#endif

	#ifndef WINVER
	#define WINVER _WIN32_WINNT
	#endif

	#if !defined(_WIN32_IE)
	#define _WIN32_IE 0x0500
	#elif _WIN32_IE < 0x0500
	#error OOCore requires _WIN32_IE >= 0x0500!
	#endif
#endif // WIN32

#define ACE_AS_STATIC_LIBS 1

#include <ace/ARGV.h>
#include <ace/Asynch_Acceptor.h>
#include <ace/CDR_Stream.h>
#include <ace/Condition_Thread_Mutex.h>
#include <ace/Configuration.h>
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

#include <list>
#include <map>
#include <set>

#if defined(ACE_NLOGGING)
#error You must not define ACE_NLOGGING, cos we use it!
#endif

#if defined(ACE_WIN32)
// For the Windows path functions
#include <shlwapi.h>
#include <shlobj.h>
#endif

namespace Root
{
	struct ConfigState
	{
		bool		bNoSandbox;
		bool		bAlternateSpawn;
	};

	extern ConfigState s_config_state;
}

#endif // OOSERVER_ROOT_H_INCLUDED_
