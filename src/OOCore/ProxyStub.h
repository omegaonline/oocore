#ifndef OOCORE_PROXYSTUB_H_INCLUDED_
#define OOCORE_PROXYSTUB_H_INCLUDED_

#include <map>

#include <boost/preprocessor.hpp> 
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/comparison.hpp>
#include <boost/mpl/int.hpp>

#include "./ProxyStub_Macros.h"
#include "./ProxyStub_Types.h"

namespace OOProxyStub
{
	template <class OBJECT>
	class ProxyStub_Impl : 
		public OBJECT,
		public OOCore::Stub
	{
	public:
		// Proxy constructor
		ProxyStub_Impl(OOCore::ProxyStubManager* manager, const OOObject::cookie_t& key) :
			m_bStub(false),
			m_key(key),
			m_manager(manager),
			m_refcount(0)
		{}

		// Stub constructor
		ProxyStub_Impl(OOCore::ProxyStubManager* manager, OBJECT* obj) :
			m_bStub(true),
			m_object(obj),
 			m_manager(manager),
			m_refcount(0)
		{}

		OOObject::int32_t AddRef()
		{
			++m_refcount;
			return 0;
		}

		OOObject::int32_t Release_i(int id)
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

		OOObject::int32_t QueryInterface_i(int id, const OOObject::guid_t& iid, OOObject::Object** ppVal)
		{
			if (!ppVal)
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);

			if (m_bStub)
			{
				if (iid == OOObject::Object::IID ||
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
				if (iid == OOObject::Object::IID ||
					iid == OBJECT::IID)
				{
					AddRef();
					*ppVal = static_cast<OBJECT*>(this);
					return 0;
				}

				Impl::object_t<OOObject::Object**> ppVal_stub(ppVal,iid);
				Impl::marshaller_t qi_mshl(method(id));
				qi_mshl << iid;
				OOObject::int32_t ret = qi_mshl.send_and_recv();
				qi_mshl >> ppVal_stub;
				return ret;
			}
		}

		int Invoke(OOObject::uint32_t method, OOObject::int32_t& ret_code, OOCore::InputStream* input, OOCore::OutputStream* output)
		{
			return invoke_i(m_object,method,m_manager,ret_code,input,output);
		}

	protected:
		Impl::marshaller_t method(int id, OOObject::bool_t sync = true	/* TODO Extra flags here! */)
		{
			OOObject::uint32_t method = static_cast<OOObject::uint32_t>(id);
			OOObject::uint32_t trans_id;
			OOCore::Object_Ptr<OOCore::OutputStream> output;

			if (m_bStub || m_manager->CreateRequest(m_key,method,sync,&trans_id,&output) != 0)
				return Impl::marshaller_t();
			
			return Impl::marshaller_t(m_manager,sync,output,trans_id);
		}

		virtual int invoke_i(OBJECT* obj, OOObject::uint32_t method, OOCore::ProxyStubManager* manager, OOObject::int32_t& ret_code, OOCore::InputStream* input, OOCore::OutputStream* output) = 0;

	private:
		const bool m_bStub;
		OOObject::cookie_t m_key;
		OOCore::Object_Ptr<OBJECT> m_object;
		OOCore::Object_Ptr<OOCore::ProxyStubManager> m_manager;
		ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;
	};
};


#endif // OOCORE_PROXYSTUB_H_INCLUDED_
