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
#include "ApartmentImpl.h"

namespace OOCore
{
	struct Message
	{
		Message(size_t len) : m_payload(len)
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
			channel_reflect = 0x20000
		};

		Omega::uint32_t                m_src_channel_id;
		Omega::uint16_t                m_dest_thread_id;
		Omega::uint16_t                m_src_thread_id;
		Omega::uint32_t                m_attribs;
		Omega::uint32_t                m_seq_no;
		Omega::uint16_t                m_type;
		Omega::uint16_t                m_apartment_id;
		OOBase::timeval_t              m_deadline;
		OOBase::CDRStream              m_payload;
	};

	class UserSession
	{
	public:
		static Omega::IException* init();
		static void term();
		static bool handle_request(Omega::uint32_t timeout);

		static OTL::ObjectPtr<OTL::ObjectImpl<Channel> > create_channel(Omega::uint32_t src_channel_id, const Omega::guid_t& message_oid);
		Omega::Remoting::MarshalFlags_t classify_channel(Omega::uint32_t channel);
		OOBase::CDRStream* send_request(Omega::uint16_t apartment_id, Omega::uint32_t dest_channel_id, const OOBase::CDRStream* request, Omega::uint32_t timeout, Omega::uint32_t attribs);
		void send_response(Omega::uint16_t apt_id, Omega::uint32_t seq_no, Omega::uint32_t dest_channel_id, Omega::uint16_t dest_thread_id, const OOBase::CDRStream* response, const OOBase::timeval_t& deadline, Omega::uint32_t attribs = Message::synchronous);

		static Omega::Apartment::IApartment* create_apartment();
		static Omega::uint16_t get_current_apartment();
		static void remove_apartment(Omega::uint16_t id);
		
		OOBase::SmartPtr<Apartment> get_apartment(Omega::uint16_t id);
		Omega::uint16_t update_state(Omega::uint16_t apartment_id, Omega::uint32_t* pTimeout);
				
	private:
		friend class ThreadContext;
		friend class OOBase::SingletonNoDestroy<UserSession>;
		typedef OOBase::SingletonNoDestroy<UserSession> USER_SESSION;

		UserSession();
		~UserSession();
		UserSession(const UserSession&) {}
		UserSession& operator = (const UserSession&) { return *this; }

		OOBase::RWMutex                  m_lock;
		OOBase::Thread                   m_worker_thread;
		OOBase::SmartPtr<OOBase::Socket> m_stream;
		Omega::uint32_t                  m_channel_id;
		Omega::uint32_t                  m_nIPSCookie;

		struct ThreadContext
		{
			Omega::uint16_t                             m_thread_id;
			OOBase::BoundedQueue<Message*>              m_msg_queue;

			// Transient data
			OOBase::AtomicInt<size_t>                   m_usage_count;
			std::map<Omega::uint32_t,Omega::uint16_t>   m_mapChannelThreads;
			OOBase::timeval_t                           m_deadline;
			Omega::uint32_t                             m_seq_no;
			Omega::uint16_t                             m_current_apt;

			static ThreadContext* instance();

		private:
			friend class OOBase::TLSSingleton<ThreadContext>;
			
			ThreadContext();
			~ThreadContext();
		};

		OOBase::AtomicInt<size_t>                m_usage_count;
		std::map<Omega::uint16_t,ThreadContext*> m_mapThreadContexts;
		OOBase::BoundedQueue<Message*>           m_default_msg_queue;

		// Accessors for ThreadContext
		Omega::uint16_t insert_thread_context(ThreadContext* pContext);
		void remove_thread_context(Omega::uint16_t thread_id);

		// Proper private members
		void init_i();
		void term_i();
		void bootstrap();
		std::string discover_server_port(OOBase::timeval_t& wait);
		
		int run_read_loop();
		bool pump_requests(const OOBase::timeval_t* deadline = 0, bool bOnce = false);
		void process_request(const Message* pMsg, const OOBase::timeval_t& deadline);
		OOBase::CDRStream* wait_for_response(Omega::uint16_t apartment_id, Omega::uint32_t seq_no, const OOBase::timeval_t* deadline, Omega::uint32_t from_channel_id);
		OOBase::CDRStream build_header(Omega::uint32_t seq_no, Omega::uint32_t src_channel_id, Omega::uint16_t src_thread_id, Omega::uint32_t dest_channel_id, Omega::uint16_t dest_thread_id, const OOBase::CDRStream* msg, const OOBase::timeval_t& deadline, Omega::uint16_t flags, Omega::uint32_t attribs);
		//bool send_channel_close(Omega::uint32_t closed_channel_id);
		void process_channel_close(Omega::uint32_t closed_channel_id);
		
		static int io_worker_fn(void* pParam);

		// Apartment members
		Omega::uint16_t                                        m_next_apartment;
		OOBase::SmartPtr<Apartment>                            m_ptrZeroApt;
		std::map<Omega::uint16_t,OOBase::SmartPtr<Apartment> > m_mapApartments;
		
		Omega::Apartment::IApartment* create_apartment_i();
		OTL::ObjectPtr<OTL::ObjectImpl<Channel> > create_channel_i(Omega::uint32_t src_channel_id, const Omega::guid_t& message_oid);
	};
}

#endif // OOCORE_USER_SESSION_H_INCLUDED_
