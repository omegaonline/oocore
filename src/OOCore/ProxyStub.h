#ifndef OOCORE_PROXYSTUB_H_INCLUDED_
#define OOCORE_PROXYSTUB_H_INCLUDED_

#include <map>

#include <boost/preprocessor.hpp> 
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/comparison.hpp>
#include <boost/mpl/int.hpp>

#include "./Proxy_Marshaller.h"
#include "./Stub_Marshaller.h"
#include "./Delegate.h"
#include "./Object_Marshaller.h"

namespace Marshall_A
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

#include "./ProxyStub_Macros.h"
#include "./ProxyStub_Types.h"

namespace OOCore
{
namespace Proxy_Stub
{
	template <class OBJECT>
	class ProxyStub_Impl : 
		public OBJECT,
		public OOCore::Stub
	{
	public:
		// Proxy constructor
		ProxyStub_Impl(OOCore::ProxyStubManager* manager, const OOObj::cookie_t& key) :
			m_bStub(false),
			m_key(key),
			m_manager(manager),
			m_refcount(0)
		{}

		// Stub constructor
		ProxyStub_Impl(OOCore::ProxyStubManager* manager, OOObj::Object* obj) :
			m_bStub(true),
			m_object(obj),
 			m_manager(manager),
			m_refcount(0)
		{}

		OOObj::int32_t AddRef()
		{
			++m_refcount;
			return 0;
		}

		OOObj::int32_t Release_i(int id)
		{
			if (m_bStub)
			{
				if (--m_refcount == 0)
					delete this;
			}
			else
			{
				if (--m_refcount == 0)
				{
					method(id).send_and_recv();
					delete this;
				}
			}
			return 0;
		}

		OOObj::int32_t QueryInterface_i(int id, const OOObj::guid_t& iid, OOObj::Object** ppVal)
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

				Proxy_Stub::object_t<OOObj::Object**> ppVal_stub(ppVal,iid);
				marshaller_t qi_mshl(method(id));
				qi_mshl << iid;
				OOObj::int32_t ret = qi_mshl.send_and_recv();
				qi_mshl >> ppVal_stub;
				return ret;
			}
		}

		int Invoke(OOObj::uint32_t method, OOObj::int32_t& ret_code, OOCore::InputStream* input, OOCore::OutputStream* output)
		{
			return Invoke_i(method,ret_code,input,output);
		}

	protected:
		marshaller_t method(int id	/* TODO Extra flags here! */);

	private:
		const bool m_bStub;
		OOObj::cookie_t m_key;
		OOCore::Object_Ptr<OOObj::Object> m_object;
		OOCore::Object_Ptr<OOCore::ProxyStubManager> m_manager;
		ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;
	};
};
};

#endif // OOCORE_PROXYSTUB_H_INCLUDED_
