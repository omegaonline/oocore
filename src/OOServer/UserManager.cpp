#include "OOServer.h"

#include "./UserManager.h"
#include "./Channel.h"
#include "./UserServiceTable.h"
#include "./UserRegistry.h"

int UserMain(u_short uPort)
{
	return User::Manager::run(uPort);
}

using namespace Omega;
using namespace OTL;

namespace User
{
	class InterProcessService :
		public ObjectBase,
		public Remoting::IInterProcessService
	{
	public:
		void Init(ObjectPtr<Remoting::IObjectManager> ptrOM, Manager* pManager)
		{
			m_ptrOM = ptrOM;
			m_pManager = pManager;
		}

		BEGIN_INTERFACE_MAP(InterProcessService)
			INTERFACE_ENTRY(Remoting::IInterProcessService)
		END_INTERFACE_MAP()

	private:
		ACE_Thread_Mutex                           m_lock;
		ObjectPtr<Remoting::IObjectManager>        m_ptrOM;
		ObjectPtr<ObjectImpl<ServiceTable> >       m_ptrST;
		ObjectPtr<ObjectImpl<Registry::BaseKey> >  m_ptrReg;
		Manager*                                   m_pManager;

	// Remoting::IInterProcessService members
	public:
		Omega::Registry::IRegistryKey* GetRegistry();
		Activation::IServiceTable* GetServiceTable();
	};

	class InterProcessServiceFactory :
		public ObjectBase,
		public Activation::IObjectFactory
	{
	public:
		void Init(ObjectPtr<Remoting::IObjectManager> ptrOM, Manager* pManager)
		{
			m_ptrOM = ptrOM;
			m_pManager = pManager;
		}

		BEGIN_INTERFACE_MAP(InterProcessServiceFactory)
			INTERFACE_ENTRY(Activation::IObjectFactory)
		END_INTERFACE_MAP()

	private:
		ObjectPtr<Remoting::IObjectManager> m_ptrOM;
		Manager*                            m_pManager;

	// Activation::IObjectFactory members
	public:
		void CreateObject(IObject* pOuter, const guid_t& iid, IObject*& pObject);
	};

}

void User::InterProcessServiceFactory::CreateObject(IObject* pOuter, const guid_t& iid, IObject*& pObject)
{
	if (pOuter)
		Activation::INoAggregationException::Throw(Remoting::OID_InterProcess);

	ObjectPtr<SingletonObjectImpl<InterProcessService> > ptrIPS = SingletonObjectImpl<InterProcessService>::CreateObjectPtr();
	ptrIPS->Init(m_ptrOM,m_pManager);

	pObject = ptrIPS->QueryInterface(iid);
	if (!pObject)
		INoInterfaceException::Throw(iid,OMEGA_SOURCE_INFO);
}

Registry::IRegistryKey* User::InterProcessService::GetRegistry()
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,m_lock,OOSERVER_THROW_LASTERROR());

	if (!m_ptrReg)
	{
		m_ptrReg = ObjectImpl<User::Registry::BaseKey>::CreateObjectPtr();
		m_ptrReg->Init(m_pManager);
	}

	return m_ptrReg.AddRefReturn();
}

Activation::IServiceTable* User::InterProcessService::GetServiceTable()
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,m_lock,OOSERVER_THROW_LASTERROR());

	if (!m_ptrST)
	{
		m_ptrST = ObjectImpl<User::ServiceTable>::CreateObjectPtr();
		m_ptrST->Init(m_ptrOM);
	}

	return m_ptrST.AddRefReturn();
}

// UserManager
User::Manager::Manager() :
	LocalAcceptor<Connection>(),
	m_root_handle(ACE_INVALID_HANDLE),
	m_uNextChannelId(1)
{
}

User::Manager::~Manager()
{
	term();
}

int User::Manager::run(u_short uPort)
{
	return USER_MANAGER::instance()->run_event_loop_i(uPort);
}

int User::Manager::run_event_loop_i(u_short uPort)
{
	int ret = init(uPort);
	if (ret != 0)
		return ret;

	// Determine default threads from processor count
	int threads = ACE_OS::num_processors();
	if (threads < 2)
		threads = 2;

	// Spawn off the request threads
	int req_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads,request_worker_fn);
	if (req_thrd_grp_id == -1)
		ret = -1;
	else
	{
		// Spawn off the proactor threads
		int pro_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads-1,proactor_worker_fn);
		if (pro_thrd_grp_id == -1)
			ret = -1;
		else
		{
			//ACE_DEBUG((LM_INFO,ACE_TEXT("OOServer user context has started successfully.")));

			ret = proactor_worker_fn(0);

			// Wait for all the proactor threads to finish
			ACE_Thread_Manager::instance()->wait_grp(pro_thrd_grp_id);
		}

		// Stop handling requests
		RequestHandler<User::Request>::stop();

		// Wait for all the request threads to finish
		ACE_Thread_Manager::instance()->wait_grp(req_thrd_grp_id);
	}

	//ACE_DEBUG((LM_INFO,ACE_TEXT("OOServer user context has stopped.")));

	return ret;
}

int User::Manager::init(u_short uPort)
{
	ACE_SOCK_Connector connector;
	ACE_INET_Addr addr(uPort,(ACE_UINT32)INADDR_LOOPBACK);
	ACE_SOCK_Stream stream;
	ACE_Time_Value wait(5);

	// Connect to the root
	int ret = connector.connect(stream,addr,&wait);
	if (ret != 0)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("connect() failed")));
	}
	else
	{
		// Bind a tcp socket
		ACE_INET_Addr sa((u_short)0,(ACE_UINT32)INADDR_LOOPBACK);
		if ((ret = open(sa)) != 0)
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("acceptor::open() failed")));
		}
		else
		{
			// Get our port number
			int len = sa.get_size ();
			sockaddr* addr = reinterpret_cast<sockaddr*>(sa.get_addr());
			if ((ret = ACE_OS::getsockname(this->get_handle(),addr,&len)) == -1)
			{
				ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Failed to discover local port")));
			}
			else
			{
				sa.set_type(addr->sa_family);
				sa.set_size(len);

				uPort = sa.get_port_number();
				if (stream.send(&uPort,sizeof(uPort),&wait) < static_cast<ssize_t>(sizeof(uPort)))
				{
					ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("send()")));
					ret = -1;
				}
				else
				{
					ret = bootstrap(stream);
				}

				if (ret == 0)
				{
					// Create a new RootConnection
					Root::Connection* pRC;
					ACE_NEW_NORETURN(pRC,Root::Connection(this,ACE_CString()));
					if (!pRC)
					{
						ret = -1;
					}
					else if ((ret=pRC->open(stream.get_handle())) == 0)
					{
						// Stash the root handle
						m_root_handle = stream.get_handle();

						// Clear the handle in the stream, pRC now owns it
						stream.set_handle(ACE_INVALID_HANDLE);
					}
					else
					{
						delete pRC;
					}
				}
			}
		}

		stream.close();
	}

	return ret;
}

void User::Manager::term()
{
	ACE_GUARD_REACTION(ACE_Recursive_Thread_Mutex,guard,m_lock,OOSERVER_THROW_LASTERROR());

	if (m_root_handle != ACE_INVALID_HANDLE)
	{
		ACE_OS::shutdown(m_root_handle,SD_BOTH);
		ACE_OS::closesocket(m_root_handle);
		m_root_handle = ACE_INVALID_HANDLE;
	}
}

int User::Manager::bootstrap(ACE_SOCK_STREAM& stream)
{
	// Talk to the root...
	char sandbox = 0;
	if (stream.recv(&sandbox,sizeof(sandbox)) != sizeof(sandbox))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("recv() failed")),-1);

	bool bSandbox = sandbox ? true : false;

	// Register our service
	try
	{
		ObjectPtr<Remoting::IObjectManager> ptrOM;
		if (!bSandbox)
			ptrOM = get_object_manager(m_root_handle,1);

		ObjectPtr<Activation::IServiceTable> ptrServiceTable;
		ptrServiceTable.Attach(Activation::IServiceTable::GetServiceTable());

		ObjectPtr<ObjectImpl<InterProcessServiceFactory> > ptrOF = ObjectImpl<InterProcessServiceFactory>::CreateObjectPtr();
		ptrOF->Init(ptrOM,this);

		ptrServiceTable->Register(Remoting::OID_InterProcess,Activation::IServiceTable::Default,ptrOF);
	}
	catch (IException* pE)
	{
		pE->Release();
		return -1;
	}

	return 0;
}

void User::Manager::root_connection_closed(const ACE_CString& /*key*/, ACE_HANDLE /*handle*/)
{
	try
	{
		// Stop accepting
		ACE_OS::shutdown(get_handle(),SD_BOTH);

		{
			ACE_GUARD_REACTION(ACE_Recursive_Thread_Mutex,guard,m_lock,OOSERVER_THROW_LASTERROR());

			// Shutdown all client handles
			for (std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.begin();j!=m_mapReverseChannelIds.end();)
			{
				if (j->first == m_root_handle)
				{
					m_mapReverseChannelIds.erase(j++);
				}
				else
				{
					ACE_OS::shutdown(j->first,SD_SEND);
					++j;
				}
			}
		}

		// Wait for everyone to close
		ACE_Time_Value wait(15);
		ACE_Countdown_Time timeout(&wait);
		while (!timeout.stopped())
		{
			ACE_GUARD_REACTION(ACE_Recursive_Thread_Mutex,guard,m_lock,OOSERVER_THROW_LASTERROR());

			if (m_mapReverseChannelIds.empty())
				break;

			timeout.update();
		}
	}
	catch (...)
	{}

	// We close when the root connection closes
	ACE_Proactor::instance()->proactor_end_event_loop();

	term();
}

ACE_THR_FUNC_RETURN User::Manager::proactor_worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)ACE_Proactor::instance()->proactor_run_event_loop();
}

int User::Manager::validate_connection(const ACE_Asynch_Accept::Result& result, const ACE_INET_Addr& remote, const ACE_INET_Addr& local)
{
	// Check we can accept it...
	if (LocalAcceptor<Connection>::validate_connection(result,remote,local) != 0)
		return -1;

	try
	{
		ACE_GUARD_REACTION(ACE_Recursive_Thread_Mutex,guard,m_lock,OOSERVER_THROW_LASTERROR());

		ChannelPair channel = {result.accept_handle(), 0};
		ACE_CDR::UShort uChannelId = m_uNextChannelId++;
		while (uChannelId==0 || m_mapChannelIds.find(uChannelId)!=m_mapChannelIds.end())
		{
			uChannelId = m_uNextChannelId++;
		}
		m_mapChannelIds.insert(std::map<ACE_CDR::UShort,ChannelPair>::value_type(uChannelId,channel));
		m_mapReverseChannelIds.insert(std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::value_type(result.accept_handle(),std::map<ACE_CDR::UShort,ACE_CDR::UShort>()));
	}
	catch (...)
	{
		return -1;
	}

	return 0;
}

int User::Manager::enqueue_root_request(ACE_InputCDR* input, ACE_HANDLE handle)
{
	User::Request* req;
	ACE_NEW_RETURN(req,Request(handle,input),-1);

	req->m_bRoot = true;

	int ret = enqueue_request(req);
	if (ret <= 0)
		delete req;

	return ret;
}

int User::Manager::enqueue_user_request(ACE_InputCDR* input, ACE_HANDLE handle)
{
	Request* req;
	ACE_NEW_RETURN(req,Request(handle,input),-1);

	req->m_bRoot = false;

	int ret = USER_MANAGER::instance()->enqueue_request(req);
	if (ret <= 0)
		delete req;

	return ret;
}

void User::Manager::user_connection_closed(ACE_HANDLE handle)
{
	USER_MANAGER::instance()->user_connection_closed_i(handle);
}

void User::Manager::user_connection_closed_i(ACE_HANDLE handle)
{
	ACE_OS::closesocket(handle);

	ACE_GUARD_REACTION(ACE_Recursive_Thread_Mutex,guard,m_lock,OOSERVER_THROW_LASTERROR());

	try
	{
		std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(handle);
		if (j!=m_mapReverseChannelIds.end())
		{
			for (std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator k=j->second.begin();k!=j->second.end();++k)
			{
				m_mapChannelIds.erase(k->second);
			}
			m_mapReverseChannelIds.erase(j);
		}
	}
	catch (...)
	{}
}

ACE_THR_FUNC_RETURN User::Manager::request_worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)USER_MANAGER::instance()->pump_requests();
}

void User::Manager::process_request(Request* request, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	if (dest_channel_id == 0)
	{
		if (request->m_bRoot && src_channel_id==0)
			process_root_request(request->handle(),*request->input(),trans_id,request_deadline);
		else
			process_request(request->handle(),*request->input(),src_channel_id,trans_id,request_deadline);
	}
	else
	{
		// Forward to the correct channel...
		forward_request(request,dest_channel_id,src_channel_id,trans_id,request_deadline);
	}

	delete request;
}

void User::Manager::process_root_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	ACE_CDR::ULong op_code = ACE_CDR::ULong(-1);
    request >> op_code;

	//ACE_DEBUG((LM_DEBUG,ACE_TEXT("User context: Process root request %u"),op_code));

	if (!request.good_bit())
		return;

	switch (op_code)
	{
	default:
		;
	}
}

void User::Manager::process_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	//ACE_DEBUG((LM_DEBUG,ACE_TEXT("User context: Process request %u from %u"),trans_id,src_channel_id));

	// Init the error stream
	ACE_OutputCDR error;

	try
	{
		// Find and/or create the object manager associated with src_channel_id
		ObjectPtr<Remoting::IObjectManager> ptrOM;
		ptrOM = get_object_manager(handle,src_channel_id);

		// Convert deadline time to #msecs
		ACE_Time_Value wait(*request_deadline - ACE_OS::gettimeofday());
		if (wait <= ACE_Time_Value::zero)
		{
			if (trans_id != 0)
			{
				// Error code 1 - Request timed out
				error.write_octet(1);
				if (send_response(handle,0,trans_id,error.begin(),request_deadline) != 0)
					OOSERVER_THROW_LASTERROR();
			}
			return;
		}

		ACE_UINT64 msecs = 0;
		static_cast<const ACE_Time_Value>(wait).msec(msecs);
		if (msecs > ACE_UINT32_MAX)
			msecs = ACE_UINT32_MAX;

		// Wrap up the request
		ObjectPtr<ObjectImpl<InputCDR> > ptrRequest;
		ptrRequest = ObjectImpl<InputCDR>::CreateObjectPtr();
		ptrRequest->init(request);

		// Create a response if required
		ObjectPtr<ObjectImpl<OutputCDRImpl> > ptrResponse;
		if (trans_id != 0)
		{
			ptrResponse = ObjectImpl<OutputCDRImpl>::CreateObjectPtr();
			ptrResponse->WriteByte(0);
		}

		ObjectPtr<Remoting::ICallContext> ptrPrevCallContext;
		void* TODO; // Setup the CallContext... Use a self-destructing class!

		try
		{
			ptrOM->Invoke(ptrRequest,ptrResponse,static_cast<uint32_t>(msecs));
		}
		catch (IException* pInner)
		{
			// Make sure we release the exception
			ObjectPtr<IException> ptrInner;
			ptrInner.Attach(pInner);

			ACE_ERROR((LM_ERROR,ACE_TEXT("Invoke failed: %s\nAt: %s"),(LPCTSTR)pInner->Description(),(LPCTSTR)pInner->Source()));

			// Reply with an exception if we can send replies...
			if (trans_id != 0)
			{
				// Dump the previous output and create a fresh output
				ptrResponse = ObjectImpl<OutputCDRImpl>::CreateObjectPtr();
				ptrResponse->WriteByte(0);
				ptrResponse->WriteBoolean(false);

				// Write the exception onto the wire
				ObjectPtr<MetaInfo::IWireManager> ptrWM(ptrOM);
				MetaInfo::wire_write(ptrWM,ptrResponse,pInner,pInner->ActualIID());
			}
		}

		if (trans_id != 0)
		{
			if (send_response(handle,0,trans_id,static_cast<ACE_Message_Block*>(ptrResponse->GetMessageBlock()),request_deadline) != 0)
				OOSERVER_THROW_LASTERROR();
		}
	}
	catch (IException* pOuter)
	{
		// Make sure we release the exception
		ObjectPtr<IException> ptrOuter;
		ptrOuter.Attach(pOuter);

		if (trans_id != 0)
		{
			// Error code 2 - Exception raw
			error.write_octet(2);
			string_t strDesc = pOuter->Description();
			error.write_string(static_cast<ACE_CDR::ULong>(strDesc.Length()),strDesc);
			string_t strSrc = pOuter->Source();
			error.write_string(static_cast<ACE_CDR::ULong>(strSrc.Length()),strSrc);

			send_response(handle,0,trans_id,error.begin(),request_deadline);
		}
	}
}

void User::Manager::forward_request(Request* request, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	ChannelPair dest_channel;
	ACE_CDR::UShort reply_channel_id;
	try
	{
		ACE_GUARD_REACTION(ACE_Recursive_Thread_Mutex,guard,m_lock,OOSERVER_THROW_LASTERROR());

		// Find the destination channel
		std::map<ACE_CDR::UShort,ChannelPair>::iterator i=m_mapChannelIds.find(dest_channel_id);
		if (i == m_mapChannelIds.end())
			return;
		dest_channel = i->second;

		// Find the local channel id that matches src_channel_id
		std::map<ACE_HANDLE,std::map<ACE_CDR::UShort,ACE_CDR::UShort> >::iterator j=m_mapReverseChannelIds.find(request->handle());
		if (j==m_mapReverseChannelIds.end())
			return;

		std::map<ACE_CDR::UShort,ACE_CDR::UShort>::iterator k = j->second.find(src_channel_id);
		if (k == j->second.end())
		{
			ChannelPair channel = {request->handle(), src_channel_id};
			reply_channel_id = m_uNextChannelId++;
			while (reply_channel_id==0 || m_mapChannelIds.find(reply_channel_id)!=m_mapChannelIds.end())
			{
				reply_channel_id = m_uNextChannelId++;
			}
			m_mapChannelIds.insert(std::map<ACE_CDR::UShort,ChannelPair>::value_type(reply_channel_id,channel));

			k = j->second.insert(std::map<ACE_CDR::UShort,ACE_CDR::UShort>::value_type(src_channel_id,reply_channel_id)).first;
		}
		reply_channel_id = k->second;
	}
	catch (...)
	{
		return;
	}

	//ACE_DEBUG((LM_DEBUG,ACE_TEXT("User context: Forwarding request from %u(%u) to %u(%u)"),reply_channel_id,src_channel_id,dest_channel_id,dest_channel.channel));

	if (trans_id == 0)
	{
		RequestHandler<Request>::send_asynch(dest_channel.handle,dest_channel.channel,reply_channel_id,request->input()->start(),request_deadline);
	}
	else
	{
		Request* response;
		if (RequestHandler<Request>::send_synch(dest_channel.handle,dest_channel.channel,reply_channel_id,request->input()->start(),response,request_deadline) == 0)
		{
			send_response(request->handle(),src_channel_id,trans_id,response->input()->start(),request_deadline);
			delete response;
		}
	}
}

void User::Manager::send_asynch(ACE_HANDLE handle, ACE_CDR::UShort dest_channel_id, const ACE_Message_Block* request, ACE_Time_Value* wait)
{
	if (handle == ACE_INVALID_HANDLE)
		handle = m_root_handle;

	ACE_Time_Value deadline(5);
	if (wait)
		deadline = ACE_OS::gettimeofday() + *wait;

	if (RequestHandler<Request>::send_asynch(handle,dest_channel_id,0,request,&deadline) != 0)
		OOSERVER_THROW_LASTERROR();
}

ACE_InputCDR User::Manager::send_synch(ACE_HANDLE handle, ACE_CDR::UShort dest_channel_id, const ACE_Message_Block* request, ACE_Time_Value* wait)
{
	if (handle == ACE_INVALID_HANDLE)
		handle = m_root_handle;

	ACE_Time_Value deadline(5);
	if (wait)
		deadline = ACE_OS::gettimeofday() + *wait;

	Request* pResponse;
	int err = RequestHandler<Request>::send_synch(handle,dest_channel_id,0,request,pResponse,&deadline);
	if (err != 0)
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = *pResponse->input();
	delete pResponse;
	return response;
}

ObjectPtr<Remoting::IObjectManager> User::Manager::get_object_manager(ACE_HANDLE handle, ACE_CDR::UShort channel_id)
{
	ObjectPtr<Remoting::IObjectManager> ptrOM;
	try
	{
		ACE_GUARD_REACTION(ACE_Recursive_Thread_Mutex,guard,m_lock,OOSERVER_THROW_LASTERROR());

		std::map<ACE_HANDLE,ObjectPtr<Remoting::IObjectManager> >::iterator i=m_mapOMs.find(handle);
		if (i == m_mapOMs.end())
		{
			// Create a new channel
			ObjectPtr<ObjectImpl<Channel> > ptrChannel = ObjectImpl<Channel>::CreateObjectPtr();
			ptrChannel->init(this,handle,channel_id);

			// Create a new OM
			ptrOM = ObjectPtr<Remoting::IObjectManager>(OID_StdObjectManager,Activation::InProcess);

			// Associate it with the channel
			ptrOM->Connect(ptrChannel);

			// And add to the map
			m_mapOMs.insert(std::map<ACE_HANDLE,ObjectPtr<Remoting::IObjectManager> >::value_type(handle,ptrOM));
		}
		else
		{
			ptrOM = i->second;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}

	return ptrOM;
}
