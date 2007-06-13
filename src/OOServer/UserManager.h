#ifndef OOSERVER_USER_MANAGER_H_INCLUDED_
#define OOSERVER_USER_MANAGER_H_INCLUDED_

#include "./LocalAcceptor.h"
#include "./RequestHandler.h"
#include "./RootConnection.h"
#include "./UserConnection.h"
#include "./Protocol.h"

namespace User
{
	class Request : public RequestBase
	{
	public:
		Request::Request(ACE_HANDLE handle, ACE_InputCDR* input) :
		RequestBase(handle,input)
		{}

		bool	m_bRoot;
	};

	class Manager :
		public LocalAcceptor<Connection>,
		public RequestHandler<User::Request>
	{
	public:
		static int run(u_short uPort);
		static bool enqueue_user_request(ACE_InputCDR* input, ACE_HANDLE handle);
		static void user_connection_closed(ACE_HANDLE handle);

		void send_asynch(ACE_HANDLE handle, ACE_CDR::UShort dest_channel_id, const ACE_Message_Block* request, const ACE_Time_Value* deadline = 0);
		ACE_InputCDR send_synch(ACE_HANDLE handle, ACE_CDR::UShort dest_channel_id, const ACE_Message_Block* request, const ACE_Time_Value* deadline = 0);

	private:
		friend class ACE_Singleton<Manager, ACE_Thread_Mutex>;
		typedef ACE_Singleton<Manager, ACE_Thread_Mutex> USER_MANAGER;

		Manager();
		virtual ~Manager();
		Manager(const Manager&) :
            LocalAcceptor<Connection>(), RequestHandler<User::Request>()
        {}
		Manager& operator = (const Manager&) { return *this; }

		ACE_RW_Thread_Mutex			m_lock;
		ACE_HANDLE                  m_root_handle;
		ACE_CDR::UShort             m_uNextChannelId;

		struct ChannelPair
		{
			ACE_HANDLE			handle;
			ACE_CDR::UShort		channel;
		};
		std::map<ACE_CDR::UShort,ChannelPair>                                 m_mapChannelIds;
		std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >       m_mapReverseChannelIds;
		std::map<ACE_HANDLE,OTL::ObjectPtr<Omega::Remoting::IObjectManager> > m_mapOMs;

		int run_event_loop_i(u_short uPort);
		int init(u_short uPort);
		void stop_i();
		void term();
		int bootstrap(ACE_SOCK_STREAM& stream);

		bool enqueue_root_request(ACE_InputCDR* input, ACE_HANDLE handle);
		void root_connection_closed(const ACE_CString& key, ACE_HANDLE handle);
		void process_request(User::Request* request, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, const ACE_Time_Value& request_deadline);
		void forward_request(User::Request* request, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, const ACE_Time_Value& request_deadline);

		static ACE_THR_FUNC_RETURN proactor_worker_fn(void*);

		void process_root_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::ULong trans_id, const ACE_Time_Value& request_deadline);
		void process_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, const ACE_Time_Value& request_deadline);
		OTL::ObjectPtr<Omega::Remoting::IObjectManager> get_object_manager(ACE_HANDLE handle, ACE_CDR::UShort channel_id);

		void user_connection_closed_i(ACE_HANDLE handle);
		int validate_connection(const ACE_Asynch_Accept::Result& result, const ACE_INET_Addr& remote, const ACE_INET_Addr& local);
	};
}

#endif // OOSERVER_USER_MANAGER_H_INCLUDED_
