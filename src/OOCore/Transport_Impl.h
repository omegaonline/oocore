#ifndef OOCORE_TRANSPORT_IMPL_H_INCLUDED_
#define OOCORE_TRANSPORT_IMPL_H_INCLUDED_

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
	
	int open_transport();
	int close_transport();

	OOObj::int32_t CreateObject(const OOObj::char_t* class_name, const OOObj::guid_t& iid, OOObj::Object** ppVal);
	
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
		
	struct msg_param : ACE_Method_Request
	{
		msg_param(ObjectManager* om, Impl::InputStream_CDR* i) :
			OM(om),input(i)
		{}
			
		Object_Ptr<ObjectManager> OM;
		Object_Ptr<Impl::InputStream_CDR> input;

		int call()
		{
			return OM->ProcessMessage(input);
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
