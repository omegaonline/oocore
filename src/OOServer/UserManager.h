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

#ifndef OOSERVER_USER_MANAGER_H_INCLUDED_
#define OOSERVER_USER_MANAGER_H_INCLUDED_

#include "./MessageConnection.h"
#include "./Protocol.h"
#include "./Channel.h"

namespace User
{
	class Manager : public Root::MessageHandler
	{
	public:
		static int run(const ACE_CString& strPipe);

		static Omega::uint32_t open_stream(const Omega::string_t& strEndPoint);
		
		ACE_InputCDR* sendrecv_root(const ACE_OutputCDR& request);

	private:
		friend class Channel;
		friend class ChannelMarshalFactory;
		friend class AsyncStreamCallback;
		friend class ACE_Singleton<Manager, ACE_Thread_Mutex>;
		friend class Root::MessagePipeAsyncAcceptor<Manager>;
		typedef ACE_Singleton<Manager, ACE_Thread_Mutex> USER_MANAGER;

		Manager();
		virtual ~Manager();
		Manager(const Manager&) : Root::MessageHandler() {}
		Manager& operator = (const Manager&) { return *this; }

		static const ACE_CDR::ULong m_root_channel;

		ACE_RW_Thread_Mutex m_lock;
		Omega::uint32_t     m_nIPSCookie;

		Root::MessagePipeAsyncAcceptor<Manager> m_process_acceptor;

		int on_accept(const ACE_Refcounted_Auto_Ptr<Root::MessagePipe,ACE_Null_Mutex>& pipe);
		
		struct OMInfo
		{
			Omega::Remoting::MarshalFlags_t                 m_marshal_flags;
			OTL::ObjectPtr<OTL::ObjectImpl<Channel> >       m_ptrChannel;
			OTL::ObjectPtr<Omega::Remoting::IObjectManager> m_ptrOM;
		};
		std::map<ACE_CDR::ULong,OMInfo> m_mapOMs;

		virtual bool channel_open(ACE_CDR::ULong channel);
		virtual bool route_off(ACE_CDR::ULong dest_channel_id, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort dest_thread_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs, const ACE_Message_Block* mb);

		int run_event_loop_i(const ACE_CString& strPipe);
		bool init(const ACE_CString& strPipe);
		bool bootstrap(ACE_CDR::ULong sandbox_channel);
		void end_event_loop();

		virtual void channel_closed(ACE_CDR::ULong channel);
		
		OTL::ObjectPtr<Omega::Remoting::IObjectManager> create_object_manager(ACE_CDR::ULong src_channel_id);
		void process_request(ACE_InputCDR& request, ACE_CDR::ULong seq_no, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs);
		void process_user_request(const ACE_InputCDR& input, ACE_CDR::ULong seq_no, ACE_CDR::ULong src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs);
		void process_root_request(ACE_InputCDR& input, ACE_CDR::ULong seq_no, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs);

		std::map<Omega::uint32_t,OTL::ObjectPtr<Omega::IO::IStream> > m_mapStreams;
		Omega::uint32_t open_stream_i(const Omega::string_t& strEndPoint);

		Omega::uint32_t m_nNextStream;

		static ACE_THR_FUNC_RETURN proactor_worker_fn(void*);
		static ACE_THR_FUNC_RETURN request_worker_fn(void* pParam);
	};
}

#endif // OOSERVER_USER_MANAGER_H_INCLUDED_

