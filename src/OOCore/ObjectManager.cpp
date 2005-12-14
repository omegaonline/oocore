#include "./ObjectManager.h"

#include "./Engine.h"
#include "./Proxy_Stub_Factory.h"
#include "./PassThruStub.h"

DEFINE_IID(OOCore::Impl::RemoteObjectFactory,B1BC71BE-4DCC-4f0f-8483-A75D35126D2A);

OOCore::ObjectManager::ObjectManager() :
	m_bIsAcceptor(false),
	m_bOpened(false),
	m_next_trans_id(static_cast<OOObject::uint32_t>(ACE_OS::rand()))
{
}

OOCore::ObjectManager::~ObjectManager()
{
}

int 
OOCore::ObjectManager::Open(Transport* transport, const bool AsAcceptor)
{
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	if (m_ptrTransport)
	{
		errno = EISCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Calling open repeatedly on an ObjectManager!\n")),-1);
	}

	m_ptrTransport = transport;

	int res;
	if (AsAcceptor)
		res = accept();
	else
		res = connect();

	if (res!=0)
		m_ptrTransport = 0;

	return res;
}

bool 
OOCore::ObjectManager::await_close(void * p)
{
	ObjectManager* pThis = static_cast<ObjectManager*>(p);

	return (pThis->m_stub_map.current_size()==0);
}

int
OOCore::ObjectManager::Close()
{
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	if (!m_bIsAcceptor && m_ptrRemoteFactory)
	{
		// Inform the other end we are leaving
		m_ptrRemoteFactory->SetReverse(0);
	}
	m_ptrRemoteFactory = 0;

	// Wait until all stubs have gone
	ACE_Time_Value wait(5);
	if (ENGINE::instance()->pump_requests(&wait,await_close,this) != 0)
	{
		// Close all the stubs
		for (ACE_Active_Map_Manager<Stub*>::iterator i=m_stub_map.begin();i!=m_stub_map.end();++i)
		{
			(*i).int_id_->Release();
		}
		m_stub_map.unbind_all();

		m_stub_obj_map.remove_all();
	}

	m_ptrTransport = 0;

	return 0;
}

int 
OOCore::ObjectManager::connect()
{
	if (m_ptrRemoteFactory || m_bIsAcceptor)
	{
		errno = EISCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) ObjectManager already connected\n")),-1);
	}

	// We call this synchronously, 'cos the data should already be there
	ACE_Time_Value wait(5);
	if (ENGINE::instance()->pump_requests(&wait,await_connect,this) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Proxy creation timed out and gave up\n")),-1);

	if (m_ptrRemoteFactory->SetReverse(this) != 0)
	{
		m_ptrRemoteFactory = 0;
		return -1;
	}

	return 0;
}

int 
OOCore::ObjectManager::accept()
{
	if (m_ptrRemoteFactory || m_bIsAcceptor)
	{
		errno = EISCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) ObjectManager already connected\n")),-1);
	}

	// Write out the interface info
	if (!m_ptrTransport)
	{
		errno = ENOTCONN;
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Object Manager closed\n")),-1);
	}
	
	// Create the stub
	OOObject::cookie_t key;
	if (CreateStub(RemoteObjectFactory::IID,static_cast<RemoteObjectFactory*>(this),&key) != 0)
		return -1;

	Object_Ptr<OutputStream> output;
	if (m_ptrTransport->CreateOutputStream(&output) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to create output stream\n")),-1);

	// Write out the key
	if (Impl::OutputStream_Wrapper(output).write(key) != 0)
	{
		ReleaseStub(key);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write stub key\n")),-1);
	}

	// Send the message back
	if (m_ptrTransport->Send(output) != 0)
		return -1;

	m_bIsAcceptor = true;
	m_bOpened = true;
	
	return 0;
}

bool 
OOCore::ObjectManager::await_connect(void * p)
{
	ObjectManager* pThis = static_cast<ObjectManager*>(p);

	return pThis->m_ptrRemoteFactory;
}

int 
OOCore::ObjectManager::ProcessMessage(InputStream* input_stream)
{
	// Check we have input
	if (!input_stream)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Calling process msg with a NULL message!\n")),-1);
	}

	Impl::InputStream_Wrapper input(input_stream);

	// Check for the connect state first 
	if (!m_bIsAcceptor && !m_bOpened)
		return process_connect(input);
	
	// Read the message ident
	OOObject::byte_t request;
	if (input.read(request) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read request status\n")),-1);
	
	if (request == 0)
		return process_response(input);
	else if (request == 1)
		return process_request(input);
	else
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid request status\n")),-1);
	}
}

int 
OOCore::ObjectManager::process_connect(Impl::InputStream_Wrapper& input)
{
	if (!m_ptrTransport)
	{
		errno = ENOTCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Connect with no transport!\n")),-1);
	}

	// Read the object key
	OOObject::cookie_t key;
	if (input.read(key) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read key\n")),-1);
	
	int ret = CreateProxy(RemoteObjectFactory::IID,key,reinterpret_cast<OOObject::Object**>(&m_ptrRemoteFactory));
	if (ret == 0)
		m_bOpened = true;
	
	return ret;
}

int 
OOCore::ObjectManager::process_request(Impl::InputStream_Wrapper& input)
{
	// Read the transaction key
	OOObject::uint32_t trans_id;
	if (input.read(trans_id) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read transaction key\n")),-1);
	
	// Read the stub key
	OOObject::cookie_t key;
	if (input.read(key) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read request key\n")),-1);
	
	// Find the stub
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);
	Stub* st;
	if (m_stub_map.find(key,st) != 0)
	{
		errno = ENOENT;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid stub key\n")),-1);
	}

	Object_Ptr<Stub> stub(st);
	Object_Ptr<Transport> transport = m_ptrTransport;
	guard.release();
	
	if (!transport)
	{
		errno = ENOTCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Object Manager closed\n")),-1);
	}

	// Read the sync state
	OOObject::uint32_t flags_i;
	if (input.read(flags_i) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read sync flag\n")),-1);

	Marshall_Flags flags = static_cast<Marshall_Flags>(flags_i);

	// Create the output stream
	Object_Ptr<OutputStream> output;
	if (transport->CreateOutputStream(&output) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to create output stream\n")),-1);
		
	// Write that we are a response
	if (output->WriteByte(0) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write response flag\n")),-1);

	// Write the transaction id
	if (output->WriteULong(trans_id) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write transaction id\n")),-1);

	// Invoke the method on the stub
	if (stub->Invoke(flags,5,input,output) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invoke failed\n")),-1);

	if (flags & SYNC)
	{
		// Send the response
		if (transport->Send(output) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to send output\n")),-1);
	}

	return 0;
}

int 
OOCore::ObjectManager::process_response(Impl::InputStream_Wrapper& input)
{
	// Read the transaction key
	OOObject::uint32_t trans_id;
	if (input.read(trans_id) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read transaction key\n")),-1);
	
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	// Check if its a transaction we care about
	if (m_transaction_set.find(trans_id) == m_transaction_set.end())
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Discarding unknown transaction\n")),0);

	// Pop it in the arrived map
	if (!m_response_map.insert(std::map<OOObject::uint32_t,Object_Ptr<InputStream> >::value_type(trans_id,static_cast<InputStream*>(input))).second)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to insert into response map\n")),-1);
	
	return 0;
}

int 
OOCore::ObjectManager::CreateProxy(const OOObject::guid_t& iid, const OOObject::cookie_t& key, OOObject::Object** ppVal)
{
	if (!ppVal)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);
	}

	// Check if we have already created a proxy for this key
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);
	
	if (m_proxy_obj_map.find(key,*ppVal))
	{
		// Yes we have!
		(*ppVal)->AddRef();
		return 0;
	}

	// Create a new one
	if (Impl::PROXY_STUB_FACTORY::instance()->create_proxy(this,iid,key,ppVal) != 0)
		return -1;
	
	// Pop it in the map
	m_proxy_obj_map.insert(key,iid,*ppVal);

	return 0;
}

int 
OOCore::ObjectManager::CreateStub(const OOObject::guid_t& iid, OOObject::Object* obj, OOObject::cookie_t* key)
{
	if (!key || !obj)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);
	}

	// Aquire lock first
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	// Check if we have created a stub for this obj already
	if (m_stub_obj_map.find(obj,iid,*key))
	{
		// Yes we have
		return 0;
	}

	// Insert the stub into the map (we add NULL to create a key, and then replace its value)
	if (m_stub_map.bind(0,*key) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to bind stub info\n")),-1);
	
	// Create the stub
	Stub* stub = 0;
	
	// If obj is a proxy, we can create a 'pass-through' stub!
	OOObject::cookie_t proxy_key;
	if (m_proxy_obj_map.find(obj,iid,proxy_key))
	{
		// Yes it is!
		ACE_NEW_NORETURN(stub,OOCore::Impl::PassThruStub(this,proxy_key,*key));
		if (stub==0)
		{
			m_stub_map.unbind(*key);
			return -1;
		}
	}
	// Create one from the factory
	else if (Impl::PROXY_STUB_FACTORY::instance()->create_stub(this,iid,obj,*key,&stub) != 0)
	{
		m_stub_map.unbind(*key);
		return -1;
	}

	// Insert the stub into the map
	if (m_stub_map.rebind(*key,stub) != 0)
	{
		stub->Release();
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to bind stub info\n")),-1);
	}

	// Add the obj to the stub key maps
	m_stub_obj_map.insert(*key,iid,obj);
	
	return 0;
}

int 
OOCore::ObjectManager::ReleaseProxy(const OOObject::cookie_t& key)
{
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);
	
	// Remove from proxy map
	return (m_proxy_obj_map.remove(key) ? 0 : -1);
}

int 
OOCore::ObjectManager::ReleaseStub(const OOObject::cookie_t& key)
{
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	// Remove from stub map
	Stub* stub;
	if (m_stub_map.unbind(key,stub) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Trying to remove unbound stub\n")),-1);
	
	// Remove from stub obj map
	m_stub_obj_map.remove(key);
	
	stub->Release();
	
	return 0;
}

int 
OOCore::ObjectManager::CreateRequest(Marshall_Flags flags, const OOObject::cookie_t& proxy_key, OOObject::uint32_t* trans_id, OutputStream** output_stream)
{
	if (!trans_id || !output_stream)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);
	}

	// Put it in the transaction map
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	if (!m_ptrTransport)
	{
		errno = ENOTCONN;
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) CreateRequest called on closed ObjectManager\n")),-1);
	}

	Object_Ptr<Transport> transport = m_ptrTransport;

	// Increment trans_id until we can insert it
	do 
	{
		*trans_id = m_next_trans_id++;
	} while (!m_transaction_set.insert(*trans_id).second);

	guard.release();

	Object_Ptr<OutputStream> ptrOutput;
	if (transport->CreateOutputStream(&ptrOutput) != 0)
	{
		CancelRequest(*trans_id);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to create output stream\n")),-1);
	}

	Impl::OutputStream_Wrapper output(ptrOutput);
	
	// Write message ident (request)
	if (output.write(OOObject::byte_t(1)) != 0)
	{
		CancelRequest(*trans_id);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write request flag\n")),-1);
	}
	
	// Write the transaction id
	if (output.write(*trans_id) != 0)
	{
		CancelRequest(*trans_id);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write transaction id\n")),-1);
	}
		
	// Write the object key
	if (output.write(proxy_key) != 0)
	{
		CancelRequest(*trans_id);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write proxy key\n")),-1);
	}

	// Write the flags
	if (output->WriteUShort(flags) != 0)
	{
		CancelRequest(*trans_id);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write sync status\n")),-1);
	}

	*output_stream = ptrOutput;
	(*output_stream)->AddRef();

	return 0;
}

int 
OOCore::ObjectManager::CancelRequest(OOObject::uint32_t trans_id)
{
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	return (m_transaction_set.erase(trans_id) == 1 ? 0 : -1);
}

bool 
OOCore::ObjectManager::await_response(void* p)
{
	response_wait* rw = static_cast<response_wait*>(p);

	return rw->pThis->await_response_i(rw->trans_id,rw->input);
}

bool 
OOCore::ObjectManager::await_response_i(OOObject::uint32_t trans_id, InputStream** input)
{
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	if (!m_ptrTransport)
		return true;

	std::map<OOObject::uint32_t,Object_Ptr<InputStream> >::iterator i = m_response_map.find(trans_id);
	if (i!=m_response_map.end())
	{
		(*input) = i->second;
		(*input)->AddRef();
		m_response_map.erase(i);
        return true;
	}

	return false;
}

int 
OOCore::ObjectManager::SendAndReceive(Marshall_Flags flags, OOObject::uint16_t wait_secs, OutputStream* output, OOObject::uint32_t trans_id, InputStream** input)
{
	if (!input || !output)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);
	}

	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	if (!m_ptrTransport)
	{
		errno = ENOTCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Calling send and receive on a closed object manager\n")),-1);
	}

	Object_Ptr<Transport> transport = m_ptrTransport;

	guard.release();
	
	if (transport->Send(output) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Send failed\n")),-1);
	
	if (flags & SYNC)
	{
		ACE_Time_Value wait(wait_secs);

		response_wait rw(this,trans_id,input);
		return ENGINE::instance()->pump_requests(&wait,await_response,&rw);
	}
	else
	{
		return 0;
	}
}

OOObject::int32_t 
OOCore::ObjectManager::CreateObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	if (!ppVal)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);
	}

	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	if (!m_ptrRemoteFactory)
	{
		errno = ENOTCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No remote object factory\n")),-1);
	}

	Object_Ptr<RemoteObjectFactory> fact = m_ptrRemoteFactory;

	guard.release();
	
	return fact->CreateRemoteObject(clsid,iid,ppVal);
}

OOObject::int32_t 
OOCore::ObjectManager::CreateRemoteObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	return OOObject::CreateObject(clsid,iid,ppVal);
}

OOObject::int32_t 
OOCore::ObjectManager::SetReverse(RemoteObjectFactory* pRemote)
{
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	if (!m_bIsAcceptor)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Trying to set reverse object factory for acceptor!\n")),-1);
	}

	if (m_ptrRemoteFactory && pRemote)
	{
		errno = EISCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Trying to set reverse object factory multiple times\n")),-1);
	}

	m_ptrRemoteFactory = pRemote;

	return 0;
}

OOObject::int32_t 
OOCore::ObjectManager::AddObjectFactory(const OOObject::guid_t& clsid, OOCore::ObjectFactory* pFactory)
{
	if (!Impl::g_IsServer)
	{
		errno = EOPNOTSUPP;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) AddRemoteObject should not be called on a client\n")),-1);
	}

	return OOCore::AddObjectFactory(clsid,pFactory);
}

OOObject::int32_t 
OOCore::ObjectManager::RemoveObjectFactory(const OOObject::guid_t& clsid)
{
	if (!Impl::g_IsServer)
	{
		errno = EOPNOTSUPP;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) RemoveRemoteObject should not be called on a client\n")),-1);
	}

	return OOCore::RemoveObjectFactory(clsid);
}
