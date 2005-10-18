#include "./Object_Stub.h"

#include "./Array_Marshaller.h"

OOCore_Object_Stub_Base::OOCore_Object_Stub_Base(OOObj::Object* object) :
	m_object(object), m_refcount(1)
{
}

OOCore_Object_Stub_Base::~OOCore_Object_Stub_Base(void)
{
}

void OOCore_Object_Stub_Base::add_delegate(size_t method, const OOObj::Delegate::Base* del)
{
	if (method>1)
		dispatch_tbl[method] = del;
}

int OOCore_Object_Stub_Base::close()
{
	delete this;
	return 0;
}

int OOCore_Object_Stub_Base::invoke(unsigned int method, int& ret_code, OOCore_Stub_Marshaller& mshl)
{
	ret_code = 0;

	if (method==0)
	{
		++m_refcount;
	}
	else if (method==1)
	{
		if (m_refcount<=0)
			ret_code = -1;
		
		if (--m_refcount == 0)
			return 1;
	}
	else
	{
		const OOObj::Delegate::Base* del;
		if (dispatch_tbl.get(del,method) != 0)
		{
			ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid method index %ul\n"),method));
			return -1;
		}

		ret_code = del->invoke(mshl);
	}

	return 0;
}
