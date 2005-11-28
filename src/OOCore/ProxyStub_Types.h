#ifndef OOCORE_PROXYSTUB_TYPES_H_INCLUDED_
#define OOCORE_PROXYSTUB_TYPES_H_INCLUDED_

namespace OOCore
{
namespace Proxy_Stub
{
	struct used_t { char a[1]; };
	struct unused_t { char a[2]; };

	// The size_is attribute can only be applied 
	// to pointer types
	template <class T>
	class array_t;

	template <class T>
	class array_t<T**>
	{
	public:
        array_t(T** ar, OOObj::uint32_t* c);
	};

	template <class T>
	class array_t<T*>
	{
	public:
		array_t(T* ar, OOObj::uint32_t c);
	};

	// The string attribute can only be applied 
	// to const character pointer types
	template <class T>
	class string_t;

	template <class T>
	class string_t<const T*>
	{
	public:
		string_t(const T* s);
	};

	template <class T>
	class object_t;

	template <class T>
	class object_t<T*>
	{
	public:
		object_t(const OOObj::guid_t& iid);
		object_t(T* obj, const OOObj::guid_t& iid);
	};

	template <class T>
	class object_t<T**>
	{
	public:
		object_t(const OOObj::guid_t& iid);
		object_t(T** obj, const OOObj::guid_t& iid);
	};

	template <class T>
	class stub_param_t
	{
	public:
		stub_param_t(OOCore::InputStream* input);

		operator T();
		bool failed();
	};

	template <class T>
	class stub_param_t<T*>
	{
	public:
		stub_param_t();
		stub_param_t(OOCore::InputStream* input);

		operator T*();
		int respond(OOCore::OutputStream* output);
		bool failed();
	};

	template <class T>
	class stub_param_t<array_t<T*> >
	{
	public:
		stub_param_t(OOCore::InputStream* input, OOObj::uint32_t c);
		
		operator T*();
		bool failed();
	};

	template <class T>
	class stub_param_t<array_t<T**> >
	{
	public:
		stub_param_t(OOObj::uint32_t* c);
		stub_param_t(OOCore::InputStream* input, OOObj::uint32_t* c);

		operator T**();
		int respond(OOCore::OutputStream* output);
		bool failed();
	};

	template <class T>
	class stub_param_t<string_t<T> >
	{
	public:
		stub_param_t(OOCore::InputStream* input);

		operator T();
		bool failed();
	};

	template <class T>
	class stub_param_t<object_t<T> >
	{
	public:
		stub_param_t(const OOObj::guid_t& iid);
		stub_param_t(OOCore::InputStream* input, const OOObj::guid_t& iid);

		operator T();
		int respond(OOCore::OutputStream* output);
		bool failed();
	};

	class invoker_t
	{
	public:
		template <class T, class I>
		static int Invoke(T* pT, I* iface, OOObj::uint32_t method, OOObj::int32_t& ret_code, OOCore::InputStream* input, OOCore::OutputStream* output)
		{
			DECLARE_INVOKE_TABLE()
		}
	};

	class marshaller_t
	{
	public:
		template <class T>
		marshaller_t& operator <<(const T& val);

		template <class T>
		marshaller_t& operator <<(T* val);

		template <class T>
		marshaller_t& operator >>(T v);

		OOObj::int32_t send_and_recv();
	};
};
};

#endif // OOCORE_PROXYSTUB_TYPES_H_INCLUDED_
