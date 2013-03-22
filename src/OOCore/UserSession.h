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

#include "Channel.h"
#include "Compartment.h"

namespace OOCore
{
	struct Message
	{
		Message(size_t len = 0) :
				m_src_channel_id(0),
				m_attribs(0),
				m_dest_thread_id(0),
				m_src_thread_id(0),
				m_type(Response),
				m_payload(len),
				m_dest_cmpt_id(0)
		{}

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
			channel_reflect = 0x20000,
			channel_ping = 0x30000
		};

		// This is the order for I/O
		Omega::uint32_t    m_src_channel_id;
		Omega::uint32_t    m_attribs;
		Omega::uint16_t    m_dest_thread_id;
		Omega::uint16_t    m_src_thread_id;
		Type               m_type;
		OOBase::CDRStream  m_payload;

		// Not persisted
		Omega::uint16_t    m_dest_cmpt_id;
	};

	class UserSession : public OOBase::NonCopyable
	{
		friend class ThreadContext;
		friend class OOBase::Singleton<UserSession,OOCore::DLL>;

	public:
		void init(bool bHosted);
		void term();

		bool pump_request(const OOBase::Timeout& timeout = OOBase::Timeout());
		
		static Omega::IObject* create_channel(Omega::uint32_t src_channel_id, const Omega::guid_t& message_oid, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
		void send_request(Omega::uint32_t dest_channel_id, OOBase::CDRStream* request, OOBase::CDRStream* response, Omega::uint32_t attribs);
		void send_response(const Message& msg, OOBase::CDRStream* response);
		Omega::uint32_t get_channel_id() const;

		static OTL::ObjectImpl<OOCore::ComptChannel>* create_compartment(const Omega::guid_t& channel_oid);
		OOBase::SmartPtr<Compartment> get_compartment(Omega::uint16_t id);
		void remove_compartment(Omega::uint16_t id);
		Omega::uint16_t update_state(Omega::uint16_t compartment_id);

	private:
		UserSession();
		~UserSession();
		
		// 'Main' data
		OOBase::RWMutex                  m_lock;
		OOBase::Thread                   m_worker_thread;
		OOBase::RefPtr<OOBase::Socket>   m_stream;
		Omega::uint32_t                  m_channel_id;

		// Startup/shutdown data
		OOBase::Condition                m_cond;
		OOBase::Condition::Mutex         m_cond_mutex;
		size_t                           m_init_count;
		enum
		{
			eStopped,
			eStarting,
			eStarted,
			eStopping
		} m_init_state;

		struct ThreadContext
		{
			Omega::uint16_t               m_thread_id;
			OOBase::BoundedQueue<Message> m_msg_queue;

			// 'Private' thread-local data
			OOBase::Atomic<size_t>        m_usage_count;
			Omega::uint16_t               m_current_cmpt;

			OOBase::HashTable<Omega::uint32_t,Omega::uint16_t,OOBase::ThreadLocalAllocator> m_mapChannelThreads;

			static ThreadContext* instance();

		private:
			friend class OOBase::TLSSingleton<ThreadContext,OOCore::DLL>;

			ThreadContext();
			~ThreadContext();
		};

		OOBase::Atomic<size_t>                              m_usage_count;		
		OOBase::HandleTable<Omega::uint16_t,ThreadContext*> m_mapThreadContexts;
		OOBase::BoundedQueue<Message>                       m_default_msg_queue;

		// Accessors for ThreadContext
		Omega::uint16_t insert_thread_context(ThreadContext* pContext);
		void remove_thread_context(Omega::uint16_t thread_id);

		// Proper private members
		void start(bool bHosted);
		void stop();
		void connect_root(OOBase::CDRStream& response);

		// Our object factory members
		OOBase::Stack<Omega::uint32_t> m_rot_cookies;

		// Message pumping
		int run_read_loop();
		void respond_exception(OOBase::CDRStream& response, Omega::IException* pE);
		void send_response_catch(const Message& msg, OOBase::CDRStream* response);
		void process_request(ThreadContext* pContext, const Message& msg);
		void wait_for_response(ThreadContext* pContext, OOBase::CDRStream& response, Omega::uint32_t from_channel_id);
		void build_header(OOBase::CDRStream& header, Omega::uint32_t src_channel_id, Omega::uint16_t src_thread_id, Omega::uint32_t dest_channel_id, Omega::uint16_t dest_thread_id, const OOBase::CDRStream* request, Message::Type type, Omega::uint32_t attribs);
		void process_channel_close(Omega::uint32_t closed_channel_id);
		void wait_or_alert(const OOBase::Atomic<size_t>& usage);

		static int io_worker_fn(void* pParam);

		// Compartment members
		OOBase::HandleTable<Omega::uint16_t,OOBase::SmartPtr<Compartment> > m_mapCompartments;

		void close_compartments();
		OTL::ObjectImpl<OOCore::ComptChannel>* create_compartment_i(const Omega::guid_t& channel_oid);
		Omega::IObject* create_channel_i(Omega::uint32_t src_channel_id, const Omega::guid_t& message_oid, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags);
	};
}

#endif // OOCORE_USER_SESSION_H_INCLUDED_
