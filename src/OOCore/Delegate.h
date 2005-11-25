//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "ProxyStub.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_DELEGATE_H_INCLUDED_
#define OOCORE_DELEGATE_H_INCLUDED_

#include "./OOCore_Impl.h"

namespace Marshall_A
{
namespace Delegate
{

class Base
{
public:
	virtual OOObj::int32_t invoke(Stub_Marshaller& mshl) const = 0;
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
    Base* bind(OOObj::Object* obj)
    {
		object_ptr = obj;
        stub_ptr = &method_stub<T, TMethod>;
		return this;
    }

	int invoke(Stub_Marshaller& mshl) const
	{
		return (*stub_ptr)(object_ptr);
	}

private:
    typedef int (*stub_type)(void* object_ptr);

    DELEG_DATA_DECL

    template <class T, int (T::*TMethod)(void)>
    static OOObj::int32_t method_stub(void* object_ptr)
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

    template <class T, OOObj::int32_t (T::*TMethod)(P1)>
	Base* bind(OOObj::Object* obj)
    {
		object_ptr = obj;
        stub_ptr = &method_stub<T, TMethod>;
		return this;
    }

	OOObj::int32_t invoke(Stub_Marshaller& mshl) const
	{
		P1 p1(mshl.EXPLICIT_TEMPLATE(unpack,P1)());
		return (*stub_ptr)(object_ptr,p1);
	}

private:
    typedef OOObj::int32_t (*stub_type)(void* object_ptr, P1);

    DELEG_DATA_DECL

    template <class T, OOObj::int32_t (T::*TMethod)(P1)>
    static OOObj::int32_t method_stub(void* object_ptr, P1 p1)
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

    template <class T, OOObj::int32_t (T::*TMethod)(P1,P2)>
    Base* bind(OOObj::Object* obj)
    {
		object_ptr = obj;
        stub_ptr = &method_stub<T, TMethod>;
		return this;
    }

	OOObj::int32_t invoke(Stub_Marshaller& mshl) const
	{
		P1 p1(mshl.EXPLICIT_TEMPLATE(unpack,P1)());
		P2 p2(mshl.EXPLICIT_TEMPLATE(unpack,P2)());
		return (*stub_ptr)(object_ptr,p1,p2);
	}

private:
    typedef OOObj::int32_t (*stub_type)(void* object_ptr, P1, P2);

    DELEG_DATA_DECL

    template <class T, OOObj::int32_t (T::*TMethod)(P1,P2)>
    static OOObj::int32_t method_stub(void* object_ptr, P1 p1, P2 p2)
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

    template <class T, OOObj::int32_t (T::*TMethod)(P1,P2,P3)>
    Base* bind(OOObj::Object* obj)
    {
		object_ptr = obj;
        stub_ptr = &method_stub<T, TMethod>;
		return this;
    }

	OOObj::int32_t invoke(Stub_Marshaller& mshl) const
	{
		P1 p1(mshl.EXPLICIT_TEMPLATE(unpack,P1)());
		P2 p2(mshl.EXPLICIT_TEMPLATE(unpack,P2)());
		P3 p3(mshl.EXPLICIT_TEMPLATE(unpack,P3)());
		return (*stub_ptr)(object_ptr,p1,p2,p3);
	}

private:
    typedef OOObj::int32_t (*stub_type)(void* object_ptr, P1, P2, P3);

    DELEG_DATA_DECL

    template <class T, OOObj::int32_t (T::*TMethod)(P1,P2,P3)>
    static OOObj::int32_t method_stub(void* object_ptr, P1 p1, P2 p2, P3 p3)
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

    template <class T, OOObj::int32_t (T::*TMethod)(P1,P2,P3,P4)>
    Base* bind(OOObj::Object* obj)
    {
		object_ptr = obj;
        stub_ptr = &method_stub<T, TMethod>;
		return this;
    }

	OOObj::int32_t invoke(Stub_Marshaller& mshl) const
	{
		P1 p1(mshl.EXPLICIT_TEMPLATE(unpack,P1)());
		P2 p2(mshl.EXPLICIT_TEMPLATE(unpack,P2)());
		P3 p3(mshl.EXPLICIT_TEMPLATE(unpack,P3)());
		P4 p4(mshl.EXPLICIT_TEMPLATE(unpack,P4)());
		return (*stub_ptr)(object_ptr,p1,p2,p3,p4);
	}

private:
    typedef OOObj::int32_t (*stub_type)(void* object_ptr, P1, P2, P3, P4);

    DELEG_DATA_DECL

    template <class T, OOObj::int32_t (T::*TMethod)(P1,P2,P3,P4)>
	static OOObj::int32_t method_stub(void* object_ptr, P1 p1, P2 p2, P3 p3, P4 p4)
    {
        T* p = static_cast<T*>(object_ptr);
        return (p->*TMethod)(p1,p2,p3,p4);
    }
};

};
};

#endif // OOCORE_DELEGATE_H_INCLUDED_
