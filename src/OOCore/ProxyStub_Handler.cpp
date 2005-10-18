#include "./ProxyStub_Handler.h"

#include <ace/Reactor.h>
#include <ace/Countdown_Time.h>

#include "./Binding.h"
#include "./Channel.h"
#include "./Marshaller.h"

OOCore_ProxyStub_Handler::proxystub_node OOCore_ProxyStub_Handler::m_core_node = {ACE_DLL(), &CreateProxy, &CreateStub};
OOCore_Proxy_Marshaller OOCore_ProxyStub_Handler::m_failmshl;

OOCore_ProxyStub_Handler::OOCore_ProxyStub_Handler(OOCore_Channel* channel) :
	OOCore_Channel_Handler(channel),
	m_conn_proxy(0),
	m_connected(false)
{
}

OOCore_ProxyStub_Handler::~OOCore_ProxyStub_Handler(void)
{
	for (ACE_Active_Map_Manager<proxystub_node*>::iterator i=m_dll_map.begin();i!=m_dll_map.end();++i)
	{
		delete (*i).int_id_;
	}

	for (ACE_Active_Map_Manager<OOCore_Proxy_Marshaller*>::iterator j=m_transaction_map.begin();j!=m_transaction_map.end();++j)
	{
		delete (*j).int_id_;
	}

	for (std::map<ACE_Active_Map_Manager_Key,ACE_InputCDR*>::iterator k=m_response_map.begin();k!=m_response_map.end();++k)
	{
		delete k->second;
	}

	for (ACE_Active_Map_Manager<OOCore_Object_Stub_Base*>::iterator l=m_stub_map.begin();l!=m_stub_map.end();++l)
	{
		(*l).int_id_->close();
	}
}

int OOCore_ProxyStub_Handler::load_proxy_stub(const OOObj::GUID& iid, proxystub_node*& node)
{
	// Lock for write
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	ACE_Active_Map_Manager_Key key;

	std::map<OOObj::GUID,ACE_Active_Map_Manager_Key>::const_iterator i=m_proxystub_map.find(iid);
	if (i==m_proxystub_map.end())
	{
		guard.release();

		// Find the stub DLL name
		ACE_NS_WString dll_name;
		if (BINDING::instance()->find(iid.to_string().c_str(),dll_name) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) No proxy/stub registered\n")),-1);
			
		// Check if it's us
		if (dll_name==ACE_TEXT_WIDE("OOCore"))
		{
			node = &m_core_node;
			return 0;
		}

		// Open the DLL
		proxystub_node* new_node;
		ACE_NEW_RETURN(new_node,proxystub_node,-1);

		if (new_node->dll.open(ACE_TEXT_WCHAR_TO_TCHAR(dll_name.c_str())) != 0)
			return -1;
		
		// Bind to the CreateStub function
		new_node->stub_fn = static_cast<OOObj::CreateStub_Function>(new_node->dll.symbol(ACE_TEXT("CreateStub")));
		if (new_node->stub_fn == 0)
			return -1;

		// Bind to the CreateProxy function
		new_node->proxy_fn = static_cast<OOObj::CreateProxy_Function>(new_node->dll.symbol(ACE_TEXT("CreateProxy")));
		if (new_node->proxy_fn == 0)
			return -1;

		ACE_Guard<ACE_Thread_Mutex> guard2(m_lock);

		// Add the node info to the map
		if (m_dll_map.bind(new_node,key) != 0)
			return -1;
	
		// Add the key to the other map
		if (!m_proxystub_map.insert(std::map<OOObj::GUID,ACE_Active_Map_Manager_Key>::value_type(iid,key)).second)
			return -1;
	}
	else
	{
		key = i->second;
	}

	guard.acquire();

	return m_dll_map.find(key,node);
}

int OOCore_ProxyStub_Handler::create_stub(const OOObj::GUID& iid, OOObj::Object* obj, OOObj::cookie_t& key)
{
	OOCore_Object_Stub_Base* stub;

	// Check the iid
	if (iid == OOObj::Object::IID)
	{
		ACE_NEW_RETURN(stub,OOObj::Object_Stub(obj),-1);
	}
	else
	{
		// Get the proxy/stub node
		proxystub_node* node;
		if (load_proxy_stub(iid,node) != 0)
			return -1;

		// Call CreateStub
		if ((node->stub_fn)(iid,obj,&stub) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) CreateStub failed\n")),-1);
	}

	// Insert the stub into the map
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
	if (m_stub_map.bind(stub,key) != 0)
	{
		stub->close();
		return -1;
	}

	return 0;
}

int OOCore_ProxyStub_Handler::remove_stub(const OOObj::cookie_t& key)
{
	OOCore_Object_Stub_Base* stub;

	{
		ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
		if (m_stub_map.unbind(key,stub) != 0)
			return -1;
	}

	stub->close();

	return 0;
}

int OOCore_ProxyStub_Handler::create_first_stub(const OOObj::GUID& iid, OOObj::Object* obj)
{
	// Create the stub
	OOObj::cookie_t key;
	if (create_stub(iid,obj,key) != 0)
		return -1;

	// Write out the interface info
	ACE_OutputCDR output;
	if (!(output << iid) ||
		!(output << key))
	{
		remove_stub(key);
		return -1;
	}

	// Send the message back
	ACE_Message_Block* mb = output.begin()->duplicate();
	OOCore_Channel* ch = channel();
	if (ch==0 || ch->send(mb) == -1)
	{
		mb->release();
		return -1;
	}

	return 0;
}

int OOCore_ProxyStub_Handler::create_proxy(const OOObj::GUID& iid, const OOObj::cookie_t& key, OOObj::Object** proxy)
{
	// Check the iid
	if (iid == OOObj::Object::IID)
	{
		ACE_NEW_RETURN(*proxy,OOObj::Object_Proxy<OOObj::Object>(key,this),-1);
		(*proxy)->AddRef();
	}
	else
	{
		// Get the proxy/stub node
		proxystub_node* node;
		if (load_proxy_stub(iid,node) != 0)
			return -1;

		// Call CreateProxy
		if ((node->proxy_fn)(iid,key,this,proxy) != 0)
			ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) CreateProxy failed\n")),-1);
	}

	return 0;
}

int OOCore_ProxyStub_Handler::create_first_proxy(OOObj::Object** proxy)
{
	if (m_conn_proxy != 0)
		return -1;

	m_conn_proxy = proxy;
	
	// We call this synchronously, 'cos the data should already be there
	ACE_Time_Value wait(ACE_OS::gettimeofday());
	wait += 5;
	if (handle_recv(&wait) != 0)
	{
		m_conn_proxy = 0;
		return -1;
	}

	// We never use m_conn_proxy again, but use its value as a flag;
	return 0;
}

int OOCore_ProxyStub_Handler::handle_connect(ACE_InputCDR& input)
{
	// Read the iid
	OOObj::GUID iid;
	if (!(input >> iid))
		return -1;
	
	// Read the object key
	OOObj::cookie_t key;
	if (!(input >> key))
		return -1;
	
	if (create_proxy(iid,key,m_conn_proxy) != 0)
		return -1;

	m_connected = true;
	
	return 0;
}

int OOCore_ProxyStub_Handler::handle_recv(ACE_Time_Value* wait)
{
	// Recv the message
	ACE_Message_Block* mb;
	OOCore_Channel* ch = channel();
	if (ch==0 || ch->recv(mb,wait)==-1)
		return -1;

	// Create an input stream on the message block
	// (this makes a copy of the block)
	ACE_InputCDR* input;
	ACE_NEW_NORETURN(input,ACE_InputCDR(mb));
	mb->release();
	if (input == 0)
		return -1;

	// Check for the first connect state
	if (!m_connected && m_conn_proxy != 0)
	{
		int ret = handle_connect(*input);
		delete input;
		return ret;
	}

	// Read the message ident
	ACE_CDR::Boolean request;
	if (!input->read_boolean(request))
	{
		delete input;
		return -1;
	}

	if (request)
		return recv_request(input);
	else
		return recv_response(input);
}

int OOCore_ProxyStub_Handler::recv_request(ACE_InputCDR* input)
{
	// Read the transaction key
	ACE_Active_Map_Manager_Key trans_key;
	if (!(*input >> trans_key))
	{
		delete input;
		return -1;
	}

	// Read the stub key
	OOObj::cookie_t key;
	if (!(*input >> key))
	{
		delete input;
		return -1;
	}

	// Read the method number
	ACE_CDR::ULong method;
	if (!input->read_ulong(method))
	{
		delete input;
		return -1;
	}

	// Read the sync state
	ACE_CDR::Boolean sync;
	if (!input->read_boolean(sync))
	{
		delete input;
		return -1;
	}

	// Find the stub
	OOCore_Object_Stub_Base* stub;
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
	if (m_stub_map.find(key,stub) != 0)
	{
		delete input;
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid Object key\n")),-1);
	}
	guard.release();

	// Create a marshaller
	OOCore_Stub_Marshaller mshl(this,input,sync);

	// Invoke the method
	int ret_code;
	int inv_code = stub->invoke(method,ret_code,mshl);
	if (inv_code!=0 && inv_code!=1)
		return -1;
	
	if (mshl.m_sync)
	{
		// Build a response message
		ACE_OutputCDR output;

		// Write that we are a response
		if (!output.write_boolean(false))
			return -1;
	
		// Write the transaction id
		if (!(output << trans_key))
			return -1;

		// Write the response code
		if (!output.write_long(ret_code))
			return -1;

		if (ret_code==0)
		{
			// Write results
			if (mshl.output_response(output) != 0)
				return -1;
		}

		// Send the response
		ACE_Message_Block* mb = output.begin()->duplicate();
		OOCore_Channel* ch = channel();
		if (ch==0 || ch->send(mb) != 0)
		{
			mb->release();
			return -1;
		}
	}

	if (inv_code == 1)
	{
		if (remove_stub(key) != 0)
			return -1;
	}

	return 0;
}

int OOCore_ProxyStub_Handler::recv_response(ACE_InputCDR* input)
{
	// Read the transaction key
	ACE_Active_Map_Manager_Key trans_key;
	if (!(*input >> trans_key))
	{
		delete input;
		return -1;
	}

	// Put in the response map
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
	if (!m_response_map.insert(std::map<ACE_Active_Map_Manager_Key,ACE_InputCDR*>::value_type(trans_key,input)).second)
	{
		delete input;
		return -1;
	}

	return 0;
}

int OOCore_ProxyStub_Handler::get_response(const ACE_Active_Map_Manager_Key& trans_key, ACE_InputCDR*& input, ACE_Time_Value* wait)
{
	// Default the timeout to 30 secs
	ACE_Time_Value wait2(30),*wait3;
	if (wait!=0)
		wait3 = wait;
	else
		wait3 = &wait2;
	
	// Spin waiting for a response
	ACE_Countdown_Time countdown(wait3);
	while (*wait3 != ACE_Time_Value::zero && !closed())
	{
		ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
		std::map<ACE_Active_Map_Manager_Key,ACE_InputCDR*>::iterator i = m_response_map.find(trans_key);
		guard.release();

		if (i!=m_response_map.end())
		{
			input = i->second;
			m_response_map.erase(i);
            return 0;
		}
		
		int work = ACE_Reactor::instance()->work_pending();
		if (work==1)
			ACE_Reactor::instance()->handle_events();
		else if (work==-1)
		{
			ACE_Time_Value t(0,1000);
			ACE_Reactor::instance()->handle_events(t);
		}

		countdown.update();
	}

	return -1;
}

int OOCore_ProxyStub_Handler::handle_close()
{
	// Close down the stubs we control
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
	for (ACE_Active_Map_Manager<OOCore_Object_Stub_Base*>::iterator l=m_stub_map.begin();l!=m_stub_map.end();++l)
	{
		(*l).int_id_->close();
	}
	m_stub_map.close();

	guard.release();
	
	return OOCore_Channel_Handler::handle_close();
}

OOCore_Proxy_Marshaller& OOCore_ProxyStub_Handler::create_marshaller(const ACE_Active_Map_Manager_Key& key, unsigned int method, bool sync, ACE_Time_Value* wait)
{
	if (closed())
		return m_failmshl;

	// Create a new OOCore_Proxy_Marshaller*
	OOCore_Proxy_Marshaller* mshl;
	ACE_NEW_RETURN(mshl,OOCore_Proxy_Marshaller(this,sync),m_failmshl);

	// Put it in the transaction map
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
	int ret = m_transaction_map.bind(mshl,mshl->m_trans_key);
	guard.release();
	if (ret!=0)
	{
		mshl->fail();
		return *mshl;
	}

	// Write message ident (request)
	if (!mshl->m_output.write_boolean(true))
	{
		mshl->fail();
		return *mshl;
	}

	// Write the transaction key
	if (!(mshl->m_output << mshl->m_trans_key))
	{
		mshl->fail();
		return *mshl;
	}

	// Write the object key
	if (!(mshl->m_output << key))
	{
		mshl->fail();
		return *mshl;
	}

	// Write the method number
	if (!mshl->m_output.write_ulong(method))
	{
		mshl->fail();
		return *mshl;
	}

	// Write the sync state
	if (!mshl->m_output.write_boolean(sync))
	{
		mshl->fail();
		return *mshl;
	}

	return *mshl;
}

int OOCore_ProxyStub_Handler::remove_marshaller(const ACE_Active_Map_Manager_Key& trans_key)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	OOCore_Proxy_Marshaller* mshl;
	if (m_transaction_map.unbind(trans_key,mshl) != 0)
	{
		return -1;
	}
	guard.release();

	delete mshl;
	
	return 0;
}