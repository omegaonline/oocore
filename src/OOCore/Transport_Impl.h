#ifndef OOCORE_TRANSPORT_IMPL_H_INCLUDED_
#define OOCORE_TRANSPORT_IMPL_H_INCLUDED_

#include <queue>

#include <ace/Message_Block.h>
#include <ace/Method_Request.h>

#include "./ObjectManager.h"
#include "./InputStream_CDR.h"

#include "./OOCore_export.h"

namespace OOCore
{

class OOCore_Export Transport_Impl : 
	public Object_Impl<Transport>
{
public:
	Transport_Impl(void);
	
	int open_transport(const bool bAcceptor);
	int close_transport();

	OOObject::int32_t CreateObject(const OOObject::guid_t& clsid, const OOObject::guid_t& iid, OOObject::Object** ppVal);
	OOObject::int32_t AddObjectFactory(const OOObject::guid_t& clsid, ObjectFactory* pFactory);
	OOObject::int32_t RemoveObjectFactory(const OOObject::guid_t& clsid);
	
protected:
	virtual ~Transport_Impl(void);

	// Operations
	virtual int handle_recv();

	// Overrides	
	virtual int send(ACE_Message_Block* mb, ACE_Time_Value* wait = 0) = 0;
	virtual int recv(ACE_Message_Block*& mb, ACE_Time_Value* wait = 0) = 0;
			
private:
	ACE_Thread_Mutex m_lock;
	ACE_Message_Block* m_curr_block;
	Object_Ptr<ObjectManager> m_ptrOM;
	std::queue<ACE_Message_Block*> m_init_queue;
		
	struct msg_param : ACE_Method_Request
	{
		msg_param(Object_Ptr<ObjectManager>& om, const Object_Ptr<Impl::InputStream_CDR>& i) :
			OM(om),input(i)
		{}
			
		Object_Ptr<ObjectManager> OM;
		Object_Ptr<Impl::InputStream_CDR> input;

		int call()
		{
			OM->ProcessMessage(input);
			return 0;
		}
	};
	
	int process_block(ACE_Message_Block* mb);
	int read_header(ACE_InputCDR& input, size_t& msg_size);
	
// OOCore::Transport members
public:
	int CreateOutputStream(OutputStream** ppStream);
	int Send(OutputStream* output);
};

};

#endif // OOCORE_TRANSPORT_IMPL_H_INCLUDED_
