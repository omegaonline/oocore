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

#ifndef OOSERVER_MSG_PIPE_H_INCLUDED_
#define OOSERVER_MSG_PIPE_H_INCLUDED_

namespace Root
{
	class MessagePipe
	{
		friend class MessagePipeAcceptor;

	public:
		MessagePipe();
		
		static ACE_WString unique_name(const ACE_WString& strPrefix);
		static int connect(MessagePipe& pipe, const ACE_WString& strAddr, ACE_Time_Value* wait = 0);
		void close();

		ACE_HANDLE get_read_handle() const;
		
		ssize_t send(const void* buf, size_t len, size_t* sent = 0);
		ssize_t send(const ACE_Message_Block* mb, ACE_Time_Value* timeout = 0, size_t* sent = 0);
		ssize_t recv(void* buf, size_t len);

	private:
#if defined(ACE_HAS_WIN32_NAMED_PIPES)
		ACE_HANDLE m_hRead;
		ACE_HANDLE m_hWrite;
#else
		ACE_HANDLE m_hSocket;
#endif
	};

	class MessagePipeAcceptor
	{
	public:
		MessagePipeAcceptor();
		~MessagePipeAcceptor();

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
		int open(const ACE_WString& strAddr, HANDLE hToken);
#else
		int open(const ACE_WString& strAddr, uid_t uid);
#endif

		int accept(MessagePipe& pipe, ACE_Time_Value* timeout = 0);
		ACE_HANDLE get_handle();
		void close();
	
#if defined(ACE_HAS_WIN32_NAMED_PIPES)
		static bool CreateSA(HANDLE hToken, void*& pSD, PACL& pACL);

	private:
		SECURITY_ATTRIBUTES m_sa;
		PACL                m_pACL;
		ACE_SPIPE_Acceptor  m_acceptor_up;
		ACE_SPIPE_Acceptor  m_acceptor_down;
#else
	private:
		ACE_SOCK_Acceptor   m_acceptor;
		ACE_WString         m_strAddr;
#endif

		MessagePipeAcceptor(const MessagePipeAcceptor&) {}
		MessagePipeAcceptor& operator = (const MessagePipeAcceptor&) { return *this; }
	};

	template <class T>
	class MessagePipeAsyncAcceptor : public ACE_Event_Handler
	{
	public:
		MessagePipeAsyncAcceptor() : ACE_Event_Handler()
		{}

		int start(T* pHandler, const ACE_WString& strAddr);
		void stop();

	private:
		T*                  m_pHandler;
		MessagePipeAcceptor m_acceptor;

		int handle_signal(int, siginfo_t*, ucontext_t*);

		MessagePipeAsyncAcceptor(const MessagePipeAsyncAcceptor&) : ACE_Event_Handler() {};
		MessagePipeAsyncAcceptor& operator = (const MessagePipeAsyncAcceptor&) { return *this; };
	};

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	template <class T>
	class MessagePipeSingleAsyncAcceptor : public ACE_Event_Handler
	{
	public:
		MessagePipeSingleAsyncAcceptor();
		virtual ~MessagePipeSingleAsyncAcceptor();

		int start(T* pHandler, const ACE_WString& strAddr);
		void stop();

	private:
		T*                  m_pHandler;
		SECURITY_ATTRIBUTES m_sa;
		PACL                m_pACL;
		ACE_SPIPE_Acceptor  m_acceptor;

		int handle_signal(int, siginfo_t*, ucontext_t*);

		MessagePipeSingleAsyncAcceptor(const MessagePipeSingleAsyncAcceptor&) : ACE_Event_Handler() {};
		MessagePipeSingleAsyncAcceptor& operator = (const MessagePipeSingleAsyncAcceptor&) { return *this; };
	};
#else
	#define MessagePipeSingleAsyncAcceptor MessagePipeAsyncAcceptor
#endif
}

#include "./MessagePipe.inl"

#endif // OOSERVER_MSG_PIPE_H_INCLUDED_
