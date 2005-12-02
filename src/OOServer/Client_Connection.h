#ifndef OOSERVER_CLIENT_CONNECTION_H_INCLUDED_
#define OOSERVER_CLIENT_CONNECTION_H_INCLUDED_

#include <ace/Acceptor.h>
#include <ace/MEM_Acceptor.h>
#include <ace/MEM_Stream.h>

#include "../OOCore/Transport_Svc_Handler.h"

class Client_Connection : 
	public OOCore::Transport_Svc_Handler<true,ACE_MEM_STREAM,ACE_MEM_STREAM_MIN_BUFFER>
{
	typedef OOCore::Transport_Svc_Handler<true,ACE_MEM_STREAM,ACE_MEM_STREAM_MIN_BUFFER> svc_base;
	typedef ACE_Acceptor<Client_Connection, ACE_MEM_ACCEPTOR> acceptor_type;

public:
	static int init(void);

private:
	ssize_t send_n(ACE_Message_Block* mb);
};

#endif // OOSERVER_CLIENT_CONNECTION_H_INCLUDED_
