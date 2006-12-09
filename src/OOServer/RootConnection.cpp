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

#include "./RootConnection.h"
#include "./RootManager.h"

int RootConnection::open(void* pAcceptor)
{
	int ret = ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>::open(pAcceptor);
	if (ret != 0)
		return ret;

	m_pManager = static_cast<RootManager*>(pAcceptor);

	return 0;
}

int RootConnection::handle_input(ACE_HANDLE)
{
	// Recv the request size
	Session::Request request = {0};
	if (peer().recv(&request.cbSize, sizeof(request.cbSize)) < sizeof(request.cbSize))
	{
		ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Connection closed\n")));
		return -1;
	}

	// Check the request size
	if (request.cbSize < sizeof(request))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Invalid request\n")),-1);

	// Read the reest of the request
	if (peer().recv(&request.uid, request.cbSize - sizeof(request.cbSize)) < static_cast<ssize_t>(request.cbSize - sizeof(request.cbSize)))
	{
		ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Connection closed\n")));
		return -1;
	}
	
	// Ask the client manager for a response...
	Session::Response response = {0};
	m_pManager->connect_client(request,response);
	
	// Try to send the response
	ssize_t send_cnt = peer().send(&response,response.cbSize);
	if (send_cnt == static_cast<ssize_t>(response.cbSize))
		return -1; // All done

	// Check for errors
	if (send_cnt == -1 && ACE_OS::last_error() != EWOULDBLOCK)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("send()")),-1);

	// If we are blocking, queue up the rest of the response, and send later...
	if (send_cnt == -1)
		send_cnt = 0;
	size_t remaining = static_cast<size_t>(response.cbSize - send_cnt);

	ACE_Message_Block* mb = 0;
	ACE_NEW_RETURN(mb,ACE_Message_Block(&(reinterpret_cast<const char*>(&response)[send_cnt]),remaining),-1);
	
	bool output_off = msg_queue()->is_empty();
	if (putq(mb) == -1)
	{
		mb->release();
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p; discarding data\n"),ACE_TEXT("putq() failed")),-1);
	}

	if (output_off)
		return reactor()->register_handler(this,ACE_Event_Handler::WRITE_MASK);
	else
		return -1; // All done
}

int RootConnection::handle_output(ACE_HANDLE)
{
	ACE_Message_Block* mb = 0;
	ACE_Time_Value nowait(ACE_OS::gettimeofday());
	while (getq(mb, &nowait) == 0)
	{
		ssize_t send_cnt = peer().send(mb->rd_ptr(),mb->length());
		if (send_cnt == -1)
			ACE_ERROR((LM_ERROR,ACE_TEXT("(%P|%t) %p\n"),ACE_TEXT("send")));
		else
			mb->rd_ptr(static_cast<size_t>(send_cnt));

		if (mb->length() > 0)
		{
			ungetq(mb);
			break;
		}
		mb->release();
	}

	return (msg_queue()->is_empty() ? -1 : 0);
}

int RootConnection::handle_close(ACE_HANDLE fd, ACE_Reactor_Mask mask)
{
	return ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>::handle_close(fd,mask);
}
