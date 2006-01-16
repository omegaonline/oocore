#include "./Engine.h"
#include "./Proxy_Stub_Factory.h"
#include "./PassThruStub.h"
#include "./InputStream_CDR.h"

DEFINE_IID(OOCore::Impl::RemoteObjectFactory,B1BC71BE-4DCC-4f0f-8483-A75D35126D2A);

OOCore::ObjectManager::ObjectManager() :
	m_is_opening(false),
	m_next_trans_id(static_cast<OOObject::uint32_t>(ACE_OS::rand()))
{
}

OOCore::ObjectManager::~ObjectManager()
{
}

int 
OOCore::ObjectManager::Open(Channel* channel)
{
	if (m_ptrChannel)
	{
		errno = EISCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Calling open repeatedly on an ObjectManager!\n")),-1);
	}

	m_ptrChannel = channel;

	return 0;
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
	Object_Ptr<Impl::RemoteObjectFactory> fact = m_ptrRemoteFactory.clear();

	// Inform the other end we are leaving...
	if (fact)
		fact->SetReverse(0);
	
	if (!Impl::g_IsServer)
	{
		// Wait until all stubs have gone
		ACE_Time_Value wait(DEFAULT_WAIT);
		ENGINE::instance()->pump_requests(&wait,await_close,this);
	}

	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	// Close all the stubs
	for (ACE_Active_Map_Manager<Stub*>::iterator i=m_stub_map.begin();i!=m_stub_map.end();++i)
	{
		(*i).int_id_->Release();
	}
	m_stub_map.unbind_all();
		
	m_stub_obj_map.remove_all();

	m_ptrChannel = 0;
	
	return 0;
}

int 
OOCore::ObjectManager::request_remote_factory()
{
	if (m_is_opening)
	{
		// Wait for a connect response
		ACE_Time_Value wait(DEFAULT_WAIT);
		if (ENGINE::instance()->pump_requests(&wait,await_connect,this) != 0)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Connection negotiation timed out and gave up\n")),-1);
	
		return 0;
	}

	m_is_opening = true;

	// Create the stub
	OOCore::ProxyStubManager::cookie_t key;
	if (CreateStub(RemoteObjectFactory::IID,static_cast<RemoteObjectFactory*>(this),&key) != 0)
	{
		m_is_opening = false;
		return -1;
	}

	Object_Ptr<Channel> channel = m_ptrChannel;
	if (!channel)
	{
		m_is_opening = false;
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) No channel to negotiate on!\n")),-1);
	}

	// Write out the interface info
	Object_Ptr<OutputStream> output;
	if (channel->CreateOutputStream(&output) != 0)
	{
		m_is_opening = false;
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to create output stream\n")),-1);
	}

	// Write out the message ident (connect) and key
	OutputStream_Wrapper out(output);
	if (out->WriteByte(CONNECT) != 0 ||
		out.write(key) != 0)
	{
		m_is_opening = false;
		ReleaseStub(key);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write negotiate key\n")),-1);
	}

	// Send the message back
	if (channel->Send(output) != 0)
	{
		m_is_opening = false;
		ReleaseStub(key);
		return -1;
	}

	// Wait for a connect response
	ACE_Time_Value wait(DEFAULT_WAIT);
	if (ENGINE::instance()->pump_requests(&wait,await_connect,this) != 0)
	{
		m_is_opening = false;
		ReleaseStub(key);
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Connection negotiation timed out and gave up\n")),-1);
	}

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

	// Wrap the stream
	InputStream_Wrapper input(input_stream);

	// Read the message ident
	OOObject::byte_t message;
	if (input->ReadByte(message) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read message opcode\n")),-1);
	
	switch (message)
	{
	case REQUEST:
		return process_request(input);

	case RESPONSE:
		return process_response(input);

	case CONNECT:
		return process_connect(input);

	default:
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid message opcode %d\n"),message),-1);
	}
}

int 
OOCore::ObjectManager::process_connect(InputStream_Wrapper& input)
{
	// Read the object key
	OOCore::ProxyStubManager::cookie_t key;
	if (input.read(key) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read key\n")),-1);
	
	// Create a proxy for the other end's RemoteFactory
	int ret = CreateProxy(RemoteObjectFactory::IID,key,reinterpret_cast<OOObject::Object**>(&m_ptrRemoteFactory));
	if (ret != 0)
		return ret;

	return m_ptrRemoteFactory->SetReverse(this);
}

int 
OOCore::ObjectManager::process_request(InputStream_Wrapper& input)
{
	// Read the transaction key
	OOObject::uint32_t trans_id;
	if (input.read(trans_id) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read transaction key\n")),-1);
	
	// Read the stub key
	OOCore::ProxyStubManager::cookie_t key;
	if (input.read(key) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read request key\n")),-1);

	// Read the sync state
	TypeInfo::Method_Attributes_t flags;
	if (input.read(flags) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read sync flag\n")),-1);

	OOObject::int32_t ret_code = 0;
	
	// Find the stub
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	Stub* st = 0;
	if (m_stub_map.find(key,st) != 0)
	{
		ret_code = -1;
		errno = ENOENT;
		ACE_ERROR((LM_ERROR,ACE_TEXT("(%P|%t) %@: Invalid stub key %d:%d\n"),this,key.slot_index(),key.slot_generation()));
	}

	guard.release();

	Object_Ptr<Stub> stub(st);
	Object_Ptr<OutputStream> output;
	Object_Ptr<Channel> channel = m_ptrChannel;
	
	if (!(flags & TypeInfo::async_method))
	{
		if (!channel)
		{
			errno = ENOTCONN;
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Object Manager closed\n")),-1);
		}
		
		// Create the output stream
		if (channel->CreateOutputStream(&output) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to create output stream\n")),-1);
			
		// Write that we are a response
		if (output->WriteByte(RESPONSE) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write response flag\n")),-1);

		// Write the transaction id
		if (output->WriteULong(trans_id) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write transaction id\n")),-1);
	}

	// Invoke the method on the stub
	if (ret_code == 0)
		ret_code = stub->Invoke(flags,DEFAULT_WAIT,input,output);

	if (!(flags & TypeInfo::async_method))
	{
		if (ret_code != 0)
		{
			ACE_ERROR((LM_DEBUG,ACE_TEXT("(%P|%t) Invoke failed: %d '%m'\n"),errno));

			// Clear the output, its probably garbage
			output = 0;
	        
			// Recreate the header...
			if (channel->CreateOutputStream(&output) != 0)
				ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to create output stream\n")),-1);
				
			// Write that we are a response
			if (output->WriteByte(RESPONSE) != 0)
				ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write response flag\n")),-1);

			// Write the transaction id
			if (output->WriteULong(trans_id) != 0)
				ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write transaction id\n")),-1);

			// Write ret code
			if (output->WriteLong(ret_code) != 0)
				ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write ret_code\n")),-1);

			// Write errno
			if (output->WriteLong(errno) != 0)
				ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write errno\n")),-1);
		}
		
        // Send the response
		if (channel->Send(output) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to send output\n")),-1);
	}

	return 0;
}

int 
OOCore::ObjectManager::process_response(InputStream_Wrapper& input)
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
OOCore::ObjectManager::CreateProxy(const OOObject::guid_t& iid, const OOCore::ProxyStubManager::cookie_t& key, OOObject::Object** ppVal)
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

	// Check if we asking for a stub we already have
	/*Stub* stub;
	if (m_stub_map.find(key,stub) == 0)
	{
		if (stub->GetObject(ppVal) == 0)
			return 0;
	}*/

	// Create a new one
	if (Impl::PROXY_STUB_FACTORY::instance()->create_proxy(this,iid,key,ppVal) != 0)
		return -1;
	
	// Pop it in the map
	m_proxy_obj_map.insert(key,iid,*ppVal);

	//ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) %@: Created proxy %d:%d\n"),this,key.slot_index(),key.slot_generation()));

	return 0;
}

int
OOCore::ObjectManager::create_pass_thru(OOObject::Object* obj, const OOCore::ProxyStubManager::cookie_t& stub_key, Stub** stub)
{
	// Locks are held already!

	if (!m_ptrChannel)
	{
		errno = ENOTCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) ObjectManager closed.\n")),-1);
	}
	
	// Check if obj is a proxy
	Object_Ptr<OOCore::Proxy> proxy;
	if (obj->QueryInterface(OOCore::Proxy::IID,reinterpret_cast<OOObject::Object**>(&proxy)) != 0)
		return -1;

	// Get the proxy's key
	OOCore::ProxyStubManager::cookie_t proxy_key;
	if (proxy->GetKey(&proxy_key) != 0)
		return -1;

	// Get the proxy's ProxyStubManager
	Object_Ptr<OOCore::ProxyStubManager> manager;
	if (proxy->GetManager(&manager) != 0)
		return -1;

	// Create stream from the other manager
	Object_Ptr<OutputStream> their_output;
	OOObject::uint32_t trans_id;
	if (manager->CreateRequest(OOCore::TypeInfo::sync_method,proxy_key,&trans_id,&their_output) != 0)
		return -1;

	// Immediately cancel the request
	manager->CancelRequest(trans_id);

	// Check if their channel's streams are actually *_CDR streams
	Object_Ptr<Impl::InputStream_CDR> magic;
	if (their_output.QueryInterface(&magic) != 0)
		return -1;

	magic.clear();

    // Create a stream from our channel
	Object_Ptr<OutputStream> our_output;
	if (m_ptrChannel->CreateOutputStream(&our_output) != 0)
		return -1;

	// Check if our channel's streams are actually *_CDR streams
	if (our_output.QueryInterface(&magic) != 0)
		return -1;

	// If we get here then we can create a pass thru
	ACE_NEW_RETURN(*stub,OOCore::Impl::PassThruStub(this,stub_key,manager,proxy_key,proxy),-1);

	(*stub)->AddRef();

	//ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) %@: Created pass-thru %d:%d -> %d:%d\n"),this,stub_key.slot_index(),stub_key.slot_generation(),proxy_key.slot_index(),proxy_key.slot_generation()));

	return 0;
}

int 
OOCore::ObjectManager::CreateStub(const OOObject::guid_t& iid, OOObject::Object* obj, OOCore::ProxyStubManager::cookie_t* key)
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
	
	// Try to create a pass through stub or create one from the factory
	if (create_pass_thru(obj,*key,&stub) != 0 && 
		Impl::PROXY_STUB_FACTORY::instance()->create_stub(this,iid,obj,*key,&stub) != 0)
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

	//ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) %@: Created stub %d:%d\n"),this,key->slot_index(),key->slot_generation()));
	
	return 0;
}

int 
OOCore::ObjectManager::ReleaseProxy(const OOCore::ProxyStubManager::cookie_t& key)
{
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	//ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) %@: Removed proxy %d:%d\n"),this,key.slot_index(),key.slot_generation()));
	
	// Remove from proxy map
	return (m_proxy_obj_map.remove(key) ? 0 : -1);
}

int 
OOCore::ObjectManager::ReleaseStub(const OOCore::ProxyStubManager::cookie_t& key)
{
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	// Remove from stub map
	Stub* stub;
	if (m_stub_map.unbind(key,stub) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Trying to remove unbound stub\n")),-1);
	
	// Remove from stub obj map
	m_stub_obj_map.remove(key);
	
	stub->Release();
	
	//ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) %@: Removed stub %d:%d\n"),this,key.slot_index(),key.slot_generation()));
	
	return 0;
}

int 
OOCore::ObjectManager::CreateRequest(TypeInfo::Method_Attributes_t flags, const OOCore::ProxyStubManager::cookie_t& proxy_key, OOObject::uint32_t* trans_id, OutputStream** output_stream)
{
	if (!trans_id || !output_stream)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);
	}

	Object_Ptr<Channel> channel = m_ptrChannel;
	if (!channel)
	{
		errno = ESHUTDOWN;
		return -1;
	}

	// Put it in the transaction map
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	// Increment trans_id until we can insert it
	do 
	{
		*trans_id = m_next_trans_id++;
	} while (!m_transaction_set.insert(*trans_id).second);

	guard.release();

	Object_Ptr<OutputStream> ptrOutput;
	if (channel->CreateOutputStream(&ptrOutput) != 0)
	{
		CancelRequest(*trans_id);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to create output stream\n")),-1);
	}

	// Write message ident (request)
	if (ptrOutput->WriteByte(REQUEST) != 0)
	{
		CancelRequest(*trans_id);
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write request flag\n")),-1);
	}
	
	OutputStream_Wrapper output(ptrOutput);

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
	if (output.write(flags) != 0)
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
	if (!m_ptrChannel)
		return true;

	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock,0);

	if (!guard.locked())
		return false;

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

OOObject::int32_t  
OOCore::ObjectManager::SendAndReceive(TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, OutputStream* output, OOObject::uint32_t trans_id, InputStream** input)
{
	if (!input || !output)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);
	}

	Object_Ptr<Channel> channel = m_ptrChannel;
	if (!channel)
	{
		errno = ENOTCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Calling send and receive on a closed object manager\n")),-1);
	}

	if (channel->Send(output) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Send failed\n")),-1);
	
	if (!(flags & TypeInfo::async_method))
	{
		ACE_Time_Value wait(wait_secs);

		response_wait rw(this,trans_id,input);
		if (ENGINE::instance()->pump_requests(&wait,await_response,&rw) != 0)
		{
			errno = ETIMEDOUT;
            return -1;
		}
		else if (*input==0)
		{
			errno = ESHUTDOWN;
			return -1;
		}

		// Read the response code
		OOObject::int32_t ret_code;
		if ((*input)->ReadLong(ret_code) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read return code\n")),-1);

		// If it fails, get the error code, and set it
		if (ret_code != 0)
		{
			OOObject::int32_t error_no;
			if ((*input)->ReadLong(error_no) != 0)
				ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read errno\n")),-1);

			errno = error_no;

			return ret_code;
		}
	}
	
	return 0;
}

OOObject::int32_t 
OOCore::ObjectManager::CreateRemoteObject(const OOObject::char_t* remote_url, const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	if (!ppVal)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);
	}

	Object_Ptr<RemoteObjectFactory> fact = m_ptrRemoteFactory;

	if (!fact)
	{
		int ret = request_remote_factory();
		if (ret!=0)
			return ret;

		fact = m_ptrRemoteFactory;
	}

	return fact->RequestRemoteObject(remote_url,clsid,iid,ppVal);
}

OOObject::int32_t 
OOCore::ObjectManager::RequestRemoteObject(const OOObject::char_t* remote_url, const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal)
{
	return OOObject::CreateRemoteObject(remote_url,clsid,iid,ppVal);
}

OOObject::int32_t 
OOCore::ObjectManager::SetReverse(RemoteObjectFactory* pRemote)
{
	if (m_ptrRemoteFactory && pRemote)
	{
		errno = EISCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Trying to set reverse object factory multiple times\n")),-1);
	}

	m_ptrRemoteFactory = pRemote;

	return 0;
}

OOObject::int32_t 
OOCore::ObjectManager::AddObjectFactory(ObjectFactory::Flags_t flags, const OOObject::guid_t& clsid, OOCore::ObjectFactory* pFactory)
{
	if (!Impl::g_IsServer)
	{
		errno = EOPNOTSUPP;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) AddObjectFactory should not be called on a client\n")),-1);
	}

	return OOCore::AddObjectFactory(flags,clsid,pFactory);
}

OOObject::int32_t 
OOCore::ObjectManager::RemoveObjectFactory(const OOObject::guid_t& clsid)
{
	if (!Impl::g_IsServer)
	{
		errno = EOPNOTSUPP;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) RemoveObjectFactory should not be called on a client\n")),-1);
	}

	return OOCore::RemoveObjectFactory(clsid);
}
