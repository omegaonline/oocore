//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "Transport_Connector.h" instead
//
//////////////////////////////////////////////////////

#ifndef _OOCORE_TRANSPORT_BASE_H_INCLUDED_
#define _OOCORE_TRANSPORT_BASE_H_INCLUDED_

#include <list>

#include <ace/Active_Map_Manager.h>
#include <ace/CDR_Stream.h>

#include "./Channel_Handler.h"
#include "./OOCore.h"
#include "./Object.h"

#include "./OOCore_export.h"

class OOCore_Transport_MsgHeader;
class OOCore_ProxyStub_Handler;

class OOCore_Export OOCore_Transport_Base
{
	friend class OOCore_Transport_MsgHeader;
	friend class OOCore_Transport_Handler;

public:
	int open_channel(const OOObj::char_t* service, OOCore_Channel** channel);
	int create_object(const OOObj::char_t* service, const OOObj::GUID& iid, OOObj::Object** ppVal);

protected:
	OOCore_Transport_Base(void);
	virtual ~OOCore_Transport_Base(void);

	// Transports should use the following functions
	int recv(ACE_Message_Block* mb);
	int connect_primary_channel(OOCore_Channel** channel);
	int accept_channel(OOCore_Channel*& channel, ACE_Active_Map_Manager_Key& key);
	int addref();
	int release();
	int close_transport();
		
	// All Transports must implement the following functions
	virtual int send(ACE_Message_Block* mb, ACE_Time_Value* wait = 0) = 0;
	virtual int find_channel(const ACE_Active_Map_Manager_Key& key, OOCore_Channel*& channel) = 0;
	virtual int bind_channel(OOCore_Channel* channel, ACE_Active_Map_Manager_Key& key) = 0;
	virtual int unbind_channel(const ACE_Active_Map_Manager_Key& key) = 0;
	virtual int close_all_channels() = 0;
	virtual int connect_channel(const OOObj::char_t* name, ACE_Active_Map_Manager_Key& key, OOCore_Channel** channel) = 0;
	
private:
	enum
	{
		NOT_CONNECTED,
		CONNECTING,
		CONNECTED,
		CLOSED
	} m_connected;
	
	ACE_Thread_Mutex m_lock;
	ACE_Message_Block* m_curr_block;
	OOCore_Channel** m_conn_channel;
	ACE_Atomic_Op<ACE_Thread_Mutex,long> m_conn_count;
	ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;
	std::multimap<ACE_Active_Map_Manager_Key,ACE_Message_Block*>  m_pending_list;
	
	int add_channel(OOCore_Channel* channel, ACE_Active_Map_Manager_Key& key);
	int connect_first_channel_i(const OOCore_Transport_MsgHeader& header, ACE_InputCDR& input);
	int bad_channel(const OOCore_Transport_MsgHeader& header, ACE_InputCDR& input);
	int connect_channel_i(const OOObj::char_t* name, OOCore_Channel** channel);
	int process_msg(const OOCore_Transport_MsgHeader& header, ACE_InputCDR& input);
	int connect_secondary_channel(ACE_Active_Map_Manager_Key& key, OOCore_Channel** channel, ACE_Message_Block* mb = 0);

	static bool await_connect(void* p);
};

class OOCore_Transport_MsgHeader
{
public:
	ACE_CDR::ULong	msg_size;
	ACE_Active_Map_Manager_Key	key;

	// Returns 0 on success, -1 on error, 1 on pending
	int read(ACE_InputCDR& input);
	int write(ACE_OutputCDR& output);

private:
	ACE_CDR::UShort	header_size;

	// Helpers
	static const ACE_CDR::UShort v1_size()
	{
		return static_cast<ACE_CDR::UShort>((sizeof(ACE_CDR::UShort) + sizeof(ACE_CDR::ULong) + ACE_Active_Map_Manager_Key::size()));
	}
};

class OOCore_Transport_Handler : public OOCore_Channel_Handler
{
public:
	OOCore_Transport_Handler(OOCore_Channel* channel, OOCore_Transport_Base* transport) :
		OOCore_Channel_Handler(channel),
		m_transport(transport)
	{
		m_transport->addref();
	}

	void channel_key(const ACE_Active_Map_Manager_Key& key)
	{
		m_key = key;
	}

	OOCore_Transport_Base* m_transport;
	ACE_Active_Map_Manager_Key m_key;

protected:
	virtual ~OOCore_Transport_Handler() 
	{
		m_transport->release();
	};
	
private:
	int handle_recv(ACE_Message_Block* mb);
	int handle_close();
};

#endif // _OOCORE_TRANSPORT_BASE_H_INCLUDED_
