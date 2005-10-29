#ifndef _OOCORE_PROXYSTUB_HANDLER_H_INCLUDED_
#define _OOCORE_PROXYSTUB_HANDLER_H_INCLUDED_

#include <map>

#include <ace/CDR_Stream.h>
#include <ace/DLL.h>

#include "./Channel_Handler.h"
#include "./Marshaller.h"
#include "./Object_Proxy.h"
#include "./Object_Stub.h"
#include "./OOCore_Impl.h"
#include "./OOCore.h"

#include "./OOCore_export.h"

class OOCore_Export OOCore_ProxyStub_Handler :
	public OOCore_Channel_Handler
{
public:
	OOCore_ProxyStub_Handler(OOCore_Channel* channel);
	
	int create_first_stub(const OOObj::GUID& iid, OOObj::Object* obj);
	int create_first_proxy(OOObj::Object** proxy);

	int create_proxy(const OOObj::GUID& iid, const OOObj::cookie_t& key, OOObj::Object** proxy);
	int create_stub(const OOObj::GUID& iid, OOObj::Object* obj, OOObj::cookie_t& key);
	int remove_stub(const OOObj::cookie_t& key);
	
	OOCore_Proxy_Marshaller& create_marshaller(const ACE_Active_Map_Manager_Key& key, unsigned int method, bool sync, ACE_Time_Value* wait = 0);
	int remove_marshaller(const ACE_Active_Map_Manager_Key& trans_key);

	int get_response(const ACE_Active_Map_Manager_Key& trans_key, ACE_InputCDR*& input, ACE_Time_Value* wait);

	void PS_addref();
	void PS_release();

protected:
	virtual ~OOCore_ProxyStub_Handler(void);

private:
	struct proxystub_node
	{
		ACE_DLL dll;
		OOObj::CreateProxy_Function proxy_fn;
		OOObj::CreateStub_Function stub_fn;
	};

	struct response_wait
	{
		response_wait(OOCore_ProxyStub_Handler* t, const ACE_Active_Map_Manager_Key& k, ACE_InputCDR*& i) :
			pThis(t), trans_key(k), input(i)
		{ }

		OOCore_ProxyStub_Handler* pThis;
		const ACE_Active_Map_Manager_Key& trans_key;
		ACE_InputCDR*& input;
	};

	std::map<OOObj::GUID,ACE_Active_Map_Manager_Key> m_proxystub_map;
	ACE_Active_Map_Manager<proxystub_node*> m_dll_map;
	ACE_Active_Map_Manager<OOCore_Proxy_Marshaller*> m_transaction_map;
	ACE_Active_Map_Manager<OOCore_Object_Stub_Base*> m_stub_map;
	std::map<ACE_Active_Map_Manager_Key,ACE_InputCDR*> m_response_map; 
	ACE_Thread_Mutex m_lock;

	static proxystub_node m_core_node;
	static OOCore_Proxy_Marshaller m_failmshl;

	OOObj::Object** m_conn_proxy;
	bool m_connected;
	ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;
		
	int load_proxy_stub(const OOObj::GUID& iid, proxystub_node*& node);
	int handle_recv(ACE_Message_Block* mb);
	int recv_request(ACE_InputCDR* input);
	int recv_response(ACE_InputCDR* input);
	int handle_close();
	int handle_connect(ACE_InputCDR& input);

	static bool await_connect(void * p);
	static bool await_response(void* p);
	bool await_response_i(const ACE_Active_Map_Manager_Key& trans_key, ACE_InputCDR*& input);
};

#endif // _OOCORE_PROXYSTUB_HANDLER_H_INCLUDED_
