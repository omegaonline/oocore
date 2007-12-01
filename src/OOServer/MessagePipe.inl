///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
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
//	It can be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_MSG_PIPE_INL_INCLUDED_
#define OOSERVER_MSG_PIPE_INL_INCLUDED_

template <class T>
int Root::MessagePipeAsyncAcceptor<T>::start(T* pHandler, int key, const ACE_WString& strAddr)
{
	m_pHandler = pHandler;
	m_key = key;

	if (m_acceptor.open(strAddr,0) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"acceptor.open() failed"),-1);

	// This will probably have to change under UNIX

	if (ACE_Reactor::instance()->register_handler(this,m_acceptor.get_handle()) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"register_handler() failed"),-1);

	return 0;
}

template <class T>
int Root::MessagePipeAsyncAcceptor<T>::handle_signal(int, siginfo_t*, ucontext_t*)
{
	MessagePipe pipe;

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	if (m_acceptor.accept(pipe) != 0 && GetLastError() != ERROR_MORE_DATA)
#else
	if (m_acceptor.accept(pipe) != 0)
#endif
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"acceptor.accept() failed"),-1);
	}

	return m_pHandler->on_accept(pipe,m_key);
}

template <class T>
void Root::MessagePipeAsyncAcceptor<T>::stop()
{
	ACE_Reactor::instance()->remove_handler(m_acceptor.get_handle(),ACE_Event_Handler::ALL_EVENTS_MASK);

    m_acceptor.close();
}

#if defined(ACE_HAS_WIN32_NAMED_PIPES)

template <class T>
Root::MessagePipeSingleAsyncAcceptor<T>::MessagePipeSingleAsyncAcceptor() : ACE_Event_Handler()
{
	m_sa.lpSecurityDescriptor = NULL;
	m_sa.nLength = 0;
	m_pACL = NULL;
}

template <class T>
Root::MessagePipeSingleAsyncAcceptor<T>::~MessagePipeSingleAsyncAcceptor()	
{
	LocalFree(m_pACL);
	LocalFree(m_sa.lpSecurityDescriptor);
}

template <class T>
int Root::MessagePipeSingleAsyncAcceptor<T>::start(T* pHandler, int key, const ACE_WString& strAddr)
{
	m_pHandler = pHandler;
	m_key = key;

	if (m_sa.nLength == 0)
	{
		m_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		m_sa.bInheritHandle = FALSE;

		if (!MessagePipeAcceptor::CreateSA(0,m_sa.lpSecurityDescriptor,m_pACL))
			ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] Failed to create security descriptor: %x\n",GetLastError()),-1);
	}

	ACE_SPIPE_Addr addr;
	addr.string_to_addr(strAddr.c_str());
	if (m_acceptor.open(addr,1,ACE_DEFAULT_FILE_PERMS,&m_sa) != 0)
	{
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"acceptor.open failed"),-1);
	}

	if (ACE_Reactor::instance()->register_handler(this,m_acceptor.get_handle()) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"register_handler failed"),-1);

	return 0;
}

template <class T>
void Root::MessagePipeSingleAsyncAcceptor<T>::stop()
{
	ACE_Reactor::instance()->remove_handler(m_acceptor.get_handle(),ACE_Event_Handler::ALL_EVENTS_MASK);

	m_acceptor.close();
}

template <class T>
int Root::MessagePipeSingleAsyncAcceptor<T>::handle_signal(int, siginfo_t*, ucontext_t*)
{
	ACE_SPIPE_Stream stream;

	if (m_acceptor.accept(stream) != 0 && GetLastError() != ERROR_MORE_DATA)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"acceptor.accept() failed"),-1);

	return m_pHandler->on_accept(stream,m_key);
}
#endif

#endif // OOSERVER_MSG_PIPE_INL_INCLUDED_
