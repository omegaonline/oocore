#include "./ObjectManager.h"

#include "./OOCore_Impl.h"
#include "./Engine.h"
#include "./Proxy_Stub_Factory.h"

OOCore::ObjectManager::ObjectManager(void) :
	m_bServer(false),
	m_next_trans_id(static_cast<OOObj::uint32_t>(ACE_OS::rand()))
{
}

OOCore::ObjectManager::~ObjectManager(void)
{
}

int 
OOCore::ObjectManager::Open(Transport* transport, OOObj::bool_t AsServer)
{
	if (m_ptrTransport)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Calling open repeatedly on an ObjectManager!\n")),-1);

	m_ptrTransport = transport;

	int res;
	if (AsServer)
		res = accept();
	else
		res = connect();

	if (res!=0)
		m_ptrTransport = 0;

	return res;
}

int
OOCore::ObjectManager::Close()
{
	m_ptrTransport = 0;

	return 0;
}

int 
OOCore::ObjectManager::connect()
{
	if (m_ptrRemoteFactory || m_bServer)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) ObjectManager already connected\n")),-1);

	// We call this synchronously, 'cos the data should already be there
	ACE_Time_Value wait(5);
	if (ENGINE::instance()->pump_requests(&wait,await_connect,this) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Proxy creation timed out and gave up\n")),-1);

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
	if (m_ptrRemoteFactory || m_bServer)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) ObjectManager already connected\n")),-1);

	// Write out the interface info
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
	Object_Ptr<Transport> transport = m_ptrTransport;
	guard.release();

	if (!transport)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Object Manager closed\n")),-1);
	
	Object_Ptr<OutputStream> output;
	if (transport->CreateOutputStream(&output) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to create output stream\n")),-1);

	// Create the stub
	if (CreateStub(RemoteObjectFactory::IID,static_cast<RemoteObjectFactory*>(this),output) != 0)
		return -1;

	// Send the message back
	if (transport->Send(output) != 0)
		return -1;

	m_bServer = true;
	
	return 0;
}

bool 
OOCore::ObjectManager::await_connect(void * p)
{
	ObjectManager* pThis = static_cast<ObjectManager*>(p);

	return pThis->m_ptrRemoteFactory;
}

int 
OOCore::ObjectManager::ProcessMessage(InputStream* input)
{
	// Check we have input
	if (!input)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Calling process msg with a NULL message!\n")),-1);

	// Check for the connect state first 
	if (!m_bServer && !m_ptrRemoteFactory)
		return process_connect(input);
	
	// Read the message ident
	OOObj::bool_t request;
	if (input->ReadBoolean(request) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read request status\n")),-1);
	
	if (request)
		return process_request(input);
	else
		return process_response(input);
}

int 
OOCore::ObjectManager::process_connect(InputStream* input)
{
	if (!m_ptrTransport)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Connect with no transport!\n")),-1);

	// Read the object key
	OOObj::cookie_t key;
	if (input->ReadCookie(key) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read key\n")),-1);
	
	return CreateProxy(RemoteObjectFactory::IID,key,reinterpret_cast<OOObj::Object**>(&m_ptrRemoteFactory));
}

int 
OOCore::ObjectManager::process_request(InputStream* input)
{
	// Read the transaction key
	OOObj::uint32_t trans_id;
	if (input->ReadULong(trans_id) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read transaction key\n")),-1);
	
	// Read the stub key
	OOObj::cookie_t key;
	if (input->ReadCookie(key) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read request key\n")),-1);
	
	// Read the method number
	OOObj::uint32_t method;
	if (input->ReadULong(method) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read method ordinal\n")),-1);
	
	// Read the sync state
	OOObj::bool_t sync;
	if (input->ReadBoolean(sync) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read sync flag\n")),-1);
	
	// Find the stub
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
	Stub* st;
	if (m_stub_map.find(key,st) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid stub key\n")),-1);

	Object_Ptr<Stub> stub(st);
	Object_Ptr<Transport> transport = m_ptrTransport;
	guard.release();

	if (!transport)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Object Manager closed\n")),-1);

	// Create the output streams
	Object_Ptr<OutputStream> output1,output2;
	if (sync)
	{
		if (transport->CreateOutputStream(&output1) != 0 ||
			transport->CreateOutputStream(&output2) != 0)
		{
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to create output stream\n")),-1);
		}
	}

	// Invoke the method on the stub
	OOObj::int32_t ret_code;
	int inv_code = stub->Invoke(method,ret_code,input,output2);
	if (inv_code != 0 && inv_code != 1)
		ret_code = static_cast<OOObj::int32_t>(inv_code);
		
	if (sync)
	{
		// Write that we are a response
		if (output1->WriteBoolean(false) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write response flag\n")),-1);
	
		// Write the transaction id
		if (output1->WriteULong(trans_id) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write transaction id\n")),-1);

		// Write the response code
		if (output1->WriteLong(ret_code) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write return code\n")),-1);

		if (ret_code==0)
		{
			// Write results
			if (output1->Append(output2) != 0)
				ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to append output\n")),-1);
		}

		// Send the response
		if (transport->Send(output1) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to send output\n")),-1);
	}

	// If we released, remove the stub
	if (inv_code == 1)
	{
		if (remove_stub(key) != 0)
			return -1;
	}

	return 0;
}

int 
OOCore::ObjectManager::process_response(InputStream* input)
{
	// Read the transaction key
	OOObj::uint32_t trans_id;
	if (input->ReadULong(trans_id) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read transaction key\n")),-1);
	
	// Put in the response map
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	if (m_transaction_set.find(trans_id) == m_transaction_set.end())
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Discarding unknown transaction\n")),0);

	if (!m_response_map.insert(std::map<OOObj::uint32_t,Object_Ptr<InputStream> >::value_type(trans_id,input)).second)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to insert into response map\n")),-1);
	
	return 0;
}

int 
OOCore::ObjectManager::CreateProxy(const OOObj::guid_t& iid, const OOObj::cookie_t& key, OOObj::Object** ppVal)
{
	return Impl::PROXY_STUB_FACTORY::instance()->create_proxy(this,iid,key,ppVal);
}

int 
OOCore::ObjectManager::CreateStub(const OOObj::guid_t& iid, OOObj::Object* obj, OutputStream* output)
{
	if (!output)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);

	// Create the stub
	Stub* stub;
	if (Impl::PROXY_STUB_FACTORY::instance()->create_stub(this,iid,obj,&stub) != 0)
		return -1;

	// Insert the stub into the map
	OOObj::cookie_t key;
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
	if (m_stub_map.bind(stub,key) != 0)
	{
		guard.release();

		stub->Release();
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to bind stub info\n")),-1);
	}
	guard.release();
	
	// Write out the key
	if (output->WriteCookie(key) != 0)
	{
		remove_stub(key);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write iid and key\n")),-1);
	}

	return 0;
}

int 
OOCore::ObjectManager::remove_stub(const OOObj::cookie_t& key)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	Stub* stub;
	if (m_stub_map.unbind(key,stub) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Trying to remove unbound stub\n")),-1);
	
	guard.release();

	stub->Release();

	return 0;
}

int 
OOCore::ObjectManager::CreateRequest(const OOObj::cookie_t& proxy_key, OOObj::uint32_t method, OOObj::bool_t sync, OOObj::uint32_t* trans_id, OutputStream** output)
{
	if (!trans_id || !output)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);

	// Put it in the transaction map
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	if (!m_ptrTransport)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) CreateRequest called on closed ObjectManager\n")),-1);

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
	
	// Write message ident (request)
	if (ptrOutput->WriteBoolean(true) != 0)
	{
		CancelRequest(*trans_id);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write request flag\n")),-1);
	}
	
	// Write the transaction id
	if (ptrOutput->WriteULong(*trans_id) != 0)
	{
		CancelRequest(*trans_id);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write transaction id\n")),-1);
	}
		
	// Write the object key
	if (ptrOutput->WriteCookie(proxy_key) != 0)
	{
		CancelRequest(*trans_id);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write proxy key\n")),-1);
	}

	// Write the method number
	if (ptrOutput->WriteULong(method) != 0)
	{
		CancelRequest(*trans_id);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write method ordinal\n")),-1);
	}
		
	// Write the sync state
	if (ptrOutput->WriteBoolean(sync) != 0)
	{
		CancelRequest(*trans_id);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write sync status\n")),-1);
	}

	*output = ptrOutput;
	(*output)->AddRef();

	return 0;
}

int 
OOCore::ObjectManager::CancelRequest(OOObj::uint32_t trans_id)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	return (m_transaction_set.erase(trans_id) == 1 ? 0 : -1);
}

bool 
OOCore::ObjectManager::await_response(void* p)
{
	response_wait* rw = static_cast<response_wait*>(p);

	return rw->pThis->await_response_i(rw->trans_id,rw->input);
}

bool 
OOCore::ObjectManager::await_response_i(OOObj::uint32_t trans_id, InputStream** input)
{
	if (!m_ptrTransport)
		return true;

	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
	std::map<OOObj::uint32_t,Object_Ptr<InputStream> >::iterator i = m_response_map.find(trans_id);
	guard.release();

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
OOCore::ObjectManager::SendAndReceive(OutputStream* output, OOObj::uint32_t trans_id, InputStream** input)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
	Object_Ptr<Transport> transport = m_ptrTransport;
	guard.release();

	if (!transport)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Calling send and receive on a closed object manager\n")),-1);

	if (transport->Send(output) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Send failed\n")),-1);
	
	if (input)
	{
		// Default the timeout to 5 secs
		ACE_Time_Value wait(5);

		response_wait rw(this,trans_id,input);
		return ENGINE::instance()->pump_requests(&wait,await_response,&rw);
	}
	else
	{
		return 0;
	}
}

OOObj::int32_t 
OOCore::ObjectManager::CreateRemoteObject(const OOObj::char_t* class_name, const OOObj::guid_t& iid, OOObj::Object** ppVal)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	Object_Ptr<RemoteObjectFactory> fact = m_ptrRemoteFactory;

	guard.release();

	if (!fact)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No remote object factory\n")),-1);

	return fact->CreateObject(class_name,iid,ppVal);
}

OOObj::int32_t 
OOCore::ObjectManager::CreateObject(const OOObj::char_t* class_name, const OOObj::guid_t& iid, OOObj::Object** ppVal)
{
	// DO THE BLACK MAGIC HERE!


}

OOObj::int32_t 
OOCore::ObjectManager::SetReverse(RemoteObjectFactory* pRemote)
{
	if (!m_bServer)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Trying to set reverse object factory for non server!\n")),-1);

	if (!pRemote)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);

	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	if (m_ptrRemoteFactory)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Trying to set reverse object factory multiple times\n")),-1);

	m_ptrRemoteFactory = pRemote;

	return 0;
}
