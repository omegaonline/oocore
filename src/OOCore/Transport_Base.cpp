#include "./Transport_Base.h"

#include <ace/Reactor.h>
#include <ace/Countdown_Time.h>

#include "./Channel.h"
#include "./Binding.h"
#include "./Transport_Service.h"

OOCore_Transport_Base::OOCore_Transport_Base(void) :
	m_connected(NOT_CONNECTED),
	m_curr_block(0),
	m_conn_count(0),
	m_refcount(0)
{
	BINDING::instance()->rebind(OOCore_Transport_Service::IID.to_string().c_str(),ACE_TEXT_WIDE("OOCore"));
}

OOCore_Transport_Base::~OOCore_Transport_Base(void)
{
	if (m_curr_block)
		m_curr_block->release();
}

int OOCore_Transport_Base::addref()
{
	++m_refcount;
	return 0;
}

int OOCore_Transport_Base::release()
{
	if (--m_refcount < 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Refcount has gone negative\n")),-1);

	if (m_refcount==0)
		delete this;

	return 0;
}

int OOCore_Transport_Base::on_close()
{
	return close_transport();
}

int OOCore_Transport_Base::close_transport()
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	if (m_connected == CLOSED)
		return 0;

	if (close_all_channels() != 0)
		return -1;

	if (m_curr_block)
	{
		m_curr_block->release();
		m_curr_block = 0;
	}

	if (m_connected == CONNECTED)
	{
		m_connected = CLOSED;
	}

	ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Transport %@ close\n"),this));

	return release();
}

int OOCore_Transport_Base::recv(ACE_Message_Block* in_block)
{
	// Protect m_curr_block
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	// Append the new data
	if (m_curr_block)
	{
		// Append the message block
		m_curr_block->cont(in_block);	
	}
	else
		m_curr_block = in_block;

	// Create an input stream
	ACE_InputCDR input(m_curr_block);
	
	m_curr_block->release();
	m_curr_block = 0;

    // Loop copying bytes from the new_block
	while (input.length()>0)
	{
		// Stash current read point
		char* rd_ptr = input.start()->rd_ptr();

		// Read the header
		OOCore_Transport_MsgHeader header;
		int ret = header.read(input);
		if (ret == -1)
			return -1;

		if (ret==1) // More data required
		{
			// Update m_curr_block to input's message block
			m_curr_block = input.start()->duplicate();

			// Reset the amount just read, we read again in a moment
			m_curr_block->rd_ptr(rd_ptr - m_curr_block->rd_ptr());

			return 0;
		}

		// Process the message block
		if (process_msg(header,input) == -1)
			return -1;
	}

	return 0;
}

int OOCore_Transport_Base::bad_channel(const OOCore_Transport_MsgHeader& header, ACE_InputCDR& input)
{
	if (header.msg_size == 0 || m_conn_count==0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid channel key received\n")),-1);

	// Save the payload for later 
	ACE_Message_Block* mb = input.start()->duplicate();
	mb->wr_ptr(mb->rd_ptr() + header.msg_size);

	// Skip the bytes of the message
	if (!input.skip_bytes(header.msg_size))
	{
		mb->release();
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to skip header bytes\n")),-1);
	}

	// Lock is already held
	m_pending_list.insert(std::multimap<ACE_Active_Map_Manager_Key,ACE_Message_Block*>::value_type(header.key,mb));
	
	return 0;
}

int OOCore_Transport_Base::process_msg(const OOCore_Transport_MsgHeader& header, ACE_InputCDR& input)
{
	// Find the channel
	OOCore_Channel* channel;
	if (find_channel(header.key,channel) != 0)
	{
		if (m_connected == CONNECTING)
		{
			// This is the original connect response
			return connect_first_channel_i(header,input);
		}
		else
		{
			return bad_channel(header,input);		
		}
	}

	if (header.msg_size > 0)
	{
		// Duplicate the payload
		ACE_Message_Block* mb = input.start()->duplicate();
		mb->wr_ptr(mb->rd_ptr() + header.msg_size);

		// Put the data into the channel
		if (channel->send(mb) == -1)
		{
			mb->release();
			return -1;
		}

		// Skip the bytes of the message
		if (!input.skip_bytes(header.msg_size))
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to skip header bytes\n")),-1);
	}
		
	return 0;
}

int OOCore_Transport_Base::open_channel(const OOObj::char_t* service, OOCore_Channel** channel)
{
	return connect_channel_i(service,channel);
}

int OOCore_Transport_Base::create_object(const OOObj::char_t* service, const OOObj::GUID& iid, OOObj::Object** ppVal)
{
	OOCore_Channel* channel;
	if (open_channel(service,&channel) != 0)
		return -1;
	
	// Create a proxy handler
	OOCore_ProxyStub_Handler* ph;
	ACE_NEW_NORETURN(ph,OOCore_ProxyStub_Handler(channel));
	if (ph == 0)
	{
		channel->close();
		return -1;
	}

	// Try to create the first proxy on it
	OOObj::Object_Ptr<OOObj::Object> obj;
	if (ph->create_first_proxy(&obj) != 0)
	{
		channel->close();
		return -1;
	}

	// QI for the requested interface
	if (obj->QueryInterface(iid,ppVal) == -1)
	{
		channel->close();
		return -1;
	}

	return 0;
}

int OOCore_Transport_Base::connect_channel_i(const OOObj::char_t* name, OOCore_Channel** channel)
{
	++m_conn_count;

	ACE_Active_Map_Manager_Key key;
	if (connect_channel(name,key,channel) != 0)
	{
		--m_conn_count;
		return -1;
	}

	// Now try to handle all the recv'd packets for the arrived channel

	// Make a copy of all the relevant channel messages
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);
	std::multimap<ACE_Active_Map_Manager_Key,ACE_Message_Block*>::iterator b=m_pending_list.find(key);
	std::multimap<ACE_Active_Map_Manager_Key,ACE_Message_Block*>::iterator e;
	
	for (e=b;e->first==key;++e)
	{
	}

	// Remove them from the map
	std::multimap<ACE_Active_Map_Manager_Key,ACE_Message_Block*> mcopy(b,e);
	m_pending_list.erase(key);

	// If we are the last outstanding connect, dump extraneous messages
	if (--m_conn_count==0)
	{
		for (std::multimap<ACE_Active_Map_Manager_Key,ACE_Message_Block*>::iterator i=m_pending_list.find(key);i->first==key;++i)
		{
			i->second->release();
		}
		m_pending_list.clear();
	}
	guard.release();

	// Now process all messages
	for (std::multimap<ACE_Active_Map_Manager_Key,ACE_Message_Block*>::iterator i=mcopy.begin();i!=mcopy.end();++i)
	{
		OOCore_Transport_MsgHeader header;
		header.key = i->first;
		header.msg_size = i->second->length();

		ACE_InputCDR input(i->second);
		i->second->release();

		if (process_msg(header,input) != 0)
			return -1;
	}
		
	return 0;
}

bool OOCore_Transport_Base::await_connect(void* p)
{
	OOCore_Transport_Base* pThis = static_cast<OOCore_Transport_Base*>(p);

	return (pThis->m_connected==CONNECTED);
}

int OOCore_Transport_Base::connect_primary_channel(OOCore_Channel** channel)
{
	if (m_connected != NOT_CONNECTED)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Attempting to connect an already connected transport\n")),-1);

	m_connected = CONNECTING;
	m_conn_channel = channel;
		
	// Spin waiting for a response
	ACE_Time_Value wait(4);
	return OOCore_RunReactorEx(&wait,&OOCore_Transport_Base::await_connect,this);
}

int OOCore_Transport_Base::connect_secondary_channel(ACE_Active_Map_Manager_Key& key, OOCore_Channel** channel, ACE_Message_Block* mb)
{
	// Create a channel for control messages
	OOCore_Channel* our_channel;
	OOCore_Channel* their_channel;
	if (OOCore_Channel::create(our_channel,their_channel) == -1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to create channel\n")),-1);

	// Add the channel to the map
	if (add_channel(our_channel,key) == -1)
	{
		our_channel->close();
		return -1;
	}

	ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Transport %@ open channel %u:%u\n"),this,key.slot_index(),key.slot_generation()));

	// Send mb to the new channel
	if (mb!=0 && our_channel->send(mb) != 0)
	{
		our_channel->close();
		return -1;
	}

	*channel = their_channel;
	
	return 0;
}

int OOCore_Transport_Base::connect_first_channel_i(const OOCore_Transport_MsgHeader& header, ACE_InputCDR& input)
{
	OOCore_Channel* their_channel;
	ACE_Message_Block* mb = input.start()->duplicate();
	
	// Skip the bytes of the message
	if (!input.skip_bytes(header.msg_size))
	{
		mb->release();
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to skip header bytes\n")),-1);
	}

	ACE_Active_Map_Manager_Key key2 = header.key;
	if (connect_secondary_channel(key2,&their_channel,mb) != 0)
	{
		mb->release();
		m_connected = NOT_CONNECTED;
		return -1;
	}

	*m_conn_channel = their_channel;
	m_connected = CONNECTED;

	return 0;
}

int OOCore_Transport_Base::accept_channel(OOCore_Channel*& channel, ACE_Active_Map_Manager_Key& key)
{
	return connect_secondary_channel(key,&channel);
}

int OOCore_Transport_Base::add_channel(OOCore_Channel* channel, ACE_Active_Map_Manager_Key& key)
{
	// Create a new handler
	OOCore_Transport_Handler* handler;
	ACE_NEW_RETURN(handler,OOCore_Transport_Handler(channel,this),-1);

	// Add to the map
	if (bind_channel(channel,key) != 0)
		return -1;

	// Set the handler's key
	handler->channel_key(key);

	return 0;
}

int OOCore_Transport_MsgHeader::read(ACE_InputCDR& input)
{
	// Read the header size
	if (input.length() < sizeof(this->header_size))
		return 1;
	if (!input.read_ushort(this->header_size))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to reader header size\n")),-1);

	// Check the header size
	if (this->header_size != OOCore_Transport_MsgHeader::v1_size() /*&&
		this->header_size != OOCore_Transport_MsgHeader::v2_size()*/ )
	{
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Bad header\n")),-1);
	}

	// Check the header length
	if (input.length() < this->header_size - sizeof(this->header_size))
		return 1;

	// Read the v1 header

	// Read the message length
	if (!input.read_ulong(this->msg_size))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to read message length\n")),-1);

	// Read the channel key
	if (!(input >> this->key))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to read channel key\n")),-1);

	/*// Read the v2 header
	if (this->header_size == OOCore_Transport_MsgHeader::v2_size())
	{
	}*/

	// Read the msg data
	if (input.length() < this->msg_size)
		return 1;
	
	return 0;
}

int OOCore_Transport_MsgHeader::write(ACE_OutputCDR& output)
{
	// Init a client header
	if (!output.write_ushort(this->v1_size()))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to write header size\n")),-1);

	if (!output.write_ulong(this->msg_size))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to write message size\n")),-1);

	if (!(output << key))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to write channel key\n")),-1);
	
	return 0;
}

int OOCore_Transport_Handler::handle_recv(ACE_Message_Block* mb)
{
	// Build the header block
	OOCore_Transport_MsgHeader header;
	header.msg_size = mb->total_length();
	header.key = m_key;

	// Write the header block
	ACE_OutputCDR output;
	if (header.write(output) == -1)
	{
		mb->release();
		return -1;
	}

	// Duplicate the output message block
	ACE_Message_Block* ob = output.begin()->duplicate();

	// Skip to the end of the cont() chain
	ACE_Message_Block* c;
	for (c=ob;c->cont()!=0;c=c->cont());
	
	// Append the message block
	c->cont(mb);
	
	// Forward the message to the real client
	if (m_transport->send(ob) == -1)
	{
		ob->release();
		return -1;
	}
	
	return 0;
}

int OOCore_Transport_Handler::handle_close()
{
	ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Transport %@ close channel %u:%u\n"),m_transport,m_key.slot_index(),m_key.slot_generation()));

	if (m_transport->unbind_channel(m_key) == 1)
	{
		// TODO Delay this...
		//m_transport->on_close();
	}

	return OOCore_Channel_Handler::handle_close();
}
