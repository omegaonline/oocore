//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "Object.h" instead
//
//////////////////////////////////////////////////////

#ifndef _OOCORE_DELEGATE_H_INCLUDED_
#define _OOCORE_DELEGATE_H_INCLUDED_

#include "./Stub_Marshaller.h"
#include "./OOCore_Impl.h"

namespace OOObj
{
namespace Delegate
{

class Base
{
public:
	virtual int invoke(OOCore_Stub_Marshaller& mshl) const = 0;
};

#define DELEG_CONSTRUCTOR(c) \
	c() throw() : object_ptr(0), stub_ptr(0) {}

#define DELEG_DATA_DECL \
	void* object_ptr; stub_type stub_ptr;

class D0 : 
	public Base
{
public:
	DELEG_CONSTRUCTOR(D0)

    template <class T, int (T::*TMethod)(void)>
    const Base* bind(OOObj::Object* obj)
    {
        object_ptr = obj;
        stub_ptr = &method_stub<T, TMethod>;
		return this;
    }

	int invoke(OOCore_Stub_Marshaller& mshl) const
	{
		return (*stub_ptr)(object_ptr);
	}

private:
    typedef int (*stub_type)(void* object_ptr);

    DELEG_DATA_DECL

    template <class T, int (T::*TMethod)(void)>
    static int method_stub(void* object_ptr)
    {
        T* p = static_cast<T*>(object_ptr);
        return (p->*TMethod)();
    }
};

template <class P1>
class D1 : 
	public Base
{
public:
	DELEG_CONSTRUCTOR(D1)

    template <class T, int (T::*TMethod)(P1)>
	const Base* bind(OOObj::Object* obj)
    {
        object_ptr = obj;
        stub_ptr = &method_stub<T, TMethod>;
		return this;
    }

	int invoke(OOCore_Stub_Marshaller& mshl) const
	{
		P1 p1(mshl.EXPLICIT_TEMPLATE(unpack,P1)());
		return (*stub_ptr)(object_ptr,p1);
	}

private:
    typedef int (*stub_type)(void* object_ptr, P1);

    DELEG_DATA_DECL

    template <class T, int (T::*TMethod)(P1)>
    static int method_stub(void* object_ptr, P1 p1)
    {
        T* p = static_cast<T*>(object_ptr);
        return (p->*TMethod)(p1);
    }
};

template <class P1, class P2>
class D2 : 
	public Base
{
public:
	DELEG_CONSTRUCTOR(D2)

    template <class T, int (T::*TMethod)(P1,P2)>
    const Base* bind(OOObj::Object* obj)
    {
        object_ptr = obj;
        stub_ptr = &method_stub<T, TMethod>;
		return this;
    }

	int invoke(OOCore_Stub_Marshaller& mshl) const
	{
		P1 p1(mshl.EXPLICIT_TEMPLATE(unpack,P1)());
		P2 p2(mshl.EXPLICIT_TEMPLATE(unpack,P2)());
		return (*stub_ptr)(object_ptr,p1,p2);
	}

private:
    typedef int (*stub_type)(void* object_ptr, P1, P2);

    DELEG_DATA_DECL

    template <class T, int (T::*TMethod)(P1,P2)>
    static int method_stub(void* object_ptr, P1 p1, P2 p2)
    {
        T* p = static_cast<T*>(object_ptr);
        return (p->*TMethod)(p1,p2);
    }
};

template <class P1, class P2, class P3>
class D3 : 
	public Base
{
public:
	DELEG_CONSTRUCTOR(D3)

    template <class T, int (T::*TMethod)(P1,P2,P3)>
    const Base* bind(OOObj::Object* obj)
    {
        object_ptr = obj;
        stub_ptr = &method_stub<T, TMethod>;
		return this;
    }

	int invoke(OOCore_Stub_Marshaller& mshl) const
	{
		P1 p1(mshl.EXPLICIT_TEMPLATE(unpack,P1)());
		P2 p2(mshl.EXPLICIT_TEMPLATE(unpack,P2)());
		P3 p3(mshl.EXPLICIT_TEMPLATE(unpack,P3)());
		return (*stub_ptr)(object_ptr,p1,p2,p3);
	}

private:
    typedef int (*stub_type)(void* object_ptr, P1, P2, P3);

    DELEG_DATA_DECL

    template <class T, int (T::*TMethod)(P1,P2,P3)>
    static int method_stub(void* object_ptr, P1 p1, P2 p2, P3 p3)
    {
        T* p = static_cast<T*>(object_ptr);
        return (p->*TMethod)(p1,p2,p3);
    }
};

template <class P1, class P2, class P3, class P4>
class D4 : 
	public Base
{
public:
	DELEG_CONSTRUCTOR(D4)

    template <class T, int (T::*TMethod)(P1,P2,P3,P4)>
    const Base* bind(OOObj::Object* obj)
    {
        object_ptr = obj;
        stub_ptr = &method_stub<T, TMethod>;
		return this;
    }

	int invoke(OOCore_Stub_Marshaller& mshl) const
	{
		P1 p1(mshl.EXPLICIT_TEMPLATE(unpack,P1)());
		P2 p2(mshl.EXPLICIT_TEMPLATE(unpack,P2)());
		P3 p3(mshl.EXPLICIT_TEMPLATE(unpack,P3)());
		P4 p4(mshl.EXPLICIT_TEMPLATE(unpack,P4)());
		return (*stub_ptr)(object_ptr,p1,p2,p3,p4);
	}

private:
    typedef int (*stub_type)(void* object_ptr, P1, P2, P3, P4);

    DELEG_DATA_DECL

    template <class T, int (T::*TMethod)(P1,P2,P3,P4)>
	static int method_stub(void* object_ptr, P1 p1, P2 p2, P3 p3, P4 p4)
    {
        T* p = static_cast<T*>(object_ptr);
        return (p->*TMethod)(p1,p2,p3,p4);
    }
};

};
};

#endif // _OOCORE_DELEGATE_H_INCLUDED_
