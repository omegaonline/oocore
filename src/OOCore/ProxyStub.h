#ifndef OOCORE_PROXYSTUB_H_INCLUDED_
#define OOCORE_PROXYSTUB_H_INCLUDED_

#include "./ProxyStub_Types.h"
#include "./ProxyStub_Macros.h"

namespace OOCore
{
namespace Impl
{
	class metainfo_t
	{
	public:
		template <class T, class I>
			static int Invoke(T* pT, I* iface, OOCore::Object_Ptr<OOCore::ProxyStubManager>& manager, OOObject::uint32_t method, OOCore::Impl::InputStream_Wrapper& input, OOCore::Impl::OutputStream_Wrapper& output)
		{
			OOCORE_PS_DECLARE_INVOKE_TABLE()
		}

		template <class T>
			static int GetMethodInfo(T* pT, size_t method, const OOObject::char_t** method_name, size_t* param_count, OOCore::TypeInfo::Method_Attributes_t* attributes, OOObject::uint16_t* wait_secs)
		{
			if (method_name==0 || param_count==0 || attributes==0 || wait_secs==0) 
			{
				errno = EINVAL; 
				ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid NULL value\n")),-1); 
			}

			OOCORE_PS_DECLARE_METHODINFO_TABLE()
		}

		template <class T>
			static int GetParamInfo(T* pT, size_t method, size_t param, const OOObject::char_t** param_name, OOCore::TypeInfo::Type_t* type)
		{
			if (param_name==0 || type==0) 
			{ 
				errno = EINVAL; 
				ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid NULL value\n")),-1); 
			}

			OOCORE_PS_DECLARE_PARAMINFO_TABLE()
		}

		template <class T>
			static int GetParamAttributeData(T* pT, size_t method, size_t param, const void* attr, bool& is_index)
		{
			if (attr==0) 
			{ 
				errno = EINVAL; 
				ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid NULL value\n")),-1); 
			}

			OOCORE_PS_DECLARE_PARAMATTR_TABLE()
		}
	};

	template <class T> class type_info_t;
	OOCORE_PS_METATYPE_BUILDER(bool_t);
	OOCORE_PS_METATYPE_BUILDER(char_t);
	OOCORE_PS_METATYPE_BUILDER(byte_t);
	OOCORE_PS_METATYPE_BUILDER(int16_t);
	OOCORE_PS_METATYPE_BUILDER(uint16_t);
	OOCORE_PS_METATYPE_BUILDER(int32_t);
	OOCORE_PS_METATYPE_BUILDER(uint32_t);
	OOCORE_PS_METATYPE_BUILDER(int64_t);
	OOCORE_PS_METATYPE_BUILDER(uint64_t);
	OOCORE_PS_METATYPE_BUILDER(real4_t);
	OOCORE_PS_METATYPE_BUILDER(real8_t);
	OOCORE_PS_METATYPE_BUILDER(guid_t);

	template <class T> class type_info_t<const T>
	{
	public:
		enum { value = ( type_info_t<T>::value | OOCore::TypeInfo::cpp_const) };
	};

	template <class T> class type_info_t<T&>
	{
	public:
		enum { value = ( type_info_t<T>::value | OOCore::TypeInfo::cpp_ref) };
	};

	template <class T> class type_info_t<T*>
	{
	public:
		enum { value = type_info_t<T>::value };
	};
	
	class OOCore_Export marshaller_t
	{
	public:
		marshaller_t();
		marshaller_t(OOCore::ProxyStubManager* manager, TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, OOCore::OutputStream* output, OOObject::uint32_t trans_id);

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
		OOCore::Impl::InputStream_Wrapper				m_in;
		OOCore::Impl::OutputStream_Wrapper				m_out;
		bool											m_failed;
		OOCore::Object_Ptr<OOCore::ProxyStubManager>	m_manager;
		const TypeInfo::Method_Attributes_t				m_flags;
		const OOObject::uint32_t						m_trans_id;
		const OOObject::uint16_t						m_wait_secs;
	};
};

	template <class OBJECT>
	class ProxyStub_Impl : 
		public OBJECT,
		public OOCore::Proxy,
		public OOCore::Stub,
		public OOCore::TypeInfo
	{
	public:
		// TypeInfo constructor
		ProxyStub_Impl() :
			m_type(TYPEINFO),
			m_manager(0),
			m_refcount(0)
		{}

		// Proxy constructor
		ProxyStub_Impl(OOCore::ProxyStubManager* manager, const OOCore::ProxyStubManager::cookie_t& key) :
			m_type(PROXY),
			m_key(key),
			m_manager(manager),
			m_refcount(0)
		{}

		// Stub constructor
		ProxyStub_Impl(OOCore::ProxyStubManager* manager, const OOCore::ProxyStubManager::cookie_t& key, OBJECT* obj) :
			m_type(STUB),
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

		OOObject::int32_t Release_i(int id, bool external_call)
		{
			if (m_type==STUB && external_call)
			{
				return m_manager->ReleaseStub(m_key);
			}
			else if (--m_refcount == 0)
			{
				if (m_type==PROXY)
				{
					method(id,TypeInfo::async,DEFAULT_WAIT).send_and_recv();
					m_manager->ReleaseProxy(m_key);
				}
				delete this;
			}
			
			return 0;
		}

		OOObject::int32_t QueryInterface_i(int id, const OOObject::guid_t& iid, OOObject::Object** ppVal)
		{
			if (!ppVal)
			{
				errno = EINVAL;
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1);
			}

			OOObject::int32_t ret = 0;
			if (iid == OOObject::Object::IID)
			{
				AddRef();
				*ppVal = static_cast<OOCore::Stub*>(this);
			}
			else if (m_type==STUB)
			{
				if (iid == OOCore::Stub::IID)
				{
					AddRef();
					*ppVal = static_cast<OOCore::Stub*>(this);
				}
				else
					ret = m_object->QueryInterface(iid,ppVal);
			}
			else if (m_type==PROXY)
			{
				if (iid == OBJECT::IID)
				{
					AddRef();
					*ppVal = static_cast<OBJECT*>(this);
				}
				else if (iid == OOCore::Proxy::IID)
				{
					AddRef();
					*ppVal = static_cast<OOCore::Proxy*>(this);
				}

				Impl::object_t<OOObject::Object**> ppVal_stub(ppVal,iid);
				Impl::marshaller_t qi_mshl(method(id,TypeInfo::sync,DEFAULT_WAIT));
				qi_mshl << iid;
				ret = qi_mshl.send_and_recv();
				qi_mshl >> ppVal_stub;
			}
			else if (m_type==TYPEINFO)
			{
				if (iid == OOCore::TypeInfo::IID)
				{
					AddRef();
					*ppVal = static_cast<OOCore::TypeInfo*>(this);
				}
				else
				{
					errno = EINVAL;
					ret = -1;
				}
			}
			else
			{
				errno = EINVAL;
				ret = -1;
			}

			return ret;
		}

		int Invoke(TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, OOCore::InputStream* input, OOCore::OutputStream* output)
		{
			if (m_type!=STUB)
			{
				errno = EFAULT;
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Invalid call\n")),-1);
			}

			// Read the method number
			OOObject::uint32_t method;
			if (input->ReadULong(method) != 0)
				ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to read method ordinal\n")),-1);
			
			return Invoke_i(m_object,method,m_manager,OOCore::Impl::InputStream_Wrapper(input),OOCore::Impl::OutputStream_Wrapper(output));
		}
		
		int GetObject(OOObject::Object** ppVal)
		{
			if (m_type!=STUB)
			{
				errno = EFAULT;
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Invalid call\n")),-1);
			}
			else if (ppVal==0)
			{
				errno = EINVAL;
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Invalid NULL pointer\n")),-1);
			}

			*ppVal = m_object;
			if (*ppVal)
				(*ppVal)->AddRef();

			return 0;
		}

		int GetManager(ProxyStubManager** ppManager)
		{
			if (m_type!=PROXY)
			{
				errno = EFAULT;
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Invalid call\n")),-1);
			}
			else if (!ppManager)
			{
				errno = EINVAL;
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Invalid NULL pointer\n")),-1);
			}
			else
			{
				*ppManager = m_manager;
				(*ppManager)->AddRef();
				return 0;
			}
		}

		int GetKey(OOCore::ProxyStubManager::cookie_t* proxy_key)
		{
			if (m_type!=PROXY)
			{
				errno = EFAULT;
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Invalid call\n")),-1);
			}
			else if (!proxy_key)
			{
				errno = EINVAL;
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Invalid NULL pointer\n")),-1);
			}
			else
			{
				*proxy_key = m_key;
				return 0;
			}
		}

	protected:
		virtual ~ProxyStub_Impl() {};
		
		Impl::marshaller_t method(int id, TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs)
		{
			OOObject::uint32_t method = static_cast<OOObject::uint32_t>(id);
			OOObject::uint32_t trans_id;
			OOCore::Object_Ptr<OOCore::OutputStream> output;

			if (m_type!=PROXY || m_manager->CreateRequest(flags,m_key,&trans_id,&output) != 0)
				return Impl::marshaller_t();

			// Write the method number
			if (output->WriteULong(method) != 0)
			{
				m_manager->CancelRequest(trans_id);
				ACE_ERROR((LM_DEBUG,ACE_TEXT("(%P|%t) Failed to write method ordinal\n")));
				return Impl::marshaller_t();
			}
			
			return Impl::marshaller_t(m_manager,flags,wait_secs,output,trans_id);
		}
		
		virtual int Invoke_i(OBJECT* obj, OOObject::uint32_t& method, OOCore::Object_Ptr<OOCore::ProxyStubManager>& manager, OOCore::Impl::InputStream_Wrapper input, OOCore::Impl::OutputStream_Wrapper output) = 0;

		int unpack_iid_attr(size_t method, const void* data, const OOObject::guid_t& iid, const char* var_name, bool& is_index)
		{
			if (iid == OOObject::guid_t::NIL)
			{
				const OOObject::char_t* name;
				size_t param_count;
				OOCore::TypeInfo::Method_Attributes_t attributes;
				OOObject::uint16_t wait_secs;

				if (GetMethodInfo(method,&name,&param_count,&attributes,&wait_secs) != 0)
					return -1;

				for (size_t i=0;i<param_count;++i)
				{
					OOCore::TypeInfo::Type_t type;
					if (GetParamInfo(method,i,&name,&type) != 0)
						return -1;
					
					if (ACE_OS::strcmp(name,var_name)==0)
					{
						data = reinterpret_cast<const void*>(i);
						is_index = true;
						return 0;
					}
				}
			}

			// Must be a real iid!
			data = &iid;
			is_index = false;
			return 0;
		}

	private:
		const enum { PROXY, STUB, TYPEINFO } m_type;
		OOCore::ProxyStubManager::cookie_t m_key;
		OOCore::Object_Ptr<OBJECT> m_object;
		OOCore::Object_Ptr<OOCore::ProxyStubManager> m_manager;
		ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;
	};
};


#endif // OOCORE_PROXYSTUB_H_INCLUDED_
