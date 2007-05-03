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
	using                       ::int16_t;
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
	using                       ::uint16_t;
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
	using                       ::int32_t;
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
	using                       ::uint32_t;
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
	using                       ::int64_t;
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
	using                       ::uint64_t;
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
		inline string_t();
		inline string_t(const string_t& s);
		inline string_t(const char_t* sz);
		inline ~string_t();

		inline string_t& operator = (const string_t& s);
		inline string_t& operator = (const char_t* sz);

		inline operator const char_t*() const;
		inline bool operator == (const string_t& s) const;
		inline bool operator == (const char_t* sz) const;

		template <class T>
		bool operator != (T t) const
		{ return !((*this)==t); }

		inline string_t& operator += (const string_t& s);
		inline string_t& operator += (const char_t* sz);

		inline int Compare(const string_t& s) const;
		inline int Compare(const char_t* sz) const;
		inline int CompareNoCase(const string_t& s) const;
		inline int CompareNoCase(const char_t* sz) const;
		inline bool IsEmpty() const;
		inline size_t Length() const;
		inline size_t Find(const string_t& str, size_t pos = 0, bool bIgnoreCase = false) const;
		inline size_t Find(char_t c, size_t pos = 0, bool bIgnoreCase = false) const;
		inline size_t ReverseFind(char_t c, size_t pos = npos, bool bIgnoreCase = false) const;
		inline string_t Left(size_t length) const;
		inline string_t Mid(size_t start, size_t length = npos) const;
		inline string_t Right(size_t length) const;
		inline string_t& Clear();
		inline string_t ToLower() const;
		inline string_t ToUpper() const;

		inline static string_t Format(const char_t* pszFormat, ...);

		static const size_t npos = size_t(-1);

	private:
		typedef struct tag_handle_t
		{
			int unused;
		}* handle_t;

		inline explicit string_t(handle_t);
		handle_t m_handle;
	};

	struct guid_t
	{
		uint32_t	Data1;
		uint16_t	Data2;
		uint16_t	Data3;
		byte_t		Data4[8];

		inline bool operator==(const guid_t& rhs) const;
		inline bool operator!=(const guid_t& rhs) const;
		inline bool operator<(const guid_t& rhs) const;
		inline operator string_t() const;

		inline static guid_t FromString(const string_t& str);
		static const guid_t NIL;
	};

	class CriticalSection
	{
	public:
		inline CriticalSection();
		inline ~CriticalSection();

		inline void Lock();
		inline void Unlock();

	private:
		typedef struct tag_handle_t
		{
			int unused;
		}* handle_t;

		handle_t m_handle;
	};

	template <class MUTEX> class Guard
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

	template <class T, int S> class AtomicOpImpl
	{
	public:
		AtomicOpImpl() {};
		inline AtomicOpImpl(const AtomicOpImpl& rhs);
		inline AtomicOpImpl(const T& v);

		inline T operator ++();
		inline T operator ++(int);
		inline T operator --();
		inline T operator --(int);
		inline T* operator &();

		inline AtomicOpImpl& operator = (const AtomicOpImpl& rhs);
		inline AtomicOpImpl& operator = (const T& rhs);

		inline T value() const;
		inline T& value();
		inline T exchange(const T& v);

	private:
		T	m_value;
	};

#if defined(OMEGA_HAS_BUILTIN_ATOMIC_OP_4)
	template <class T> class AtomicOpImpl<T,4>
	{
	public:
		AtomicOpImpl() {};
		inline AtomicOpImpl(const AtomicOpImpl& rhs);
		inline AtomicOpImpl(const T& v);

		inline AtomicOpImpl& operator = (const AtomicOpImpl& rhs);
		inline AtomicOpImpl& operator = (const T& rhs);

		inline T operator ++();
		inline T operator ++(int);
		inline T operator --();
		inline T operator --(int);
		inline T* operator &();

		inline T value() const;
		inline T& value();
		inline T exchange(const T& v);

	private:
		T	m_value;
	};

#endif

#if defined(OMEGA_HAS_BUILTIN_ATOMIC_OP_8)
	template <class T> class AtomicOpImpl<T,8>
	{
	public:
		AtomicOpImpl() {};
		inline AtomicOpImpl(const AtomicOpImpl& rhs);
		inline AtomicOpImpl(const T& v);

		inline AtomicOpImpl& operator = (const AtomicOpImpl& rhs);
		inline AtomicOpImpl& operator = (const T& rhs);

		inline T operator ++();
		inline T operator ++(int);
		inline T operator --();
		inline T operator --(int);
		inline T* operator &();

		inline T value() const;
		inline T& value();
		inline T exchange(const T& v);

	private:
		T	m_value;
	};

#endif

	template <class T> struct AtomicOp
	{
		typedef AtomicOpImpl<T,sizeof(T)> type;
	};

	namespace MetaInfo
	{
		typedef bool yes_t;
		typedef bool (&no_t)[2];

		template <size_t N> struct size_t_
		{
			static const size_t		value = N;
			typedef size_t_<N>		type;
			typedef size_t_<N+1>	next;
		};

		template <class T> struct default_value
		{
			static T value()
			{
				static T v;
				return v;
			}
		};

		template <class T> struct default_value<T&>
		{
			static T value()
			{
				return default_value<T>::value();
			}
		};

		// MSVC gets twitchy about size_t/uint32_t
		#if defined(_MSC_VER)
		template <> struct default_value<uint32_t>
		{
			static uint32_t value()
			{
				static uint32_t v;
				return v;
			}
		};
		#endif

		template <class T> struct remove_const
		{
			typedef T type;
		};

		template <class T> struct remove_const<const T>
		{
			typedef T type;
		};

		template <class T> struct remove_const<T&>
		{
			typedef typename remove_const<T>::type& type;
		};

		template <class T> struct remove_const<T*>
		{
			typedef typename remove_const<T>::type* type;
		};
	}
}

#if defined(OMEGA_GUID_LINK_HERE)
	const Omega::guid_t Omega::guid_t::NIL = {0,0,0,{0,0,0,0,0,0,0,0}};
#endif

#endif // OMEGA_TYPES_H_INCLUDED_
