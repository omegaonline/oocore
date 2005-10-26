#ifndef _OOSVC_CHANNEL_ACCEPTOR_H_INCLUDED_
#define _OOSVC_CHANNEL_ACCEPTOR_H_INCLUDED_

#include "../OOCore/ProxyStub_Handler.h"

#include "./OOSvc_export.h"

class OOCore_Channel;

class OOSvc_Export OOSvc_Channel_Acceptor_Base
{
public:
	OOSvc_Channel_Acceptor_Base(const char* name);
	virtual ~OOSvc_Channel_Acceptor_Base();

	int open(bool bLocal);
	int close();
	const char* name();

	virtual int handle_accept(OOCore_Channel* channel) = 0;

private:
	const char* m_name;
};

template <class HANDLER>
class OOSvc_Channel_Acceptor : public OOSvc_Channel_Acceptor_Base
{
public:
	OOSvc_Channel_Acceptor(const char* name) :
	  OOSvc_Channel_Acceptor_Base(name)
	{
	}

	int handle_accept(OOCore_Channel* channel)
	{
		HANDLER* handler;
		ACE_NEW_RETURN(handler,HANDLER(channel),-1);

		return 0;
	}
};

template <class OBJECT, int (*CREATE_FN)(OBJECT**)>
class OOSvc_Object_Acceptor : public OOSvc_Channel_Acceptor_Base
{
public:
	OOSvc_Object_Acceptor(const char* name) :
	  OOSvc_Channel_Acceptor_Base(name)
	{
	}
	
	int handle_accept(OOCore_Channel* channel)
	{
		// Create an object instance
		OOObj::Object_Ptr<OBJECT> obj;
		if (CREATE_FN(&obj) != 0)
			return -1;

		// Create a stub handler
		OOCore_ProxyStub_Handler* sh;
		ACE_NEW_NORETURN(sh,OOCore_ProxyStub_Handler(channel));
		if (sh==0)
			return -1;
		
		if (sh->create_first_stub(OBJECT::IID,obj) != 0)
			return -1;

		return 0;
	}
};

#endif // _OOSVC_CHANNEL_ACCEPTOR_H_INCLUDED_
