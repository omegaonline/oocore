#ifndef OOCORE_PROXYSTUB_TYPES_H_INCLUDED_
#define OOCORE_PROXYSTUB_TYPES_H_INCLUDED_

#include <boost/type_traits.hpp>

#include "./OOCore_Impl.h"

namespace OOProxyStub
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
		  m_data(0), m_p(0), m_count(c)
		{ }

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
			m_data = static_cast<T*>(OOObject::Alloc(sizeof(T)*(*m_count)));
			if (!m_data) return -1;
				
			for (OOObject::uint32_t i=0;i<*m_count;++i)
			{
				if (in.read(m_data[i])!=0) return -1;
			}
			if (m_p) OOObject::Free(m_p);
			m_p = m_data;
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

		int read(OOCore::ProxyStubManager* manager, OOCore::Impl::InputStream_Wrapper& in)
		{
			OOObject::cookie_t key;
			if (in.read(key) != 0) 
				return -1;
		
			return manager->CreateProxy(m_iid,key,reinterpret_cast<OOObject::Object**>(&m_obj));
		}

		int write(OOCore::ProxyStubManager* manager, OOCore::OutputStream* out)
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

		int read(OOCore::ProxyStubManager* manager, OOCore::Impl::InputStream_Wrapper& in)
		{
			OOObject::cookie_t key;
			if (in.read(key) != 0) 
				return -1;
		
			return manager->CreateProxy(m_iid,key,&m_obj);
		}
		
		int write(OOCore::ProxyStubManager* manager, OOCore::OutputStream* out)
		{
			return manager->CreateStub(m_iid,m_obj,out);
		}
		
	private:
		const OOObject::guid_t m_iid;
		OOCore::Object_Ptr<T> m_obj;
	};

	template <class T>
	class stub_param_t
	{
	public:
		stub_param_t(OOCore::InputStream* input)
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
	class stub_param_t<T*>
	{
	public:
		stub_param_t() :
		  m_failed(false)
		{ }

		stub_param_t(OOCore::InputStream* input)
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
	class stub_param_t<array_t<T*> >
	{
	public:
		stub_param_t(OOCore::InputStream* input, const OOObject::uint32_t c) :
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
	class stub_param_t<array_t<T**> >
	{
	public:
		stub_param_t(stub_param_t<OOObject::uint32_t*>& c) :
		  m_arr(c), m_failed(false)
		{ }

		stub_param_t(OOCore::InputStream* input, stub_param_t<OOObject::uint32_t*>& c) :
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
	class stub_param_t<string_t<T> >
	{
	public:
		stub_param_t(OOCore::InputStream* input) :
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
	class stub_param_t<object_t<T*> >
	{
	public:
		stub_param_t(const OOObject::guid_t& iid) :
		  m_obj(iid), m_failed(false)
		{ }

		stub_param_t(OOCore::InputStream* input, OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid) :
		  m_obj(iid), m_failed(false)
		{
			m_failed = (m_obj.read(manager,OOCore::Impl::InputStream_Wrapper(input))!=0);
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
	class stub_param_t<object_t<T**> >
	{
	public:
		stub_param_t(const OOObject::guid_t& iid) :
		  m_obj(iid)
		{ }

		operator T**()
		{
			return m_obj;
		}

		int respond(OOCore::OutputStream* output, OOCore::ProxyStubManager* manager)
		{
			return m_obj.write(manager,output);
		}

	private:
		object_t<T**> m_obj;
	};

	class invoker_t
	{
	public:
		template <class T, class I>
		static int Invoke(T* pT, I* iface, OOCore::ProxyStubManager* manager, OOObject::uint32_t method, OOObject::int32_t& ret_code, OOCore::InputStream* input, OOCore::OutputStream* output)
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
		bool						m_failed;
		OOCore::Object_Ptr<OOCore::ProxyStubManager> m_manager;
		const OOObject::bool_t			m_sync;
		const OOObject::uint32_t		m_trans_id;
	};
};
};

#endif // OOCORE_PROXYSTUB_TYPES_H_INCLUDED_
