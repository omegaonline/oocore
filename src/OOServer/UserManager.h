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
		static int run(u_short uPort);
		
		ACE_InputCDR sendrecv_root(const ACE_OutputCDR& request);

	private:
		friend class Channel;
		friend class ACE_Singleton<Manager, ACE_Thread_Mutex>;
		typedef ACE_Singleton<Manager, ACE_Thread_Mutex> USER_MANAGER;

		Manager();
		virtual ~Manager();
		Manager(const Manager&) {}
		Manager& operator = (const Manager&) { return *this; }

		ACE_RW_Thread_Mutex			m_lock;
		ACE_CDR::UShort             m_root_channel;
				
		struct OMInfo
		{
			OTL::ObjectPtr<Omega::Remoting::IObjectManager> m_ptrOM;
			OTL::ObjectPtr<OTL::ObjectImpl<User::Channel> > m_ptrChannel;
		};
		std::map<ACE_CDR::UShort,OMInfo> m_mapOMs;

		int run_event_loop_i(u_short uPort);
		int init(u_short uPort);
		int bootstrap(ACE_CDR::UShort sandbox_channel);

		OMInfo get_object_manager(ACE_CDR::UShort src_channel_id);
		void process_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::UShort src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::UShort attribs);
		void process_user_request(OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOM, const ACE_InputCDR& input, ACE_CDR::UShort src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::UShort attribs);
		void process_root_request(ACE_InputCDR& input, ACE_CDR::UShort src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::UShort attribs);
				
		static ACE_THR_FUNC_RETURN proactor_worker_fn(void*);
		static ACE_THR_FUNC_RETURN request_worker_fn(void* pParam);
	};
}

#endif // OOSERVER_USER_MANAGER_H_INCLUDED_
