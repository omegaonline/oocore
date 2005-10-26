//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "Object.h" instead
//
//////////////////////////////////////////////////////

#ifndef _OOCORE_OBJECT_STUB_H_INCLUDED_
#define _OOCORE_OBJECT_STUB_H_INCLUDED_

#include <ace/Vector_T.h>

#include "./Delegate.h"
#include "./Object_Impl.h"

class OOCore_ProxyStub_Handler;

#include "./OOCore_export.h"

class OOCore_Export OOCore_Object_Stub_Base
{
	friend class OOCore_ProxyStub_Handler;

protected:
	OOCore_Object_Stub_Base(OOObj::Object* obj);
	virtual ~OOCore_Object_Stub_Base(void);
	
	void add_delegate(size_t method, const OOObj::Delegate::Base* del);
	
private:
	OOCore_Object_Stub_Base() {};

	int close();
	int invoke(unsigned int method, int& ret_code, OOCore_Stub_Marshaller& mshl);

	OOObj::Object_Ptr<OOObj::Object> m_object;
	ACE_Vector<const OOObj::Delegate::Base*,16> dispatch_tbl;
	ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;
};

namespace OOObj
{

// This is the definition of the exported CreateStub function
typedef int (*CreateStub_Function)(const OOObj::GUID& iid, Object* obj, OOCore_Object_Stub_Base** stub);

class Object_Stub : 
	public OOCore_Object_Stub_Base
{
public:
	Object_Stub(Object* obj) :
		OOCore_Object_Stub_Base(obj)
	{
		add_delegate(2,QueryInterface.bind<Object,&Object::QueryInterface>(obj));
	}

private:
	Delegate::D2<const OOObj::GUID&, Object**> QueryInterface;
};

};

#endif // _OOCORE_OBJECT_STUB_H_INCLUDED_
