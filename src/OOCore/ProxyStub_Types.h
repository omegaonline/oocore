#ifndef OOCORE_PROXYSTUB_TYPES_H_INCLUDED_
#define OOCORE_PROXYSTUB_TYPES_H_INCLUDED_

#include <boost/type_traits.hpp>

#include "./OOCore_Impl.h"

namespace OOCore
{
namespace Impl
{
	struct used_t { char a[1]; };
	struct unused_t { char a[2]; };
	
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

		int read(OOCore::Impl::InputStream_Wrapper& in)
		{
			m_data = static_cast<type>(OOObject::Alloc(m_count*sizeof(T)));
			if (!m_data) return -1;

			for (OOObject::uint32_t i=0;i<m_count;++i)
			{
				if (in.read(m_data[i])!=0) return -1;
			}
			m_p = m_data;
			return 0;
		}

		int write(OOCore::Impl::OutputStream_Wrapper& out)
		{
			for (OOObject::uint32_t i=0;i<m_count;++i)
			{
				if (out.write(m_p[i])!=0) return -1;
			}
			return 0;
		}

	private:
		typedef typename boost::remove_cv<T*>::type type;
		type m_data;
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
		
		int read(OOCore::Impl::InputStream_Wrapper& in)
		{
			if (m_count==0)
				return -1;

			if (!m_p || (*m_count)>m_orig_count)
			{
				m_data = static_cast<T*>(OOObject::Alloc(sizeof(T)*(*m_count)));
				if (m_p)
				{
					if (m_orig_count)
						OOObject::Free(*m_p);

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

		int write(OOCore::Impl::OutputStream_Wrapper& out)
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

	// The string attribute can only be applied 
	// to const character pointer types
	template <class T>
	class string_t;

	template <class T>
	class string_t<const T*>
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

		int read(OOCore::Impl::InputStream_Wrapper& in)
		{
			OOObject::uint32_t size;
			if (in.read(size)!=0) return -1;

			if (size > 0)
			{
				m_data = static_cast<T*>(OOObject::Alloc((size+1)*sizeof(T)));
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

		int write(OOCore::Impl::OutputStream_Wrapper& out)
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
		T* m_data;
		const T* m_p;
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

		int read(OOCore::ProxyStubManager* manager, OOCore::Impl::InputStream_Wrapper& in)
		{
			OOObject::bool_t null;
			if (in.read(null) != 0) 
				return -1;

			if (!null)
			{
				OOCore::ProxyStubManager::cookie_t key;
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

		int write(OOCore::ProxyStubManager* manager, OOCore::Impl::OutputStream_Wrapper& out)
		{
			if (!m_obj)
			{
				return out->WriteBoolean(true);
			}
			else
			{
				if (out->WriteBoolean(false)!=0)
					return -1;

				OOCore::ProxyStubManager::cookie_t key;
				if (manager->CreateStub(m_iid,m_obj,&key)!=0)
					return -1;

				return out.write(key);
			}
		}

	private:
		const OOObject::guid_t m_iid;
		OOCore::Object_Ptr<T> m_obj;
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

		int read(OOCore::ProxyStubManager* manager, OOCore::Impl::InputStream_Wrapper& in)
		{
			if (!m_ptr)
				return -1;

			OOObject::bool_t null;
			if (in.read(null) != 0) 
				return -1;

			if (!null)
			{
				OOCore::ProxyStubManager::cookie_t key;
				if (in.read(key) != 0) 
					return -1;

				if (*m_ptr)
					(*m_ptr)->Release();
							
				return manager->CreateProxy(m_iid,key,m_ptr);
			}
			else
			{
				*m_ptr = 0;
				return 0;
			}
		}
		
		int write(OOCore::ProxyStubManager* manager, OOCore::Impl::OutputStream_Wrapper& out)
		{
			if (*get_ptr() == 0)
			{
				return out->WriteBoolean(true);
			}
			else
			{
				if (out->WriteBoolean(false)!=0)
					return -1;

				OOCore::ProxyStubManager::cookie_t key;
				if (manager->CreateStub(m_iid,*get_ptr(),&key) != 0)
					return -1;

				return out.write(key);
			}
		}
		
	private:
		const OOObject::guid_t m_iid;
		OOCore::Object_Ptr<T> m_obj;
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

		param_t(OOCore::Impl::InputStream_Wrapper& input)
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

		param_t(OOCore::Impl::InputStream_Wrapper& input)
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

		int respond(OOCore::Impl::OutputStream_Wrapper& output)
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

		param_t(OOCore::Impl::InputStream_Wrapper& input, const OOObject::uint32_t c) :
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

		param_t(OOCore::Impl::InputStream_Wrapper& input, param_t<OOObject::uint32_t*>& c) :
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

		int respond(OOCore::Impl::OutputStream_Wrapper& output)
		{
			return m_arr.write(output);
		}

	private:
		array_t<T**> m_arr;
		bool m_failed;
	};

	template <class T>
	class param_t<string_t<T> >
	{
	public:
		param_t() : m_failed(true)
		{ }

		param_t(OOCore::Impl::InputStream_Wrapper& input)
		{
			m_failed = (m_str.read(input)!=0);
		}

		operator T()
		{
			return m_str;
		}
		
		bool failed()
		{
			return m_failed;
		}

	private:
		string_t<T> m_str;
		bool m_failed;
	};

	template <class T>
	class param_t<object_t<T*> >
	{
	public:
		param_t(const OOObject::guid_t& iid) :
		  m_obj(iid), m_failed(true)
		{ }

		param_t(OOCore::Impl::InputStream_Wrapper& input, OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid) :
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

		int respond(OOCore::Impl::OutputStream_Wrapper& output, OOCore::ProxyStubManager* manager)
		{
			return m_obj.write(manager,output);
		}

	private:
		object_t<T**> m_obj;
	};
};
};

#endif // OOCORE_PROXYSTUB_TYPES_H_INCLUDED_
