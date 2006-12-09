/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary and do not use precompiled headers
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_ROOT_CONNECTION_H_INCLUDED_
#define OOSERVER_ROOT_CONNECTION_H_INCLUDED_

#define ACE_HAS_VERSIONED_NAMESPACE  1
#define ACE_AS_STATIC_LIBS

#include <ace/Acceptor.h>
#include <ace/SOCK_Stream.h>
#include <ace/SOCK_Acceptor.h>

class RootManager;

class RootConnection : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>
{		
public:
	RootConnection() : ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>()
	{}

	int open(void* pAcceptor);

	int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE);
	int handle_output(ACE_HANDLE fd = ACE_INVALID_HANDLE);
	int handle_close(ACE_HANDLE fd, ACE_Reactor_Mask mask);	

private:
	RootConnection(const RootConnection&) {}
	RootConnection& operator = (const RootConnection&) {}

	RootManager*	m_pManager;
};

#endif // OOSERVER_ROOTt_CONNECTION_H_INCLUDED_
