#ifndef OOCORE_PROXYSTUB_H_INCLUDED_
#define OOCORE_PROXYSTUB_H_INCLUDED_

#include "./ProxyStub_Types.h"
#include "./ProxyStub_Macros.h"

namespace OOCore
{
namespace Impl
{
	class invoker_t
	{
	public:
		template <class T, class I>
			static int Invoke(T* pT, I* iface, OOCore::ProxyStubManager* manager, OOObject::uint32_t method, OOObject::int32_t& ret_code, OOCore::Impl::InputStream_Wrapper& input, OOCore::Impl::OutputStream_Wrapper& output)
		{
			OOCORE_PS_DECLARE_INVOKE_TABLE()
		}
	};

	class marshaller_t
	{
	public:
		marshaller_t();
		marshaller_t(OOCore::ProxyStubManager* manager, OOObject::bool_t sync, OOCore::OutputStream* output, OOObject::uint32_t trans_id);

		template <class T>
		marshaller_t& operator <<(const T& val)
		{
			if (!m_failed)
				m_failed = (m_out.write(val)!=0);
			return *this;
		}

		template <class T>
		marshaller_t& operator <<(T* val)
		{
			if (!m_failed)
				m_failed = (m_out.write(*val)!=0);
			return *this;
		}

		template <class T>
		marshaller_t& operator <<(array_t<T>& val)
		{
			if (!m_failed)
				m_failed = (val.write(m_out)!=0);
			return *this;
		}

		template <class T>
		marshaller_t& operator <<(string_t<T>& val)
		{
			if (!m_failed)
				m_failed = (val.write(m_out)!=0);
			return *this;
		}

		template <class T>
		marshaller_t& operator <<(object_t<T*>& val)
		{
			if (!m_failed)
				m_failed = (val.write(m_manager,m_out)!=0);
			return *this;
		}
		
		template <class T>
		marshaller_t& operator >>(T* val)
		{
			if (!m_failed)
				m_failed = (m_in.read(*val)!=0);
			return *this;
		}

		template <class T>
		marshaller_t& operator >>(array_t<T**>& val)
		{
			if (!m_failed)
				m_failed = (val.read(m_in)!=0);
			return *this;
		}

		template <class T>
		marshaller_t& operator >>(object_t<T**>& val)
		{
			if (!m_failed)
				m_failed = (val.read(m_manager,m_in)!=0);
			return *this;
		}

		OOObject::int32_t send_and_recv();

	private:
		OOCore::Impl::InputStream_Wrapper	m_in;
		OOCore::Impl::OutputStream_Wrapper	m_out;
		bool								m_failed;
		OOCore::Object_Ptr<OOCore::ProxyStubManager> m_manager;
		const OOObject::bool_t			m_sync;
		const OOObject::uint32_t		m_trans_id;
	};
};

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
		ProxyStub_Impl(OOCore::ProxyStubManager* manager, const OOObject::cookie_t& key, OBJECT* obj) :
			m_bStub(true),
			m_key(key),
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
				{
					m_manager->ReleaseStub(m_key);
					delete this;
					return 1;
				}
			}
			else
			{
				if (--m_refcount == 0)
				{
					method(id).send_and_recv();
					m_manager->ReleaseProxy(m_key);
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
			return invoke_i(m_object,method,m_manager,ret_code,OOCore::Impl::InputStream_Wrapper(input),OOCore::Impl::OutputStream_Wrapper(output));
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

		virtual int invoke_i(OBJECT* obj, OOObject::uint32_t method, OOCore::ProxyStubManager* manager, OOObject::int32_t& ret_code, OOCore::Impl::InputStream_Wrapper& input, OOCore::Impl::OutputStream_Wrapper& output) = 0;

	private:
		const bool m_bStub;
		OOObject::cookie_t m_key;
		OOCore::Object_Ptr<OBJECT> m_object;
		OOCore::Object_Ptr<OOCore::ProxyStubManager> m_manager;
		ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;
	};
};


#endif // OOCORE_PROXYSTUB_H_INCLUDED_
