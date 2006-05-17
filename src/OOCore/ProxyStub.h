#ifndef OOCORE_PROXYSTUB_H_INCLUDED_
#define OOCORE_PROXYSTUB_H_INCLUDED_

#include "./OOCore.h"
#include "./OOUtil.h"
#include "./ProxyStub_Macros.h"

namespace OOUtil
{
	class ProxyStub_Base
	{
	protected:
		typedef char used_t;
		typedef char (&unused_t)[2];
		
		// The size_is attribute can only be applied 
		// to pointer types
		template <class T>
		class array_t;

		template <class T>
		class array_t<T*>
		{
		public:
			array_t(const OOObject::uint32_t c) :
			m_data(0), m_p(0), m_count(c)
			{ }
			
			array_t(T* ar, const OOObject::uint32_t c) :
			m_data(0), m_p(ar), m_count(c)
			{ }

			~array_t()
			{
				if (m_data) OOObject::Free(m_data);
			}

			operator T*()
			{
				return m_p;
			}

			int read(InputStream_Ptr& in)
			{
				m_data = static_cast<type*>(OOObject::Alloc(m_count*sizeof(T)));
				if (!m_data) return -1;

				for (OOObject::uint32_t i=0;i<m_count;++i)
				{
					if (in.read(m_data[i])!=0) return -1;
				}
				m_p = m_data;
				return 0;
			}

			int write(OutputStream_Ptr& out)
			{
				for (OOObject::uint32_t i=0;i<m_count;++i)
				{
					if (out.write(m_p[i])!=0) return -1;
				}
				return 0;
			}

		private:
			typedef typename boost::remove_cv<T>::type type;
			type* m_data;
			T* m_p;
			const OOObject::uint32_t m_count;
		};

		template <class T>
		class array_t<T**>
		{
		public:
			array_t(OOObject::uint32_t* c) :
			m_data(0), m_p(0), m_count(c), m_orig_count(0)
			{ }

			array_t(T** ar, OOObject::uint32_t* c)	: 
			m_data(0), m_p(ar), m_count(c), m_orig_count(0)
			{ }

			~array_t()
			{
				if (m_data && !m_p)
					OOObject::Free(m_data);
			}

			operator T**()
			{
				return get_ptr();
			}
			
			int read(InputStream_Ptr& in)
			{
				if (m_count==0)
					return -1;

				if (!m_p || (*m_count)>m_orig_count)
				{
					m_data = static_cast<T*>(OOObject::Alloc(sizeof(T)*(*m_count)));
					if (m_p)
					{
						//if (m_orig_count)
						//	OOObject::Free(*m_p);

						*m_p = m_data;
					}
				}

				if (*get_ptr()==0)
					return -1;
					
				for (OOObject::uint32_t i=0;i<*m_count;++i)
				{
					if (in.read((*get_ptr())[i])!=0) return -1;
				}
				return 0;
			}

			int write(OutputStream_Ptr& out)
			{
				if (*get_ptr()==0 || m_count==0)
					return -1;

				m_orig_count = *m_count;

				for (OOObject::uint32_t i=0;i<*m_count;++i)
				{
					if (out.write((*get_ptr())[i])!=0) return -1;
				}
				return 0;
			}

		private:
			T* m_data;
			T** m_p;
			OOObject::uint32_t* m_count;
			OOObject::uint32_t m_orig_count;
					
			T** get_ptr()
			{
				return (m_p ? m_p : &m_data);
			}
		};

		template <class T>
		class string_t;

		template <class T>
		class string_t<T*>
		{
		public:
			string_t() :
			m_data(0), m_p(0)
			{ }

			string_t(const T* s) :
			m_data(0), m_p(s)
			{ }

			~string_t()
			{
				if (m_data) OOObject::Free(m_data);
			}

			operator const T*()
			{
				return m_p;
			}

			int read(InputStream_Ptr& in)
			{
				OOObject::uint32_t size;
				if (in.read(size)!=0) return -1;

				if (size > 0)
				{
					m_data = static_cast<type*>(OOObject::Alloc((size+1)*sizeof(T)));
					if (!m_data) 
						return -1;
				
					for (OOObject::uint32_t i=0;i<size;++i)
					{
						if (in.read(m_data[i])!=0) return -1;
					}
					m_data[size] = static_cast<T>(0);
				}
				m_p = m_data;
				return 0;
			}

			int write(OutputStream_Ptr& out)
			{
				OOObject::uint32_t size = 0;
				if (m_p) 
				{
					size_t s = ACE_OS::strlen(m_p);
					if (s > 0xffffffff)
					{
						errno = E2BIG;
						return -1;
					}

					size = static_cast<OOObject::uint32_t>(s);
				}

				if (out.write(size)!=0) 
					return -1;

				for (OOObject::uint32_t i=0;i<size;++i)
				{
					if (out.write(m_p[i])!=0) return -1;
				}
				return 0;
			}

		private:
			typedef typename boost::remove_cv<T>::type type;
			type* m_data;
			T* m_p;
		};

		template <class T>
		class string_t<T**>
		{
		public:
			string_t() :
			m_data(0), m_p(0), m_orig_count(0)
			{ }

			string_t(T** s)	: 
			m_data(0), m_p(s), m_orig_count(0)
			{ }

			~string_t()
			{
				if (m_data && !m_p)
					OOObject::Free(m_data);
			}

			operator T**()
			{
				return get_ptr();
			}
			
			int read(InputStream_Ptr& in)
			{
				OOObject::uint32_t size;
				if (in.read(size)!=0) return -1;

				if (!m_p || size>m_orig_count)
				{
					m_data = static_cast<T*>(OOObject::Alloc((size+1)*sizeof(T)));
					if (m_p)
					{
						//if (m_orig_count)
						//	OOObject::Free(*m_p);

						*m_p = m_data;
					}
				}

				if (*get_ptr()==0)
					return -1;
					
				for (OOObject::uint32_t i=0;i<size;++i)
				{
					if (in.read((*get_ptr())[i])!=0) return -1;
				}
				(*get_ptr())[size] = static_cast<T>(0);
				return 0;
			}

			int write(OutputStream_Ptr& out)
			{
				if (*get_ptr())
				{
					size_t s = ACE_OS::strlen(*get_ptr());
					if (s > 0xffffffff)
					{
						errno = E2BIG;
						return -1;
					}

					m_orig_count = static_cast<OOObject::uint32_t>(s);
				}

				if (out.write(m_orig_count)!=0) 
					return -1;
				
				for (OOObject::uint32_t i=0;i<m_orig_count;++i)
				{
					if (out.write((*get_ptr())[i])!=0) return -1;
				}
				return 0;
			}

		private:
			T* m_data;
			T** m_p;
			OOObject::uint32_t m_orig_count;
					
			T** get_ptr()
			{
				return (m_p ? m_p : &m_data);
			}
		};

		template <class T>
		class object_t;

		template <class T>
		class object_t<T*>
		{
		public:
			object_t(const OOObject::guid_t& iid) :
			m_iid(iid)
			{ }

			object_t(T* obj, const OOObject::guid_t& iid) :
			m_iid(iid), m_obj(obj)
			{ }

			operator T*()
			{
				return m_obj;
			}

			int read(OOObject::ProxyStubManager* manager, InputStream_Ptr& in)
			{
				OOObject::bool_t null;
				if (in.read(null) != 0) 
					return -1;

				if (!null)
				{
					OOObject::uint32_t key;
					if (in.read(key) != 0) 
						return -1;
				
					return manager->CreateProxy(m_iid,key,reinterpret_cast<OOObject::Object**>(&m_obj));
				}
				else
				{
					m_obj = 0;
					return 0;
				}
			}

			int write(OOObject::ProxyStubManager* manager, OutputStream_Ptr& out)
			{
				if (!m_obj)
				{
					return out->WriteBoolean(true);
				}
				else
				{
					if (out->WriteBoolean(false)!=0)
						return -1;

					OOObject::uint32_t key;
					if (manager->CreateStub(m_iid,m_obj,&key)!=0)
						return -1;

					return out.write(key);
				}
			}

		private:
			const OOObject::guid_t m_iid;
			Object_Ptr<T> m_obj;
		};

		template <class T>
		class object_t<T**>
		{
		public:
			object_t(const OOObject::guid_t& iid) :
				m_iid(iid), m_ptr(0)
			{ }

			object_t(T** obj, const OOObject::guid_t& iid) :
				m_iid(iid), m_obj(*obj), m_ptr(obj)
			{ }
			
			operator T**()
			{
				return get_ptr();
			}

			int read(OOObject::ProxyStubManager* manager, InputStream_Ptr& in)
			{
				if (!m_ptr)
					return -1;

				OOObject::bool_t null;
				if (in.read(null) != 0) 
					return -1;

				if (!null)
				{
					OOObject::uint32_t key;
					if (in.read(key) != 0) 
						return -1;

					if (*m_ptr)
						(*m_ptr)->Release();
								
					if (manager->CreateProxy(m_iid,key,m_ptr) != 0)
						return -1;

					return 0;
				}
				else
				{
					*m_ptr = 0;
					return 0;
				}
			}
			
			int write(OOObject::ProxyStubManager* manager, OutputStream_Ptr& out)
			{
				if (*get_ptr() == 0)
				{
					return out->WriteBoolean(true);
				}
				else
				{
					if (out->WriteBoolean(false)!=0)
						return -1;

					OOObject::uint32_t key;
					if (manager->CreateStub(m_iid,*get_ptr(),&key) != 0)
						return -1;

					return out.write(key);
				}
			}
			
		private:
			const OOObject::guid_t m_iid;
			Object_Ptr<T> m_obj;
			T** m_ptr;

			T** get_ptr()
			{
				return (m_ptr ? m_ptr : &m_obj);
			}
		};

		template <class T>
		class param_t
		{
		public:
			param_t() : m_failed(true)
			{ }

			param_t(InputStream_Ptr& input)
			{
				m_failed = (input.read(m_t)!=0);
			}

			operator T()
			{
				return m_t;
			}

			bool failed()
			{
				return m_failed;
			}

		private:
			typedef typename boost::remove_reference<T>::type non_ref_type;
			typedef typename boost::remove_cv<non_ref_type>::type type;
							
			type m_t;
			bool m_failed;
		};

		template <class T>
		class param_t<T*>
		{
		public:
			param_t() : m_failed(true)
			{ }

			param_t(InputStream_Ptr& input)
			{
				m_failed = (input.read(m_t)!=0);
			}

			operator T*()
			{
				return &m_t;
			}

			bool failed()
			{
				return m_failed;
			}

			int respond(OutputStream_Ptr& output)
			{
				return output.write(m_t);
			}
			
		private:
			T m_t;
			bool m_failed;
		};

		template <class T>
		class param_t<array_t<T*> >
		{
		public:
			param_t(const OOObject::uint32_t c) :
			m_arr(c), m_failed(true)
			{ }

			param_t(InputStream_Ptr& input, const OOObject::uint32_t c) :
			m_arr(c)
			{
				m_failed = (m_arr.read(input)!=0);
			}
			
			operator T*()
			{
				return m_arr;
			}

			bool failed()
			{
				return m_failed;
			}

		private:
			array_t<T*> m_arr;
			bool m_failed;
		};

		template <class T>
		class param_t<array_t<T**> >
		{
		public:
			param_t(param_t<OOObject::uint32_t*>& c) :
			m_arr(c), m_failed(true)
			{ }

			param_t(InputStream_Ptr& input, param_t<OOObject::uint32_t*>& c) :
			m_arr(c)
			{
				m_failed = (m_arr.read(input)!=0);
			}		

			operator T**()
			{
				return m_arr;
			}

			bool failed()
			{
				return m_failed;
			}

			int respond(OutputStream_Ptr& output)
			{
				return m_arr.write(output);
			}

		private:
			array_t<T**> m_arr;
			bool m_failed;
		};

		template <class T>
		class param_t<string_t<T*> >
		{
		public:
			param_t() : m_failed(true)
			{ }

			param_t(InputStream_Ptr& input)
			{
				m_failed = (m_str.read(input)!=0);
			}

			operator T*()
			{
				return m_str;
			}
			
			bool failed()
			{
				return m_failed;
			}

		private:
			string_t<T*> m_str;
			bool m_failed;
		};

		template <class T>
		class param_t<string_t<T**> >
		{
		public:
			param_t() :
			m_failed(true)
			{ }

			param_t(InputStream_Ptr& input)
			{
				m_failed = (m_str.read(input)!=0);
			}		

			operator T**()
			{
				return m_str;
			}

			bool failed()
			{
				return m_failed;
			}

			int respond(OutputStream_Ptr& output)
			{
				return m_str.write(output);
			}

		private:
			string_t<T**> m_str;
			bool m_failed;
		};

		template <class T>
		class param_t<object_t<T*> >
		{
		public:
			param_t(const OOObject::guid_t& iid) :
			m_obj(iid), m_failed(true)
			{ }

			param_t(InputStream_Ptr& input, OOObject::ProxyStubManager* manager, const OOObject::guid_t& iid) :
			m_obj(iid)
			{
				m_failed = (m_obj.read(manager,input)!=0);
			}

			operator T*()
			{
				return m_obj;
			}

			bool failed()
			{
				return m_failed;
			}

		private:
			object_t<T*> m_obj;
			bool m_failed;
		};

		template <class T>
		class param_t<object_t<T**> >
		{
		public:
			param_t(const OOObject::guid_t& iid) :
			m_obj(iid)
			{ }

			operator T**()
			{
				return m_obj;
			}

			int respond(OutputStream_Ptr& output, OOObject::ProxyStubManager* manager)
			{
				return m_obj.write(manager,output);
			}

		private:
			object_t<T**> m_obj;
		};

		class metainfo_t
		{
		public:
			template <class T, class I>
				static int Invoke(T* pT, I* iface, Object_Ptr<OOObject::ProxyStubManager>& manager, OOObject::uint32_t method, InputStream_Ptr& input, OutputStream_Ptr& output)
			{
				OOCORE_PS_DECLARE_INVOKE_TABLE()
			}

			template <class T>
				static int GetMethodInfo(T* pT, size_t method, const OOObject::char_t** method_name, size_t* param_count, OOObject::TypeInfo::Method_Attributes_t* attributes, OOObject::uint16_t* wait_secs)
			{
				if (method_name==0 || param_count==0 || attributes==0 || wait_secs==0) 
				{
					errno = EINVAL; 
					ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid NULL value\n")),-1); 
				}

				OOCORE_PS_DECLARE_METHODINFO_TABLE()
			}

			template <class T>
				static int GetParamInfo(T* pT, size_t method, size_t param, const OOObject::char_t** param_name, OOObject::TypeInfo::Type_t* type)
			{
				if (param_name==0 || type==0) 
				{ 
					errno = EINVAL; 
					ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid NULL value\n")),-1); 
				}

				OOCORE_PS_DECLARE_PARAMINFO_TABLE()
			}

			template <class T>
				static int GetParamAttributeData(T* pT, size_t method, size_t param, OOObject::TypeInfo::Param_Attrib_Data_t* data)
			{
				if (data==0) 
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
			enum { value = ( type_info_t<T>::value | OOObject::TypeInfo::cpp_const) };
		};

		template <class T> class type_info_t<T&>
		{
		public:
			enum { value = ( type_info_t<T>::value | OOObject::TypeInfo::cpp_ref) };
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
			marshaller_t(OOObject::ProxyStubManager* manager, OOObject::TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, OOObject::OutputStream* output, OOObject::uint32_t trans_id);

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
			marshaller_t& operator >>(string_t<T**>& val)
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
			InputStream_Ptr									m_in;
			OutputStream_Ptr								m_out;
			bool											m_failed;
			Object_Ptr<OOObject::ProxyStubManager>			m_manager;
			const OOObject::TypeInfo::Method_Attributes_t	m_flags;
			const OOObject::uint32_t						m_trans_id;
			const OOObject::uint16_t						m_wait_secs;
		};
	};

	template <class OBJECT>
	class ProxyStub_Impl : 
		public ProxyStub_Base,
		public OBJECT,
		public OOObject::Proxy,
		public OOObject::Stub,
		public OOObject::TypeInfo
	{
	public:
		// TypeInfo constructor
		ProxyStub_Impl() :
			m_type(TYPEINFO),
			m_manager(0),
			m_refcount(0)
		{}

		// Proxy constructor
		ProxyStub_Impl(OOObject::ProxyStubManager* manager, const OOObject::uint32_t& key) :
			m_type(PROXY),
			m_key(key),
			m_manager(manager),
			m_refcount(0)
		{}

		// Stub constructor
		ProxyStub_Impl(OOObject::ProxyStubManager* manager, const OOObject::uint32_t& key, OBJECT* obj) :
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
					method(id,TypeInfo::async_method,DEFAULT_WAIT).send_and_recv();
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
				*ppVal = static_cast<OOObject::Stub*>(this);
			}
			else if (m_type==STUB)
			{
				if (iid == OOObject::Stub::IID)
				{
					AddRef();
					*ppVal = static_cast<OOObject::Stub*>(this);
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
				else if (iid == OOObject::Proxy::IID)
				{
					AddRef();
					*ppVal = static_cast<OOObject::Proxy*>(this);
				}
				else
				{
					object_t<OOObject::Object**> ppVal_stub(ppVal,iid);
					marshaller_t qi_mshl(method(id,TypeInfo::sync_method,DEFAULT_WAIT));
					qi_mshl << iid;
					ret = qi_mshl.send_and_recv();
					qi_mshl >> ppVal_stub;
				}
			}
			else if (m_type==TYPEINFO)
			{
				if (iid == OOObject::TypeInfo::IID)
				{
					AddRef();
					*ppVal = static_cast<OOObject::TypeInfo*>(this);
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

		int Invoke(OOObject::uint32_t method, TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs, OOObject::InputStream* input, OOObject::OutputStream* output)
		{
			if (m_type!=STUB)
			{
				errno = EFAULT;
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Invalid call\n")),-1);
			}

			return Invoke_i(m_object,method,m_manager,InputStream_Ptr(input),OutputStream_Ptr(output));
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

		int GetManager(OOObject::ProxyStubManager** ppManager)
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

		int GetKey(OOObject::uint32_t* proxy_key)
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
		
		virtual int Invoke_i(OBJECT* obj, OOObject::uint32_t& method, Object_Ptr<OOObject::ProxyStubManager>& manager, InputStream_Ptr input, OutputStream_Ptr output) = 0;

		marshaller_t method(int id, OOObject::TypeInfo::Method_Attributes_t flags, OOObject::uint16_t wait_secs)
		{
			OOObject::uint32_t method = static_cast<OOObject::uint32_t>(id);
			OOObject::uint32_t trans_id;
			OOUtil::OutputStream_Ptr output;

			if (m_type!=PROXY || m_manager->CreateRequest(method,flags,m_key,&trans_id,&output) != 0)
				return marshaller_t();

			return marshaller_t(m_manager,flags,wait_secs,output,trans_id);
		}

		int unpack_iid_attr(size_t method, OOObject::TypeInfo::Param_Attrib_Data_t* data, const OOObject::guid_t& iid, const char* var_name)
		{
			if (iid == OOObject::guid_t::NIL)
			{
				const OOObject::char_t* name;
				size_t param_count;
				OOObject::TypeInfo::Method_Attributes_t attributes;
				OOObject::uint16_t wait_secs;

				if (GetMethodInfo(method,&name,&param_count,&attributes,&wait_secs) != 0)
					return -1;

				for (size_t i=0;i<param_count;++i)
				{
					OOObject::TypeInfo::Type_t type;
					if (GetParamInfo(method,i,&name,&type) != 0)
						return -1;
					
					if (ACE_OS::strcmp(name,var_name)==0)
					{
						data->index = i;
						data->is_index = true;
						return 0;
					}
				}
			}

			// Must be a real iid!
			data->iid = &iid;
			data->is_index = false;
			return 0;
		}

	private:
		const enum { PROXY, STUB, TYPEINFO }	m_type;
		OOObject::uint32_t						m_key;
		Object_Ptr<OBJECT>						m_object;
		Object_Ptr<OOObject::ProxyStubManager>	m_manager;
		ACE_Atomic_Op<ACE_Thread_Mutex,long>	m_refcount;
	};
};


#endif // OOCORE_PROXYSTUB_H_INCLUDED_
