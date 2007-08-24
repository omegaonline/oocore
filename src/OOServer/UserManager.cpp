#include "OOServer.h"

#include "./UserManager.h"
#include "./Channel.h"
#include "./UserServiceTable.h"
#include "./UserRegistry.h"

int UserMain(const ACE_WString& strPipe)
{
	if (ACE_LOG_MSG->open(L"OOServer",ACE_Log_Msg::SYSLOG,L"OOServer") != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"Error opening logger"),-1);

	return User::Manager::run(strPipe);
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
		void CreateInstance(IObject* pOuter, const guid_t& iid, IObject*& pObject);
	};

}

void User::InterProcessServiceFactory::CreateInstance(IObject* pOuter, const guid_t& iid, IObject*& pObject)
{
	if (pOuter)
		throw Activation::INoAggregationException::Create(Remoting::OID_InterProcess);

	ObjectPtr<SingletonObjectImpl<InterProcessService> > ptrIPS = SingletonObjectImpl<InterProcessService>::CreateInstancePtr();
	ptrIPS->Init(m_ptrOM,m_pManager);

	pObject = ptrIPS->QueryInterface(iid);
	if (!pObject)
		throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
}

Registry::IRegistryKey* User::InterProcessService::GetRegistry()
{
	if (!m_ptrReg)
	{
		// Double lock for speed
		OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

		if (!m_ptrReg)
		{

			m_ptrReg = ObjectImpl<User::Registry::BaseKey>::CreateInstancePtr();
			m_ptrReg->Init(m_pManager,!m_ptrOM);
		}
	}

	return m_ptrReg.AddRefReturn();
}

Activation::IServiceTable* User::InterProcessService::GetServiceTable()
{
	if (!m_ptrST)
	{
		// Double lock for speed
		OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

		if (!m_ptrST)
		{
			m_ptrST = ObjectImpl<User::ServiceTable>::CreateInstancePtr();
			m_ptrST->Init(m_ptrOM);
		}
	}

	return m_ptrST.AddRefReturn();
}

// UserManager
User::Manager::Manager() :
	m_root_channel(0)
{
}

User::Manager::~Manager()
{
}

int User::Manager::run(const ACE_WString& strPipe)
{
	return USER_MANAGER::instance()->run_event_loop_i(strPipe);
}

int User::Manager::run_event_loop_i(const ACE_WString& strPipe)
{
	int ret = -1;

	// Determine default threads from processor count
	int threads = ACE_OS::num_processors();
	if (threads < 2)
		threads = 2;

	// Spawn off the request threads
	int req_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads,request_worker_fn,this);
	if (req_thrd_grp_id == -1)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"Error spawning threads"),-1);
	else
	{
		// Spawn off the proactor threads
		int pro_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads,proactor_worker_fn);
		if (pro_thrd_grp_id == -1)
			ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"Error spawning threads"),-1);
		else
		{
			if (init(strPipe))
			{
				// Wait for stop
				ret = ACE_Reactor::instance()->run_event_loop();

				// Wait for all the request threads to finish
				ACE_Thread_Manager::instance()->wait_grp(pro_thrd_grp_id);

				// Wait for all the request threads to finish
				ACE_Thread_Manager::instance()->wait_grp(req_thrd_grp_id);
			}
		}	
	}

	return ret;
}

bool User::Manager::init(const ACE_WString& strPipe)
{
	ACE_CDR::UShort sandbox_channel = 0;

	// Connect to the root
	ACE_Time_Value wait(5);
	Root::MessagePipe pipe;
	if (Root::MessagePipe::connect(pipe,strPipe,&wait) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %x\n",GetLastError()),false);

	// Talk to the root...
	if (pipe.recv(&sandbox_channel,sizeof(sandbox_channel)) != static_cast<ssize_t>(sizeof(sandbox_channel)))
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"ACE_OS::getsockname() failed"),false);

	// Create a new MessageConnection
	Root::MessageConnection* pMC;
	ACE_NEW_RETURN(pMC,Root::MessageConnection(this),false);

	m_root_channel = pMC->open(pipe);
	if (m_root_channel == 0)
	{
		delete pMC;
		return false;
	}

	// Now bootstrap
	if (!bootstrap(sandbox_channel))
		return false;

	// Invent a new pipe name..
	ACE_WString strNewPipe = Root::MessagePipe::unique_name(L"oo");

	// Now start accepting client connections
	if (start(strNewPipe.c_str()) != 0)
		return false;

	size_t uLen = strNewPipe.length()+1;
	if (pipe.send(&uLen,sizeof(uLen)) != static_cast<ssize_t>(sizeof(uLen)))
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"pipe.send() failed"),false);

	// Then send back our port number
	if (pipe.send(strNewPipe.c_str(),uLen*sizeof(wchar_t)) != static_cast<ssize_t>(uLen*sizeof(wchar_t)))
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"pipe.send() failed"),false);

	return true;
}

bool User::Manager::bootstrap(ACE_CDR::UShort sandbox_channel)
{
	bool bSandbox = (sandbox_channel == 0);

	// Register our service
	try
	{
		OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOM;
		if (!bSandbox)
		{
			sandbox_channel = add_routing(m_root_channel,sandbox_channel);
			if (!sandbox_channel)
				return false;

			ptrOM = get_object_manager(sandbox_channel);
		}

		ObjectPtr<ObjectImpl<InterProcessServiceFactory> > ptrOF = ObjectImpl<InterProcessServiceFactory>::CreateInstancePtr();
		ptrOF->Init(ptrOM,this);

		ObjectPtr<Activation::IServiceTable> ptrServiceTable;
		ptrServiceTable.Attach(Activation::IServiceTable::GetServiceTable());
		ptrServiceTable->Register(Remoting::OID_InterProcess,Activation::IServiceTable::Default,ptrOF);
	}
	catch (IException* pE)
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] Exception thrown: %W - %W\n",pE->Description().c_str(),pE->Source().c_str()));
		pE->Release();
		return false;
	}

	return true;
}

void User::Manager::end_event_loop()
{
	// Stop accepting new clients
	stop_accepting();
	
	// Stop the reactor
	ACE_Reactor::instance()->end_reactor_event_loop();

	// Close the user processes
	close_channels();

	// Stop the proactor
	ACE_Proactor::instance()->end_event_loop();

	// Stop the MessageHandler
	stop();
}

void User::Manager::close_channels()
{
	try
	{
		OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		for (std::map<ACE_CDR::UShort,OTL::ObjectPtr<Omega::Remoting::IObjectManager> >::reverse_iterator i=m_mapOMs.rbegin();i!=m_mapOMs.rend();++i)
		{
			try
			{
				i->second->Disconnect();
			}
			catch (IException* pE)
			{
				pE->Release();
			}
			catch (...)
			{
			}

			get_channel_pipe(i->first).close();
		}
	}
	catch (...)
	{
	}

	try
	{
		OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		m_mapOMs.clear();
	}
	catch (...)
	{
	}
}

ACE_THR_FUNC_RETURN User::Manager::proactor_worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)ACE_Proactor::instance()->proactor_run_event_loop();
}

ACE_THR_FUNC_RETURN User::Manager::request_worker_fn(void* pParam)
{
	static_cast<Manager*>(pParam)->pump_requests();
	return 0;
}

void User::Manager::process_request(const Root::MessagePipe& /*handle*/, ACE_InputCDR& request, ACE_CDR::UShort src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::UShort attribs)
{
	if (src_channel_id == m_root_channel)
	{
		process_root_request(request,src_channel_id,src_thread_id,deadline,attribs);
	}
	else
	{
		// Find and/or create the object manager associated with src_channel_id
		OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOM = get_object_manager(src_channel_id);

		// Process the message...
		process_user_request(ptrOM,request,src_channel_id,src_thread_id,deadline,attribs);
	}
}

void User::Manager::process_root_request(ACE_InputCDR& request, ACE_CDR::UShort /*src_channel_id*/, ACE_CDR::UShort /*src_thread_id*/, const ACE_Time_Value& /*deadline*/, ACE_CDR::UShort /*attribs*/)
{
	Root::RootOpCode_t op_code;
	request >> op_code;

	//ACE_DEBUG((LM_DEBUG,L"User context: Process root request %u",op_code));

	if (!request.good_bit())
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"Bad request"));
		return;
	}

	switch (op_code)
	{
	case Root::End:
		// We have been told to end!
		end_event_loop();
		return;

	default:
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] Bad request op_code\n"));
		return;
	}
}

void User::Manager::process_user_request(ObjectPtr<Remoting::IObjectManager> ptrOM, const ACE_InputCDR& request, ACE_CDR::UShort src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::UShort attribs)
{
	//ACE_DEBUG((LM_DEBUG,L"User context: Process request %u from %u",trans_id,src_channel_id));

	try
	{
		// Wrap up the request
		ObjectPtr<ObjectImpl<InputCDR> > ptrRequest;
		ptrRequest = ObjectImpl<InputCDR>::CreateInstancePtr();
		ptrRequest->init(request);

		// Create a response if required
		ObjectPtr<ObjectImpl<OutputCDR> > ptrResponse;
		if (!(attribs & Remoting::asynchronous))
		{
			ptrResponse = ObjectImpl<OutputCDR>::CreateInstancePtr();
			ptrResponse->WriteByte(0);
		}

		ObjectPtr<Remoting::ICallContext> ptrPrevCallContext;
		void* TODO; // TODO Setup the CallContext... Use a self-destructing class!

		// Check timeout
		ACE_Time_Value now = ACE_OS::gettimeofday();
		if (deadline <= now)
			return;

		// Convert deadline time to #msecs
		ACE_Time_Value wait = deadline - now;
		ACE_UINT64 msecs = 0;
		static_cast<const ACE_Time_Value>(wait).msec(static_cast<ACE_UINT64&>(msecs));
		if (msecs > ACE_UINT32_MAX)
			msecs = ACE_UINT32_MAX;

		try
		{
			ptrOM->Invoke(ptrRequest,ptrResponse,static_cast<Omega::uint32_t>(msecs));
		}
		catch (IException* pInner)
		{
			// Make sure we release the exception
			ObjectPtr<IException> ptrInner;
			ptrInner.Attach(pInner);

			// Reply with an exception if we can send replies...
			if (!(attribs & Remoting::asynchronous))
			{
				// Dump the previous output and create a fresh output
				ptrResponse = ObjectImpl<OutputCDR>::CreateInstancePtr();
				ptrResponse->WriteByte(0);
				ptrResponse->WriteBoolean(false);

				// Write the exception onto the wire
				ObjectPtr<System::MetaInfo::IWireManager> ptrWM(ptrOM);
				System::MetaInfo::wire_write(ptrWM,ptrResponse,pInner,pInner->ActualIID());
			}
		}

		if (!(attribs & Remoting::asynchronous))
		{
		    ACE_Message_Block* mb = static_cast<ACE_Message_Block*>(ptrResponse->GetMessageBlock());
			send_response(src_channel_id,src_thread_id,mb,deadline,attribs);
            mb->release();
		}
	}
	catch (IException* pOuter)
	{
		// Make sure we release the exception
		ObjectPtr<IException> ptrOuter;
		ptrOuter.Attach(pOuter);

		if (!(attribs & Remoting::asynchronous))
		{
			ACE_OutputCDR error;

			// Error code 1 - Exception raw
			error.write_octet(1);
			error.write_string(string_t_to_utf8(pOuter->Description()));
			error.write_string(string_t_to_utf8(pOuter->Source()));

			send_response(src_channel_id,src_thread_id,error.begin(),deadline,attribs);
		}
	}
}

OTL::ObjectPtr<Omega::Remoting::IObjectManager> User::Manager::get_object_manager(ACE_CDR::UShort src_channel_id)
{
	OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOM;
	try
	{
		// Lookup existing..
		{
			OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

            std::map<ACE_CDR::UShort,OTL::ObjectPtr<Omega::Remoting::IObjectManager> >::iterator i=m_mapOMs.find(src_channel_id);
			if (i != m_mapOMs.end())
				ptrOM = i->second;
		}

		if (!ptrOM)
		{
			// Create a new channel
			ObjectPtr<ObjectImpl<Channel> > ptrChannel = ObjectImpl<Channel>::CreateInstancePtr();
			ptrChannel->init(this,src_channel_id);

			// Create a new OM
			ptrOM = ObjectPtr<Remoting::IObjectManager>(Remoting::OID_StdObjectManager,Activation::InProcess);

			// Associate it with the channel
			ptrOM->Connect(ptrChannel);

			// And add to the map
			OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::pair<std::map<ACE_CDR::UShort,OTL::ObjectPtr<Omega::Remoting::IObjectManager> >::iterator,bool> p = m_mapOMs.insert(std::map<ACE_CDR::UShort,OTL::ObjectPtr<Omega::Remoting::IObjectManager> >::value_type(src_channel_id,ptrOM));
			if (!p.second)
				ptrOM = p.first->second;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}

	return ptrOM;
}

ACE_InputCDR User::Manager::sendrecv_root(const ACE_OutputCDR& request)
{
	ACE_InputCDR* response = 0;
	if (!send_request(m_root_channel,request.begin(),response,15000,0))
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR ret = *response;
	delete response;
	return ret;
}
