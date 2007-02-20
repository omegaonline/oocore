#include "OOServer.h"

#include "./UserManager.h"
#include "./Channel.h"

int UserMain(u_short uPort)
{
	return UserManager::run_event_loop(uPort);
}

using namespace Omega;
using namespace OTL;

namespace
{
	class InterProcessServiceImpl :
		public ObjectBase,
		public Remoting::IInterProcessService
	{
	public:
		
		BEGIN_INTERFACE_MAP(InterProcessServiceImpl)
			INTERFACE_ENTRY(Remoting::IInterProcessService)
		END_INTERFACE_MAP()

	public:
		Registry::IRegistryKey* GetRegistryKey();
		Activation::IServiceTable* GetServiceTable();
	};

	class InterProcessServiceFactoryImpl :
		public ObjectBase,
		public Activation::IObjectFactory
	{
	public:
		BEGIN_INTERFACE_MAP(InterProcessServiceFactoryImpl)
			INTERFACE_ENTRY(Activation::IObjectFactory)
		END_INTERFACE_MAP()

	public:
		void CreateObject(IObject* pOuter, const Omega::guid_t& iid, IObject*& pObject);
	};
}

void InterProcessServiceFactoryImpl::CreateObject(IObject* pOuter, const Omega::guid_t& iid, IObject*& pObject)
{
	if (pOuter)
		Omega::Activation::INoAggregationException::Throw(Remoting::OID_InterProcess);

	pObject = SingletonObjectImpl<InterProcessServiceImpl>::CreateObjectPtr()->QueryInterface(iid);
	if (!pObject)
		Omega::INoInterfaceException::Throw(iid,OMEGA_FUNCNAME);
}

Registry::IRegistryKey* InterProcessServiceImpl::GetRegistryKey()
{
	void* TODO;
	return 0;
}

Activation::IServiceTable* InterProcessServiceImpl::GetServiceTable()
{
	void* TODO;
	return 0;
}

// UserManager
UserManager::UserManager() : 
	LocalAcceptor<UserConnection>(),
	m_root_handle(ACE_INVALID_HANDLE),
	m_uNextChannelId(1)
{
}

UserManager::~UserManager()
{
	term();
}

int UserManager::run_event_loop(u_short uPort)
{
	return USER_MANAGER::instance()->run_event_loop_i(uPort);
}

int UserManager::run_event_loop_i(u_short uPort)
{
	int ret = init(uPort);
	if (ret != 0)
		return ret;

	// Register our service
	try
	{
		ObjectPtr<Activation::IServiceTable> ptrServiceTable;
		ptrServiceTable.Attach(Activation::IServiceTable::GetServiceTable());

		ObjectPtr<ObjectImpl<InterProcessServiceFactoryImpl> > ptrOF = ObjectImpl<InterProcessServiceFactoryImpl>::CreateObjectPtr();
		ptrServiceTable->Register(Remoting::OID_InterProcess,Activation::IServiceTable::Default,ptrOF);
	}
	catch (IException* pE)
	{
		pE->Release();
		return -1;
	}

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
			// Run out bootstrap functions
			ret = bootstrap();
			if (ret != 0)
				ACE_OS::shutdown(m_root_handle,SD_BOTH);
			else
				ret = proactor_worker_fn(0);

			// Wait for all the proactor threads to finish
			ACE_Thread_Manager::instance()->wait_grp(pro_thrd_grp_id);
		}

		// Stop handling requests
		RequestHandler<UserRequest>::stop();
		
		// Wait for all the request threads to finish
		ACE_Thread_Manager::instance()->wait_grp(req_thrd_grp_id);
	}

	return ret;
}

int UserManager::init(u_short uPort)
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
				if (stream.send(&uPort,sizeof(uPort),&wait) < sizeof(uPort))
				{
					ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("send()")));
					ret = -1;
				}
				else
				{
					// Create a new RootConnection
					RootConnection*	pRC;
					ACE_NEW_NORETURN(pRC,RootConnection(this,ACE_CString()));
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

void UserManager::term()
{
	ACE_GUARD(ACE_Recursive_Thread_Mutex,guard,m_lock);

	if (m_root_handle != ACE_INVALID_HANDLE)
	{
		ACE_OS::closesocket(m_root_handle);
		m_root_handle = ACE_INVALID_HANDLE;
	}
}

int UserManager::bootstrap()
{
	// Send a test message
	ACE_OutputCDR request;
	request.write_ulong(1);
	request.write_string("Hello!");

	ACE_Time_Value wait(60);
	UserRequest* response;	
	int ret = send_synch(m_root_handle,0,request,response,&wait);
	if (ret == 0)
	{
		ACE_CString strResponse;
		if (response->input()->read_string(strResponse))
		{
			printf("Response: %s\n",strResponse.c_str());
		}
	}

	return 0;
}

void UserManager::root_connection_closed(const ACE_CString& /*key*/, ACE_HANDLE /*handle*/)
{
	try
	{
		// Stop accepting
		ACE_OS::closesocket(handle());

		{
			ACE_GUARD(ACE_Recursive_Thread_Mutex,guard,m_lock);

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

		// Give everyone a chance to shut down
		void* TODO; // Put a timeout here!
		for (;;)
		{
			ACE_GUARD(ACE_Recursive_Thread_Mutex,guard,m_lock);

			if (m_mapReverseChannelIds.empty())
				break;
		}
	}
	catch (...)
	{}

	// We close when the root connection closes
	ACE_Proactor::instance()->proactor_end_event_loop();

	term();
}

ACE_THR_FUNC_RETURN UserManager::proactor_worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)ACE_Proactor::instance()->proactor_run_event_loop();
}

int UserManager::validate_connection(const ACE_Asynch_Accept::Result& result, const ACE_INET_Addr& remote, const ACE_INET_Addr& local)
{
	// Check we can accept it...
	if (LocalAcceptor<UserConnection>::validate_connection(result,remote,local) != 0)
		return -1;

	try
	{
		ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,guard,m_lock,-1);

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

int UserManager::enqueue_root_request(ACE_InputCDR* input, ACE_HANDLE handle)
{
	UserRequest* req;
	ACE_NEW_RETURN(req,UserRequest(handle,input),-1);

	req->m_bRoot = true;
	
	int ret = enqueue_request(req);
	if (ret <= 0)
		delete req;

	return ret;
}

int UserManager::enqueue_user_request(ACE_InputCDR* input, ACE_HANDLE handle)
{
	UserRequest* req;
	ACE_NEW_RETURN(req,UserRequest(handle,input),-1);

	req->m_bRoot = false;
	
	int ret = USER_MANAGER::instance()->enqueue_request(req);
	if (ret <= 0)
		delete req;

	return ret;
}

void UserManager::user_connection_closed(ACE_HANDLE handle)
{
	USER_MANAGER::instance()->user_connection_closed_i(handle);
}

void UserManager::user_connection_closed_i(ACE_HANDLE handle)
{
	ACE_OS::closesocket(handle);

	ACE_GUARD(ACE_Recursive_Thread_Mutex,guard,m_lock);

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

ACE_THR_FUNC_RETURN UserManager::request_worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)USER_MANAGER::instance()->pump_requests();
}

void UserManager::process_request(UserRequest* request, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	if (dest_channel_id == 0)
	{
		if (request->m_bRoot)
			process_root_request(request->handle(),*request->input(),trans_id,request_deadline);
		else 
			process_request(request->handle(),*request->input(),trans_id,request_deadline);
	}
	else
	{
		// Forward to the correct channel...
		forward_request(request,dest_channel_id,src_channel_id,trans_id,request_deadline);
	}

	delete request;
}

void UserManager::process_root_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	ACE_CDR::ULong op_code;
	request >> op_code;
	if (!request.good_bit())
		return;

	switch (op_code)
	{
	default:
		;
	}
}

void UserManager::process_request(ACE_HANDLE handle, ACE_InputCDR& request, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	// Init the error stream
	ACE_OutputCDR error;

	// Find and/or create the object manager associated with src_channel_id
	ObjectPtr<Remoting::IObjectManager> ptrOM;
	try
	{
		ptrOM = get_object_manager(handle);
	}
	catch (IException* pE)
	{
		pE->Release();
		if (trans_id != 0)
		{
			// Error code 1 - Failed to resolve ObjectManager
			error.write_ulong(1);
			send_response(handle,0,trans_id,error.begin(),request_deadline);
		}
		return;
	}

	// Convert deadline time to #msecs
	ACE_Time_Value wait(*request_deadline - ACE_OS::gettimeofday());
	if (wait <= ACE_Time_Value::zero)
	{
		if (trans_id != 0)
		{
			// Error code 2 - Request timed out
			error.write_ulong(2);
			send_response(handle,0,trans_id,error.begin(),request_deadline);
		}
		return;
	}
	
	ACE_UINT64 msecs = 0;
	static_cast<const ACE_Time_Value>(wait).msec(msecs);
	if (msecs > ACE_UINT32_MAX)
		msecs = ACE_UINT32_MAX;

	// Wrap up the request
	ObjectPtr<ObjectImpl<OOServer::InputCDR> > ptrRequest;
	try
	{
		ptrRequest = ObjectImpl<OOServer::InputCDR>::CreateObjectPtr();
		ptrRequest->init(request);
	}
	catch (IException* pE)
	{
		pE->Release();
		if (trans_id != 0)
		{
			// Error code 3 - Failed to wrap request
			error.write_ulong(3);
			send_response(handle,0,trans_id,error.begin(),request_deadline);
		}
		return;
	}

	// Create a response if required
	ObjectPtr<ObjectImpl<OOServer::OutputCDR> > ptrResponse;
	if (trans_id != 0)
	{
		try
		{
			ptrResponse = ObjectImpl<OOServer::OutputCDR>::CreateObjectPtr();
		}
		catch (IException* pE)
		{
			pE->Release();
			if (trans_id != 0)
			{
				// Error code 4 - Failed to create response
				error.write_ulong(4);
				send_response(handle,0,trans_id,error.begin(),request_deadline);
			}
			return;
		}
	}

	ObjectPtr<Remoting::ICallContext> ptrPrevCallContext;
	
	void* TODO; // Setup the CallContext...

	try
	{

		ptrOM->Invoke(ptrRequest,ptrResponse,static_cast<uint32_t>(msecs));
	}
	catch (IException* pE)
	{
		// Make sure we release the exception
		ObjectPtr<IException> ptrE;
		ptrE.Attach(pE);

		// Reply with an exception if we can send replies...
		if (trans_id != 0)
		{
			// Dump the previous output and create a fresh output
			try
			{
				ptrResponse = ObjectImpl<OOServer::OutputCDR>::CreateObjectPtr();
			}
			catch (IException* pE)
			{
				pE->Release();
				if (trans_id != 0)
				{
					// Error code 4 - Failed to create response
					error.write_ulong(4);
					send_response(handle,0,trans_id,error.begin(),request_deadline);
				}
				return;
			}

			try
			{
				// Mark that we have failed
				ptrResponse->WriteBoolean(false);

				// Write the exception onto the wire
				ObjectPtr<MetaInfo::IWireManager> ptrWM(ptrOM);
				MetaInfo::wire_write(ptrWM,ptrResponse,pE,pE->ActualIID());
			}
			catch (IException* pE)
			{
				pE->Release();
				if (trans_id != 0)
				{
					// Error code 5 - Failed to marshal exception
					error.write_ulong(5);
					send_response(handle,0,trans_id,error.begin(),request_deadline);
				}
				return;
			}
		}
	}

	void* TODO2; // Restore CallContext

	if (trans_id != 0)
	{
		if (send_response(handle,0,trans_id,ptrResponse->GetMessageBlock(),request_deadline) != 0)
		{
			// Error code 6 - Failed to send response
			error.write_ulong(6);
			send_response(handle,0,trans_id,error.begin(),request_deadline);
		}
	}
}

void UserManager::forward_request(UserRequest* request, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline)
{
	ChannelPair dest_channel;
	ACE_CDR::UShort reply_channel_id;
	try
	{
		ACE_GUARD(ACE_Recursive_Thread_Mutex,guard,m_lock);

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

	if (trans_id == 0)
	{
		RequestHandler<UserRequest>::send_asynch(dest_channel.handle,dest_channel.channel,reply_channel_id,request->input()->start(),request_deadline);
	}
	else
	{
		UserRequest* response;
		if (RequestHandler<UserRequest>::send_synch(dest_channel.handle,dest_channel.channel,reply_channel_id,request->input()->start(),response,request_deadline) == 0)
		{
			send_response(request->handle(),src_channel_id,trans_id,response->input()->start(),request_deadline);
			delete response;
		}
	}
}

int UserManager::send_asynch(ACE_HANDLE handle, ACE_CDR::UShort dest_channel_id, const ACE_OutputCDR& request, ACE_Time_Value* wait)
{
	ACE_Time_Value deadline(5);
	if (wait)
		deadline = ACE_OS::gettimeofday() + *wait;

	return RequestHandler<UserRequest>::send_asynch(handle,dest_channel_id,0,request.begin(),&deadline);
}

int UserManager::send_synch(ACE_HANDLE handle, ACE_CDR::UShort dest_channel_id, const ACE_OutputCDR& request, UserRequest*& response, ACE_Time_Value* wait)
{
	ACE_Time_Value deadline(5);
	if (wait)
		deadline = ACE_OS::gettimeofday() + *wait;

	return RequestHandler<UserRequest>::send_synch(handle,dest_channel_id,0,request.begin(),response,&deadline);
}

ObjectPtr<Remoting::IObjectManager> UserManager::get_object_manager(ACE_HANDLE handle)
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
			ptrChannel->init(this,handle);

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
