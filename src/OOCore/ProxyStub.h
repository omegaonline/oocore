#ifndef OOCORE_PROXYSTUB_H_INCLUDED_
#define OOCORE_PROXYSTUB_H_INCLUDED_

#include <map>

#include "./Proxy_Marshaller.h"
#include "./Stub_Marshaller.h"
#include "./Delegate.h"
#include "./Object_Marshaller.h"

namespace Impl
{
	class ProxyStub_Base
	{
	public:
		// Proxy constructor
		ProxyStub_Base(OOCore::ProxyStubManager* manager, const OOObj::cookie_t& key);

		// Stub constructor
		ProxyStub_Base(OOCore::ProxyStubManager* manager, OOObj::Object* obj);

		virtual ~ProxyStub_Base();

	protected:
		Proxy_Marshaller method(size_t method, OOObj::bool_t sync = true);
		
		template <class T>
		Array_Marshaller<T> array(T** pArr, OOObj::uint32_t size_index, bool in = false)
		{
			return Array_Marshaller<T>(size_index,pArr,in);
		}

		template <class T>
		Array_Marshaller<T> array(T* pArr, OOObj::uint32_t size_index)
		{
			return Array_Marshaller<T>(size_index,pArr);
		}

		Object_Marshaller object(const OOObj::guid_t& iid, OOObj::Object** ppObj, bool in = false);
		Object_Marshaller object(const OOObj::guid_t& iid, OOObj::Object* pObj);

		template <class T>
		Object_Marshaller object(T** ppObj, bool in = false)
		{
			return Object_Marshaller(ppObj,in);
		}
	
		template <class T>
		Object_Marshaller object(T* pObj)
		{
			return Object_Marshaller(pObj);
		}

		OOObj::int32_t AddRef_i();
		OOObj::int32_t Release_i();
		OOObj::int32_t QueryInterface_i(const OOObj::guid_t& iid, OOObj::Object** ppVal);

		int Invoke_i(OOObj::uint32_t method, OOObj::int32_t& ret_code, OOCore::InputStream* input, OOCore::OutputStream* output);

		void add_delegate(size_t method, Delegate::Base* del);

		bool m_bStub;

	private:
		OOObj::cookie_t m_key;
		OOCore::Object_Ptr<OOObj::Object> m_object;
		OOCore::Object_Ptr<OOCore::ProxyStubManager> m_manager;
		ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;
		std::map<size_t,Delegate::Base*> dispatch_tbl;
		
		OOObj::int32_t addref_id;
		OOObj::int32_t release_id;
		OOObj::int32_t qi_id;

		int invoke(OOObj::uint32_t method, OOObj::int32_t& ret_code, Stub_Marshaller& mshl);
		
		OOObj::int32_t Release_proxy();
		OOObj::int32_t Release_stub();
	};
};

namespace Impl
{
	template <class OBJECT>
	class ProxyStub_Impl : 
		public Impl::ProxyStub_Base,
		public OBJECT,
		public OOCore::Stub
	{
	public:
		// Proxy constructor
		ProxyStub_Impl(OOCore::ProxyStubManager* manager, const OOObj::cookie_t& key) :
			ProxyStub_Base(manager,key)
		{}

		// Stub constructor
		ProxyStub_Impl(OOCore::ProxyStubManager* manager, OOObj::Object* obj) :
			ProxyStub_Base(manager,obj)
		{}

		OOObj::int32_t AddRef()
		{
			return AddRef_i();
		}

		OOObj::int32_t Release()
		{
			return Release_i();
		}

		OOObj::int32_t QueryInterface(const OOObj::guid_t& iid, OOObj::Object** ppVal)
		{
			if (!ppVal)
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);

			if (m_bStub)
			{
				if (iid == OOObj::Object::IID ||
					iid == OOCore::Stub::IID)
				{
					AddRef();
					*ppVal = static_cast<OOCore::Stub*>(this);
					return 0;
				}

				return -1;
			}
			else
			{
				if (iid == OOObj::Object::IID ||
					iid == OBJECT::IID)
				{
					AddRef();
					*ppVal = static_cast<OBJECT*>(this);
					return 0;
				}

				return QueryInterface_i(iid,ppVal);
			}
		}

		int Invoke(OOObj::uint32_t method, OOObj::int32_t& ret_code, OOCore::InputStream* input, OOCore::OutputStream* output)
		{
			return Invoke_i(method,ret_code,input,output);
		}
	};
};

#define CREATE_AUTO_STUB(iface,manager,obj) \
	iface##_ProxyStub::create_stub(manager,obj)

#define CREATE_AUTO_PROXY(iface,manager,key) \
	iface##_ProxyStub::create_proxy(manager,key)

#define BEGIN_DECLARE_AUTO_PROXY_STUB(iface) \
	class iface##_ProxyStub : public Impl::ProxyStub_Impl<iface> { \
	public: static iface* create_proxy(OOCore::ProxyStubManager* manager, const OOObj::cookie_t& key) { iface##_ProxyStub* proxy; ACE_NEW_RETURN(proxy,iface##_ProxyStub(manager,key),0); return proxy;} \
	static OOCore::Stub* create_stub(OOCore::ProxyStubManager* manager, OOObj::Object* obj) { iface##_ProxyStub* stub; ACE_NEW_RETURN(stub,iface##_ProxyStub(manager,obj),0); return stub;} \
	private: typedef iface iface_class; typedef iface##_ProxyStub this_class; \
	iface##_ProxyStub(OOCore::ProxyStubManager* manager, OOObj::Object* obj) : Impl::ProxyStub_Impl<iface>(manager,obj) { init_delegates(obj); } \
	iface##_ProxyStub(OOCore::ProxyStubManager* manager, const OOObj::cookie_t& key) : 	Impl::ProxyStub_Impl<iface>(manager,key) {}
#define END_DECLARE_AUTO_PROXY_STUB() };

#define BEGIN_PROXY_MAP()

#define PROXY_ENTRY_0(function) \
	private: char function##_id; public: OOObj::int32_t function(void) { return (method(offsetof(this_class,function##_id)))(); }

#define PROXY_ENTRY_1(function,p1) \
	private: char function##_id; public: OOObj::int32_t function(p1) { return (method(offsetof(this_class,function##_id))
#define PROXY_PARAMS_1(m1) <<m1)(); } 

#define PROXY_ENTRY_2(function,p1,p2) \
	private: char function##_id; public: OOObj::int32_t function(p1,p2) { return (method(offsetof(this_class,function##_id))
#define PROXY_PARAMS_2(m1,m2) <<m1<<m2)(); } 

#define PROXY_ENTRY_3(function,p1,p2,p3) \
	private: char function##_id; public: OOObj::int32_t function(p1,p2,p3) { return (method(offsetof(this_class,function##_id))
#define PROXY_PARAMS_3(m1,m2,m3) <<m1<<m2<<m3)(); } 

#define PROXY_ENTRY_4(function,p1,p2,p3,p4) \
	private: char function##_id; public: OOObj::int32_t function(p1,p2,p3,p4) { return (method(offsetof(this_class,function##_id))
#define PROXY_PARAMS_4(m1,m2,m3,m4) <<m1<<m2<<m3<<m4)(); } 
#define END_PROXY_MAP()

#define BEGIN_STUB_MAP() \
	private: void init_delegates(OOObj::Object* obj) {
#define STUB_ENTRY_0(function) \
	add_delegate(offsetof(this_class,function##_id),(new Impl::Delegate::D0())->bind<iface_class,&iface_class::function>(obj));
#define STUB_ENTRY_1(function,p1) \
	add_delegate(offsetof(this_class,function##_id),(new Impl::Delegate::D1<p1>())->bind<iface_class,&iface_class::function>(obj));
#define STUB_ENTRY_2(function,p1,p2) \
	add_delegate(offsetof(this_class,function##_id),(new Impl::Delegate::D2<p1,p2>())->bind<iface_class,&iface_class::function>(obj));
#define STUB_ENTRY_3(function,p1,p2,p3) \
	add_delegate(offsetof(this_class,function##_id),(new Impl::Delegate::D3<p1,p2,p3>())->bind<iface_class,&iface_class::function>(obj));
#define STUB_ENTRY_4(function,p1,p2,p3,p4) \
	add_delegate(offsetof(this_class,function##_id),(new Impl::Delegate::D4<p1,p2,p3,p4>())->bind<iface_class,&iface_class::function>(obj));
#define END_STUB_MAP() }

#endif // OOCORE_PROXYSTUB_H_INCLUDED_
