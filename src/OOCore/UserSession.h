///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOCORE_USER_SESSION_H_INCLUDED_
#define OOCORE_USER_SESSION_H_INCLUDED_

#include "./Channel.h"

namespace OOCore
{
	class UserSession
	{
	public:
		static Omega::IException* init();
		static void term();
		static void handle_requests(Omega::uint32_t timeout);
		
	private:
		friend class Channel;
		friend class ThreadContext;
		friend class ChannelMarshalFactory;
		friend class ACE_Singleton<UserSession, ACE_Thread_Mutex>;
		typedef ACE_Singleton<UserSession, ACE_Thread_Mutex> USER_SESSION;
		
		class MessagePipe
		{
		public:
			static int connect(MessagePipe& pipe, const ACE_WString& strAddr, ACE_Time_Value* wait = 0);
			void close();
			
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

		UserSession();
		~UserSession();
		UserSession(const UserSession&) {}
		UserSession& operator = (const UserSession&) { return *this; }

		ACE_RW_Thread_Mutex m_lock;
		int                 m_thrd_grp_id;
		MessagePipe         m_stream;
		ACE_CDR::ULong      m_channel_id;
		ACE_Thread_Mutex    m_send_lock;
		Omega::uint32_t     m_nIPSCookie;

		struct OMInfo
		{
			Omega::Remoting::MarshalFlags_t                 m_marshal_flags;
			OTL::ObjectPtr<Omega::Remoting::IObjectManager> m_ptrOM;
		};
		std::map<ACE_CDR::ULong,OMInfo> m_mapOMs;
		
		struct Message
		{
			enum Flags
			{
				Response = 0,
				Request = 1
			};

			enum Attributes
			{	
				// Low 16 bits must match Remoting::MethodAttributes
				synchronous = 0,
				asynchronous = 1,
				unreliable = 2,
				encrypted = 4,

				// Upper 16 bits can be used for system messages
				system_message = 0x10000,
				channel_close = 0x20000 | system_message
			};

			ACE_CDR::UShort                                      m_dest_thread_id;
			ACE_CDR::ULong                                       m_src_channel_id;
			ACE_CDR::UShort                                      m_src_thread_id;
			ACE_Time_Value                                       m_deadline;
			ACE_CDR::ULong                                       m_attribs;
			ACE_CDR::UShort                                      m_flags;
			ACE_Refcounted_Auto_Ptr<ACE_InputCDR,ACE_Null_Mutex> m_ptrPayload;
		};

		struct ThreadContext
		{
			ACE_CDR::UShort                             m_thread_id;
			ACE_Message_Queue_Ex<Message,ACE_MT_SYNCH>* m_msg_queue;
						
			// Transient data
			size_t										m_usage;
			std::map<ACE_CDR::ULong,ACE_CDR::UShort>    m_mapChannelThreads;
			ACE_Time_Value                              m_deadline;
			
			static ThreadContext* instance();

		private:
			friend class ACE_TSS_Singleton<ThreadContext,ACE_Thread_Mutex>;
			friend class ACE_TSS<ThreadContext>;

			ThreadContext();
			~ThreadContext();
		};
		
		ACE_Atomic_Op<ACE_Thread_Mutex,unsigned long>   m_consumers;
		std::map<ACE_CDR::UShort,const ThreadContext*>  m_mapThreadContexts;
		ACE_Message_Queue_Ex<Message,ACE_MT_SYNCH>      m_default_msg_queue;

		// Accessors for ThreadContext
		ACE_CDR::UShort insert_thread_context(const ThreadContext* pContext);
		void remove_thread_context(const ThreadContext* pContext);

		// Accessors for Channel
		bool send_request(ACE_CDR::ULong dest_channel_id, const ACE_Message_Block* request, ACE_InputCDR*& response, ACE_CDR::UShort timeout, ACE_CDR::ULong attribs);
		
		// Proper private members
		bool init_i();
		void term_i();
		Omega::IException* bootstrap();
		bool discover_server_port(ACE_WString& uPort);
		bool launch_server();

		int run_read_loop();
		void pump_requests(const ACE_Time_Value* deadline = 0, bool bOnce = false);
		void process_request(const UserSession::Message* pMsg, const ACE_Time_Value& deadline);
		bool wait_for_response(ACE_InputCDR*& response, const ACE_Time_Value* deadline, ACE_CDR::ULong from_channel_id);
		bool build_header(ACE_CDR::UShort src_thread_id, ACE_CDR::ULong dest_channel_id, ACE_CDR::UShort dest_thread_id, ACE_OutputCDR& header, const ACE_Message_Block* mb, const ACE_Time_Value& deadline, ACE_CDR::UShort flags, ACE_CDR::ULong attribs);
		void send_response(ACE_CDR::ULong dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* response);
		OTL::ObjectPtr<Omega::Remoting::IObjectManager> create_object_manager(ACE_CDR::ULong src_channel_id, Omega::Remoting::MarshalFlags_t marshal_flags);
		bool send_channel_close(ACE_CDR::ULong closed_channel_id);
		void process_channel_close(ACE_CDR::ULong closed_channel_id);
				
		static ACE_THR_FUNC_RETURN io_worker_fn(void* pParam);
	};
}

#endif // OOCORE_USER_SESSION_H_INCLUDED_
