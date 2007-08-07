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

	// We use the unicode CRT
	#define _UNICODE

#endif // WIN32

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4127)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#endif

/////////////////////////////////////////////////
// Include ACE components

// Link to the static lib version of ACE...
#define ACE_AS_STATIC_LIBS 1
#define ACE_USES_WCHAR

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

// End of ACE includes
/////////////////////////////////////////////////

//////////////////////////////////////////////
// Include STL components

#include <list>
#include <map>
#include <set>

// End of STL includes
//////////////////////////////////////////////

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if defined(_MSC_VER)

// Warning 4127 is rubbish!
#pragma warning(disable : 4127)

#ifndef _DEBUG
// Optimization sometimes re-orders things causing this error
#pragma warning(disable : 4702)
#endif

#endif

#endif // OOSERVER_ROOT_H_INCLUDED_
