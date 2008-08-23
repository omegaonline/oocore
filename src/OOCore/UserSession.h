///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
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
#include "./ApartmentImpl.h"

namespace OOCore
{
	struct Message
	{
		enum Type
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
			system_message = 0xFFFF0000,
			channel_close = 0x10000,
			channel_reflect = 0x20000
		};

		ACE_CDR::ULong                                         m_src_channel_id;
		ACE_CDR::UShort                                        m_dest_thread_id;
		ACE_CDR::UShort                                        m_src_thread_id;
		ACE_CDR::ULong                                         m_attribs;
		ACE_CDR::ULong                                         m_seq_no;
		ACE_CDR::UShort                                        m_type;
		ACE_CDR::UShort                                        m_apartment_id;
		ACE_Time_Value                                         m_deadline;
		ACE_Refcounted_Auto_Ptr<ACE_InputCDR,ACE_Thread_Mutex> m_ptrPayload;
	};

	class UserSession
	{
	public:
		static Omega::IException* init();
		static void term();
		static bool handle_request(Omega::uint32_t timeout);
		static ACE_Refcounted_Auto_Ptr<Apartment,ACE_Thread_Mutex> create_apartment();
		static OTL::ObjectPtr<OTL::ObjectImpl<Channel> > create_channel(ACE_CDR::ULong src_channel_id, const Omega::guid_t& message_oid);

		Omega::Remoting::MarshalFlags_t classify_channel(ACE_CDR::ULong channel);
		bool send_request(ACE_CDR::UShort apartment_id, ACE_CDR::ULong dest_channel_id, const ACE_Message_Block* request, ACE_InputCDR*& response, ACE_CDR::ULong timeout, ACE_CDR::ULong attribs);
		bool send_response(ACE_CDR::UShort apt_id, ACE_CDR::ULong seq_no, ACE_CDR::ULong dest_channel_id, ACE_CDR::UShort dest_thread_id, const ACE_Message_Block* response, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs = Message::synchronous);
				
	private:
		friend class ThreadContext;
		friend class ACE_Singleton<UserSession, ACE_Recursive_Thread_Mutex>;
		typedef ACE_Unmanaged_Singleton<UserSession, ACE_Recursive_Thread_Mutex> USER_SESSION;

		class MessagePipe
		{
		public:
			static int connect(MessagePipe& pipe, const ACE_CString& strAddr, ACE_Time_Value* wait = 0);
			void close();

			ssize_t send(const ACE_Message_Block* mb, ACE_Time_Value* timeout = 0, size_t* sent = 0);
			ssize_t recv(void* buf, size_t len);

		private:
#if defined(ACE_HAS_WIN32_NAMED_PIPES)
			ACE_HANDLE m_hRead;
			ACE_HANDLE m_hWrite;
#else
			ACE_SOCK_Stream m_stream;
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
		Omega::uint32_t     m_nIPSCookie;

		struct ThreadContext
		{
			ACE_CDR::UShort                             m_thread_id;
			ACE_Message_Queue_Ex<Message,ACE_MT_SYNCH>* m_msg_queue;

			// Transient data
			size_t                                      m_usage;
			std::map<ACE_CDR::ULong,ACE_CDR::UShort>    m_mapChannelThreads;
			ACE_Time_Value                              m_deadline;
			ACE_CDR::ULong                              m_seq_no;
			ACE_CDR::UShort                             m_current_apt;

			static ThreadContext* instance();

		private:
			friend class ACE_TSS_Singleton<ThreadContext,ACE_Recursive_Thread_Mutex>;
			friend class ACE_TSS<ThreadContext>;

			ThreadContext();
			~ThreadContext();
		};

		ACE_Atomic_Op<ACE_Thread_Mutex,unsigned long> m_consumers;
		std::map<ACE_CDR::UShort,ThreadContext*>      m_mapThreadContexts;
		ACE_Message_Queue_Ex<Message,ACE_MT_SYNCH>    m_default_msg_queue;

		// Accessors for ThreadContext
		ACE_CDR::UShort insert_thread_context(ThreadContext* pContext);
		void remove_thread_context(ACE_CDR::UShort thread_id);

		// Proper private members
		bool init_i();
		void term_i();
		Omega::IException* bootstrap();
		bool discover_server_port(ACE_CString& uPort);
		bool launch_server();

		int run_read_loop();
		int pump_requests(const ACE_Time_Value* deadline = 0, bool bOnce = false);
		void process_request(const Message* pMsg, const ACE_Time_Value& deadline);
		bool wait_for_response(ACE_InputCDR*& response, ACE_CDR::UShort apartment_id, ACE_CDR::ULong seq_no, const ACE_Time_Value* deadline, ACE_CDR::ULong from_channel_id);
		bool build_header(ACE_CDR::ULong seq_no, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort src_thread_id, ACE_CDR::ULong dest_channel_id, ACE_CDR::UShort dest_thread_id, ACE_OutputCDR& header, const ACE_Message_Block* mb, const ACE_Time_Value& deadline, ACE_CDR::UShort flags, ACE_CDR::ULong attribs);
		//bool send_channel_close(ACE_CDR::ULong closed_channel_id);
		void process_channel_close(ACE_CDR::ULong closed_channel_id);
		
		static ACE_THR_FUNC_RETURN io_worker_fn(void* pParam);

		// Apartment members
		ACE_CDR::UShort                                                                m_next_apartment;
		ACE_Refcounted_Auto_Ptr<Apartment,ACE_Thread_Mutex>                            m_ptrZeroApt;
		std::map<ACE_CDR::UShort,ACE_Refcounted_Auto_Ptr<Apartment,ACE_Thread_Mutex> > m_mapApartments;
		
		ACE_Refcounted_Auto_Ptr<Apartment,ACE_Thread_Mutex> create_apartment_i();
		OTL::ObjectPtr<OTL::ObjectImpl<Channel> > create_channel_i(ACE_CDR::ULong src_channel_id, const Omega::guid_t& message_oid);
	};
}

#endif // OOCORE_USER_SESSION_H_INCLUDED_
