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
		  m_count(c)
		{
			m_data = static_cast<T*>(OOObject::Alloc(sizeof(T)*(*m_count)));
			m_p=m_data;
		}

        array_t(T** ar, OOObject::uint32_t* c)	: 
		  m_data(0), m_p(*ar), m_count(c)
		{ }

		~array_t()
		{
			if (m_p!=m_data) OOObject::Free(m_p);

			if (m_data!=0) OOObject::Free(m_data);
		}

		operator T**()
		{
			return &m_p;
		}
		
		int read(OOCore::Impl::InputStream_Wrapper& in)
		{
			if (!m_p) return -1;
				
			for (OOObject::uint32_t i=0;i<*m_count;++i)
			{
				if (in.read(m_p[i])!=0) return -1;
			}
			return 0;
		}

		int write(OOCore::Impl::OutputStream_Wrapper& out)
		{
			for (OOObject::uint32_t i=0;i<*m_count;++i)
			{
				if (out.write(m_p[i])!=0) return -1;
			}
			return 0;
		}

	private:
		T* m_data;
		T* m_p;
		OOObject::uint32_t* m_count;
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

			m_data = static_cast<T*>(OOObject::Alloc((size+1)*sizeof(T)));
			if (!m_data) return -1;

			for (OOObject::uint32_t i=0;i<size;++i)
			{
				if (in.read(m_data[i])!=0) return -1;
			}
			m_data[size] = static_cast<T>(0);
			m_p = m_data;
			return 0;
		}

		int write(OOCore::Impl::OutputStream_Wrapper& out)
		{
			OOObject::uint32_t size = ACE_OS::strlen(m_p);
			if (out.write(size)!=0) return -1;

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

		int read_ps(OOCore::ProxyStubManager* manager, OOCore::Impl::InputStream_Wrapper& in)
		{
			OOObject::cookie_t key;
			if (in.read(key) != 0) 
				return -1;
		
			return manager->CreateProxy(m_iid,key,reinterpret_cast<OOObject::Object**>(&m_obj));
		}

		int write_ps(OOCore::ProxyStubManager* manager, OOCore::OutputStream* out)
		{
			return manager->CreateStub(m_iid,m_obj,out);
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
		  m_iid(iid)
		{ }

		object_t(T** obj, const OOObject::guid_t& iid) :
		  m_iid(iid), m_obj(*obj)
		{ }
		
		operator T**()
		{
			return &m_obj;
		}

		int read_ps(OOCore::ProxyStubManager* manager, OOCore::Impl::InputStream_Wrapper& in)
		{
			OOObject::cookie_t key;
			if (in.read(key) != 0) 
				return -1;
		
			return manager->CreateProxy(m_iid,key,&m_obj);
		}
		
		int write_ps(OOCore::ProxyStubManager* manager, OOCore::OutputStream* out)
		{
			return manager->CreateStub(m_iid,m_obj,out);
		}
		
	private:
		const OOObject::guid_t m_iid;
		OOCore::Object_Ptr<T> m_obj;
	};

	template <class T>
	class param_t
	{
	public:
		param_t(OOCore::InputStream* input)
		{
			m_failed = (OOCore::Impl::InputStream_Wrapper(input).read(m_t)!=0);
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
		param_t() :
		  m_failed(false)
		{ }

		param_t(OOCore::InputStream* input)
		{
			m_failed = (OOCore::Impl::InputStream_Wrapper(input).read(m_t)!=0);
		}

		operator T*()
		{
			return &m_t;
		}

		bool failed()
		{
			return m_failed;
		}

		int respond(OOCore::OutputStream* output)
		{
			return OOCore::Impl::OutputStream_Wrapper(output).write(m_t);
		}
		
	private:
		T m_t;
		bool m_failed;
	};

	template <class T>
	class param_t<array_t<T*> >
	{
	public:
		param_t(OOCore::InputStream* input, const OOObject::uint32_t c) :
		  m_arr(c), m_failed(false)
		{
			m_failed = (m_arr.read(OOCore::Impl::InputStream_Wrapper(input))!=0);
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
		  m_arr(c), m_failed(false)
		{ }

		param_t(OOCore::InputStream* input, param_t<OOObject::uint32_t*>& c) :
		  m_arr(c), m_failed(false)
		{
			m_failed = (m_arr.read(OOCore::Impl::InputStream_Wrapper(input))!=0);
		}		

		operator T**()
		{
			return m_arr;
		}

		bool failed()
		{
			return m_failed;
		}

		int respond(OOCore::OutputStream* output)
		{
			return m_arr.write(OOCore::Impl::OutputStream_Wrapper(output));
		}

	private:
		array_t<T**> m_arr;
		bool m_failed;
	};

	template <class T>
	class param_t<string_t<T> >
	{
	public:
		param_t(OOCore::InputStream* input) :
		  m_failed(false)
		{
			m_failed = (m_str.read(OOCore::Impl::InputStream_Wrapper(input))!=0);
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
		  m_obj(iid), m_failed(false)
		{ }

		param_t(OOCore::InputStream* input, OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid) :
		  m_obj(iid), m_failed(false)
		{
			m_failed = (m_obj.read_ps(manager,OOCore::Impl::InputStream_Wrapper(input))!=0);
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

		int respond(OOCore::OutputStream* output, OOCore::ProxyStubManager* manager)
		{
			return m_obj.write_ps(manager,output);
		}

	private:
		object_t<T**> m_obj;
	};
};
};

#endif // OOCORE_PROXYSTUB_TYPES_H_INCLUDED_
