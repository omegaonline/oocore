#ifndef OMEGA_TYPES_H_INCLUDED_
#define OMEGA_TYPES_H_INCLUDED_

#include <OOCore/OOCore_type_sizes.h>

namespace Omega
{
	typedef bool bool_t;
	typedef unsigned char byte_t;
	typedef char char_t;

#if defined(OMEGA_INT16_TYPE)
	typedef OMEGA_INT16_TYPE      int16_t;
#elif defined(OMEGA_HAS_INT16_T)
	typedef int16_t               int16_t;
#elif OMEGA_SIZEOF_SHORT == 2
	typedef short                 int16_t;
#elif OMEGA_SIZEOF_INT == 2
	typedef int                   int16_t;
#else
#error Have to add to the OMEGA_INT16 type setting
#endif  /* defined(OMEGA_INT16_TYPE) */

#if defined(OMEGA_UINT16_TYPE)
	typedef OMEGA_UINT16_TYPE     uint16_t;
#elif defined(OMEGA_HAS_UINT16_T)
	typedef uint16_t              uint16_t;
#elif OMEGA_SIZEOF_SHORT == 2
	typedef unsigned short        uint16_t;
#elif OMEGA_SIZEOF_INT == 2
	typedef unsigned int          uint16_t;
#else
#error Have to add to the OMEGA_UINT16 type setting
#endif /* defined(OMEGA_UINT16_TYPE) */

#if defined(OMEGA_INT32_TYPE)
	typedef OMEGA_INT32_TYPE      int32_t;
#elif defined(OMEGA_HAS_INT32_T)
	typedef int32_t               int32_t;
#elif OMEGA_SIZEOF_INT == 4
	typedef int                   int32_t;
#elif OMEGA_SIZEOF_LONG == 4
	typedef long                  int32_t;
#else
#error Have to add to the OMEGA_INT32 type setting
#endif /* defined(OMEGA_INT32_TYPE) */

#if defined(OMEGA_UINT32_TYPE)
	typedef OMEGA_UINT32_TYPE     uint32_t;
#elif defined(OMEGA_HAS_UINT32_T)
	typedef uint32_t              uint32_t;
#elif OMEGA_SIZEOF_INT == 4
	typedef unsigned int          uint32_t;
#elif OMEGA_SIZEOF_LONG == 4
	typedef unsigned long         uint32_t;
#else
#error Have to add to the OMEGA_UINT32 type setting
#endif /* defined(OMEGA_UINT32_TYPE) */

#if defined(OMEGA_INT64_TYPE)
	typedef OMEGA_INT64_TYPE      int64_t;
#elif defined(OMEGA_HAS_INT64_T)
	typedef int64_t               int64_t;
#elif OMEGA_SIZEOF_LONG == 8
	typedef long                  int64_t;
#elif OMEGA_SIZEOF_LONG_LONG == 8
	typedef long long             int64_t;
#else  /* no native 64 bit integer type */
#error Have to add to the OMEGA_INT64 type setting
#endif

#if defined(OMEGA_UINT64_TYPE)
	typedef OMEGA_UINT64_TYPE     uint64_t;
#elif defined(OMEGA_HAS_UINT64_T)
	typedef uint64_t              uint64_t;
#elif OMEGA_SIZEOF_LONG == 8
	typedef unsigned long         uint64_t;
#elif OMEGA_SIZEOF_LONG_LONG == 8
	typedef unsigned long long    uint64_t;
#else  /* no native 64 bit integer type */
	#error Have to add to the OMEGA_UINT64 type setting
#endif	

#if OMEGA_SIZEOF_FLOAT == 4
	typedef float real4_t;
#else  /* OMEGA_SIZEOF_FLOAT != 4 */
	#error Have to add to the OMEGA_FLOAT type setting
#endif /* OMEGA_SIZEOF_FLOAT != 4 */

#if OMEGA_SIZEOF_DOUBLE == 8
	typedef double real8_t;
#else  /* OMEGA_SIZEOF_DOUBLE != 8 */
	#error Have to add to the OMEGA_DOUBLE type setting
#endif /* OMEGA_SIZEOF_DOUBLE != 8 */

	class string_t
	{
	public:
		string_t();
		string_t(const string_t& s);
		string_t(const char_t* sz);
		~string_t();
		
		string_t& operator = (const string_t& s);
		string_t& operator = (const char_t* sz);
		
		operator const char_t*() const;
		bool operator == (const string_t& s) const;
		bool operator == (const char_t* sz) const;

		template <class T>
		bool operator != (T t) const
		{ return !((*this)==t); }

		string_t& operator += (const string_t& s);
		string_t& operator += (const char_t* sz);
		
		int CompareNoCase(const string_t& s) const;
		int CompareNoCase(const char_t* sz) const;
		bool IsEmpty() const;
		size_t Length() const;
		size_t Find(const string_t& str, size_t pos = 0, bool bIgnoreCase = false) const;
		size_t Find(char_t c, size_t pos = 0, bool bIgnoreCase = false) const;
		size_t ReverseFind(char_t c, ssize_t pos = npos, bool bIgnoreCase = false) const;
		string_t Left(size_t length) const;
		string_t Mid(size_t start, ssize_t length = -1) const;
		string_t& Clear();
		string_t ToLower() const;
		string_t ToUpper() const;

		static string_t Format(const char_t* pszFormat, ...);

		static const ssize_t npos = -1;
						
	private:
		typedef struct tag_handle_t
		{
			int unused;
		}* handle_t;

		explicit string_t(handle_t);
		handle_t m_handle;
	};
		
	struct guid_t
	{
		uint32_t	Data1;
		uint16_t	Data2;
		uint16_t	Data3;
		byte_t		Data4[8];

		bool operator==(const guid_t& rhs) const;
		bool operator!=(const guid_t& rhs) const;
		bool operator<(const guid_t& rhs) const;
		operator string_t() const;

		static guid_t FromString(const string_t& str);
		static const guid_t NIL;
	};

	class CriticalSection
	{
	public:
		CriticalSection();
		~CriticalSection();

		void Lock();
		void Unlock();

	private:
		typedef struct tag_handle_t
		{
			int unused;
		}* handle_t;
		
		handle_t m_handle;
	};

	template <class MUTEX>
	class Guard
	{
	public:
		Guard(MUTEX& lock) : m_cs(lock)
		{
			m_cs.Lock();
		}
		
		~Guard()
		{
			m_cs.Unlock();
		}

	private:
		// Copying is a really bad idea!
		Guard& operator = (const Guard& rhs)
		{ }
		
		MUTEX& m_cs;
	};

	template <class T, int S>
	class AtomicOpImpl
	{
	public:
		AtomicOpImpl() {};
		AtomicOpImpl(const AtomicOpImpl& rhs);
		AtomicOpImpl(const T& v);

		T operator ++();
		T operator ++(int);
		T operator --();
		T operator --(int);
		T* operator &();

		AtomicOpImpl& operator = (const AtomicOpImpl& rhs);
		AtomicOpImpl& operator = (const T& rhs);

		T value() const;
		T exchange(const T& v);
		
	private:
		T	m_value;
	};

#if defined(OMEGA_HAS_BUILTIN_ATOMIC_OP_4)
	template <class T>
	class AtomicOpImpl<T,4>
	{
	public:
		AtomicOpImpl() {};
		AtomicOpImpl(const AtomicOpImpl& rhs);
		AtomicOpImpl(const T& v);

		AtomicOpImpl& operator = (const AtomicOpImpl& rhs);
		AtomicOpImpl& operator = (const T& rhs);

		T operator ++();
		T operator ++(int);
		T operator --();
		T operator --(int);
		T* operator &();

		T value() const;
		T exchange(const T& v);

	private:
		T	m_value;
	};

#endif

#if defined(OMEGA_HAS_BUILTIN_ATOMIC_OP_8)
	template <class T>
	class AtomicOpImpl<T,8>
	{
	public:
		AtomicOpImpl() {};
		AtomicOpImpl(const AtomicOpImpl& rhs);
		AtomicOpImpl(const T& v);

		AtomicOpImpl& operator = (const AtomicOpImpl& rhs);
		AtomicOpImpl& operator = (const T& rhs);

		T operator ++();
		T operator ++(int);
		T operator --();
		T operator --(int);
		T* operator &();

		T value() const;
		T exchange(const T& v);

	private:
		T	m_value;
	};

#endif

	template <class T>
	struct AtomicOp
	{
		typedef AtomicOpImpl<T,sizeof(T)> type;
	};

	namespace MetaInfo
	{
		typedef bool yes_t;
		typedef bool (&no_t)[2];

		template <size_t N>
		struct size_t_
		{
			static const size_t		value = N;
			typedef size_t_<N>		type;
			typedef size_t_<N+1>	next;
		};

		template <class T>
		struct null_info
		{
			static const T value()
			{
				return T();
			}
		};

		template <class T>
		struct null_info<T*>
		{
			static T* value()
			{
				return 0;
			}
		};

		template <>
		struct null_info<Omega::uint32_t>
		{
			static const Omega::uint32_t value()
			{
				return 0;
			}
		};

		template <class T>
		void set_null(T& p)
		{
			p = null_info<T>::value();
		}

		template <class T>
		void set_null(T* p)
		{
			if (p)
				*p = null_info<T>::value();
		}
	}
}

#if defined(OMEGA_GUID_LINK_HERE)
	const Omega::guid_t Omega::guid_t::NIL = {0};
#endif

#endif // OMEGA_TYPES_H_INCLUDED_
