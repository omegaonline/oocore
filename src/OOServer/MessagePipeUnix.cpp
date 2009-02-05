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
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "./OOServer_Root.h"
#include "./MessagePipe.h"

#if !defined(ACE_HAS_WIN32_NAMED_PIPES)

Root::MessagePipe::MessagePipe()
{
}

int Root::MessagePipe::connect(ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex>& pipe, const ACE_CString& strAddr, ACE_Time_Value* wait)
{
	ACE_UNIX_Addr addr(strAddr.c_str());

	MessagePipe* p = 0;
	ACE_NEW_RETURN(p,MessagePipe,-1);
	pipe.reset(p);

	if (ACE_SOCK_Connector().connect(pipe->m_stream,addr,wait) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("connector.connect() failed")),-1);

	return 0;
}

void Root::MessagePipe::close()
{
    m_stream.close_reader();
	m_stream.close_writer();
	m_stream.close();
}

ACE_HANDLE Root::MessagePipe::get_read_handle() const
{
	return m_stream.get_handle();
}

ssize_t Root::MessagePipe::send(const void* buf, size_t len, size_t* sent)
{
	return m_stream.send_n(buf,len,0,sent);
}

ssize_t Root::MessagePipe::send(const ACE_Message_Block* mb, size_t* sent)
{
	return m_stream.send_n(mb,0,sent);
}

ssize_t Root::MessagePipe::recv(void* buf, size_t len)
{
	return m_stream.recv(buf,len);
}

Root::MessagePipeAcceptor::MessagePipeAcceptor()
{
}

Root::MessagePipeAcceptor::~MessagePipeAcceptor()
{
}

int Root::MessagePipeAcceptor::open(const ACE_CString& strAddr, uid_t uid)
{
    ACE_TString strAddr2 = ACE_TEXT_CHAR_TO_TCHAR(strAddr.c_str());

	// Remove any dangling entries
    ACE_OS::unlink(strAddr2.c_str());

    ACE_UNIX_Addr addr(strAddr2.c_str());
	if (m_acceptor.open(addr) != 0)
	    ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("acceptor.open() failed")),-1);

    if (chmod(strAddr.c_str(),S_IRWXU | S_IRWXG | S_IRWXO) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("chmod failed")),false);

	m_strAddr = strAddr;

	return 0;
}

int Root::MessagePipeAcceptor::accept(ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Thread_Mutex>& pipe, ACE_Time_Value* timeout)
{
    MessagePipe* p = 0;
	ACE_NEW_RETURN(p,MessagePipe,-1);
	pipe.reset(p);

	if (m_acceptor.accept(pipe->m_stream,0,timeout) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("acceptor.accept() failed")),-1);

	return 0;
}

ACE_HANDLE Root::MessagePipeAcceptor::get_handle()
{
	return m_acceptor.get_handle();
}

void Root::MessagePipeAcceptor::close()
{
	m_acceptor.close();

	ACE_OS::unlink(m_strAddr.c_str());
}

#endif // defined(ACE_HAS_WIN32_NAMED_PIPES)
