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
        static int run(const ACE_WString& strPipe);

		ACE_InputCDR sendrecv_root(const ACE_OutputCDR& request);

	private:
		friend class Channel;
		friend class ACE_Singleton<Manager, ACE_Thread_Mutex>;
		typedef ACE_Singleton<Manager, ACE_Thread_Mutex> USER_MANAGER;

		Manager();
		virtual ~Manager();
		Manager(const Manager&) : Root::MessageHandler() {}
		Manager& operator = (const Manager&) { return *this; }

		ACE_RW_Thread_Mutex			m_lock;
		ACE_CDR::UShort             m_root_channel;
		
		std::map<ACE_CDR::UShort,OTL::ObjectPtr<Omega::Remoting::IObjectManager> > m_mapOMs;

		int run_event_loop_i(const ACE_WString& strPipe);
		bool init(const ACE_WString& strPipe);
		bool bootstrap(Root::MessagePipe& pipe);
		void close_channels();
		void end_event_loop();

		OTL::ObjectPtr<Omega::Remoting::IObjectManager> get_object_manager(ACE_CDR::UShort src_channel_id);
		void process_request(const Root::MessagePipe& pipe, ACE_InputCDR& request, ACE_CDR::UShort src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::UShort attribs);
		void process_user_request(OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOM, const ACE_InputCDR& input, ACE_CDR::UShort src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::UShort attribs);
		void process_root_request(ACE_InputCDR& input, ACE_CDR::UShort src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::UShort attribs);

		static ACE_THR_FUNC_RETURN proactor_worker_fn(void*);
		static ACE_THR_FUNC_RETURN request_worker_fn(void* pParam);
	};
}

#endif // OOSERVER_USER_MANAGER_H_INCLUDED_
