#ifndef OOCORE_TRANSPORT_IMPL_H_INCLUDED_
#define OOCORE_TRANSPORT_IMPL_H_INCLUDED_

#include <ace/Message_Block.h>
#include <ace/Method_Request.h>

#include <queue>

#include "./ObjectManager.h"
#include "./InputStream_CDR.h"

#include "./OOCore_export.h"

namespace OOCore
{

class OOCore_Export Transport_Impl : 
	public OOUtil::Object_Root<Transport_Impl>,
	public OOObject::Transport
{
protected:
	Transport_Impl(void);
	virtual ~Transport_Impl(void);
	
	int open();

	// Operations
	int process_block(ACE_Message_Block* mb);

	// Overrides	
	virtual int send(ACE_Message_Block* mb, ACE_Time_Value* wait = 0) = 0;
	virtual int recv(ACE_Message_Block*& mb, ACE_Time_Value* wait = 0) = 0;

BEGIN_INTERFACE_MAP(Transport_Impl)
	INTERFACE_ENTRY(OOObject::Transport)
END_INTERFACE_MAP()

private:
	ACE_Thread_Mutex m_lock;
	ACE_Message_Block* m_curr_block;
	OOUtil::Object_Ptr<ObjectManager> m_ptrOM;

	struct msg_param : ACE_Method_Request
	{
		msg_param(OOUtil::Object_Ptr<ObjectManager>& om, Impl::InputStream_CDR* i) :
			OM(om),input(i)
		{}
			
		OOUtil::Object_Ptr<ObjectManager> OM;
		OOUtil::Object_Ptr<Impl::InputStream_CDR> input;

		int call()
		{
			OM->ProcessMessage(input);
			delete this;
			return 0;
		}
	};
	
	int read_header(ACE_InputCDR& input, size_t& msg_size);
	
// OOCore::Transport members
public:
	virtual int CreateOutputStream(OOObject::OutputStream** ppStream);
	virtual int Send(OOObject::OutputStream* output);
	virtual int Close();
};

};

#endif // OOCORE_TRANSPORT_IMPL_H_INCLUDED_
