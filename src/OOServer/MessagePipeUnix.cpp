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

Root::MessagePipe::MessagePipe() :
	m_hSocket(ACE_INVALID_HANDLE)
{
}

int Root::MessagePipe::connect(ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Null_Mutex>& pipe, const ACE_CString& strAddr, ACE_Time_Value* wait)
{
	ACE_UNIX_Addr addr(strAddr.c_str());

	ACE_SOCK_Stream stream;
	if (ACE_SOCK_Connector().connect(stream,addr,wait) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("connector.connect() failed")),-1);

	pipe->m_hSocket = stream.get_handle();
	stream.set_handle(ACE_INVALID_HANDLE);

	return 0;
}

void Root::MessagePipe::close()
{
	ACE_HANDLE hSocket = m_hSocket;
	m_hSocket = ACE_INVALID_HANDLE;

	if (hSocket != ACE_INVALID_HANDLE)
		ACE_OS::close(hSocket);
}

ACE_HANDLE Root::MessagePipe::get_read_handle() const
{
	return m_hSocket;
}

ssize_t Root::MessagePipe::send(const void* buf, size_t len, size_t* sent)
{
	return ACE_OS::send(m_hSocket,(const char*)buf,len);
}

ssize_t Root::MessagePipe::send(const ACE_Message_Block* mb, ACE_Time_Value* timeout, size_t* sent)
{
	return ACE::send_n(m_hSocket,mb,timeout,sent);
}

ssize_t Root::MessagePipe::recv(void* buf, size_t len)
{
	return ACE_OS::recv(m_hSocket,(char*)buf,len);
}

Root::MessagePipeAcceptor::MessagePipeAcceptor()
{
}

Root::MessagePipeAcceptor::~MessagePipeAcceptor()
{
}

int Root::MessagePipeAcceptor::open(const ACE_CString& strAddr, uid_t uid)
{
	ACE_UNIX_Addr addr(ACE_TEXT_CHAR_TO_TCHAR(strAddr.c_str()));

	if (m_acceptor.open(addr) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("acceptor.open() failed")),-1);

	m_strAddr = strAddr;

	return 0;
}

int Root::MessagePipeAcceptor::accept(ACE_Refcounted_Auto_Ptr<MessagePipe,ACE_Null_Mutex>& pipe, ACE_Time_Value* timeout)
{
	ACE_SOCK_Stream stream;
	if (m_acceptor.accept(stream,0,timeout) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("acceptor.accept() failed")),-1);

	pipe->m_hSocket = stream.get_handle();
	stream.set_handle(ACE_INVALID_HANDLE);

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
