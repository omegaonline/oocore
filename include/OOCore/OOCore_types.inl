#ifndef OOCORE_TYPES_INL_INCLUDED_
#define OOCORE_TYPES_INL_INCLUDED_

// In order to 'export' a class from a DLL in an ABI agnostic way
// we export a whole set of extern "C" functions and call them in
// the member functions of the class.  Horrible I know!

Omega::string_t::string_t(handle_t h) : m_handle(h)
{
}

OOCORE_EXPORTED_FUNCTION(void*,string_t__ctor1,0,());
Omega::string_t::string_t()
{
	m_handle = static_cast<handle_t>(string_t__ctor1());
}

OOCORE_EXPORTED_FUNCTION(void*,string_t__ctor2,1,((in),const Omega::char_t*,sz));
Omega::string_t::string_t(const char_t* sz)
{
	m_handle = static_cast<handle_t>(string_t__ctor2(sz));
}

OOCORE_EXPORTED_FUNCTION(void*,string_t__ctor3,1,((in),const void*,s1));
Omega::string_t::string_t(const Omega::string_t& s)
{
	m_handle = static_cast<handle_t>(string_t__ctor3(s.m_handle));
}

OOCORE_EXPORTED_FUNCTION_VOID(string_t__dctor,1,((in),void*,h));
Omega::string_t::~string_t()
{
	string_t__dctor(m_handle);
}

OOCORE_EXPORTED_FUNCTION(void*,string_t_assign_1,2,((in),void*,h1,(in),const void*,h2));
Omega::string_t& Omega::string_t::operator = (const string_t& s)
{
	if (this != &s)
		m_handle = static_cast<handle_t>(string_t_assign_1(m_handle,s.m_handle));
	return *this;
}

OOCORE_EXPORTED_FUNCTION(void*,string_t_assign_2,2,((in),void*,h,(in),const Omega::char_t*,sz));
Omega::string_t& Omega::string_t::operator = (const char_t* sz)
{
	m_handle = static_cast<handle_t>(string_t_assign_2(m_handle,sz));
	return *this;
}

OOCORE_EXPORTED_FUNCTION(const Omega::char_t*,string_t_cast,1,((in),const void*,h));
Omega::string_t::operator const Omega::char_t*() const
{
	return string_t_cast(m_handle);
}

OOCORE_EXPORTED_FUNCTION(bool,string_t_eq1,2,((in),const void*,h1,(in),const void*,h2));
bool Omega::string_t::operator == (const string_t& s) const
{
	return string_t_eq1(m_handle,s.m_handle);
}

OOCORE_EXPORTED_FUNCTION(bool,string_t_eq2,2,((in),const void*,h,(in),const Omega::char_t*,sz));
bool Omega::string_t::operator == (const char_t* sz) const
{
	return string_t_eq2(m_handle,sz);
}

OOCORE_EXPORTED_FUNCTION(void*,string_t_add1,2,((in),void*,h,(in),const void*,h2));
Omega::string_t& Omega::string_t::operator += (const string_t& s)
{
	m_handle = static_cast<handle_t>(string_t_add1(m_handle,s.m_handle));
	return *this;
}

OOCORE_EXPORTED_FUNCTION(void*,string_t_add2,2,((in),void*,h,(in),const Omega::char_t*,sz));
Omega::string_t& Omega::string_t::operator += (const char_t* sz)
{
	m_handle = static_cast<handle_t>(string_t_add2(m_handle,sz));
	return *this;
}

OOCORE_EXPORTED_FUNCTION(int,string_t_cmp1,2,((in),const void*,h1,(in),const void*,h2));
int Omega::string_t::Compare(const string_t& s) const
{
	return string_t_cmp1(m_handle,s.m_handle);
}

OOCORE_EXPORTED_FUNCTION(int,string_t_cmp2,2,((in),const void*,h1,(in),const Omega::char_t*,sz));
int Omega::string_t::Compare(const char_t* sz) const
{
	return string_t_cmp2(m_handle,sz);
}

OOCORE_EXPORTED_FUNCTION(int,string_t_cnc1,2,((in),const void*,h1,(in),const void*,h2));
int Omega::string_t::CompareNoCase(const string_t& s) const
{
	return string_t_cnc1(m_handle,s.m_handle);
}

OOCORE_EXPORTED_FUNCTION(int,string_t_cnc2,2,((in),const void*,h1,(in),const Omega::char_t*,sz));
int Omega::string_t::CompareNoCase(const char_t* sz) const
{
	return string_t_cnc2(m_handle,sz);
}

OOCORE_EXPORTED_FUNCTION(bool,string_t_isempty,1,((in),const void*,h));
bool Omega::string_t::IsEmpty() const
{
	return string_t_isempty(m_handle);
}

OOCORE_EXPORTED_FUNCTION(size_t,string_t_len,1,((in),const void*,h));
size_t Omega::string_t::Length() const
{
	return string_t_len(m_handle);
}

OOCORE_EXPORTED_FUNCTION(size_t,string_t_find1,3,((in),const void*,h1,(in),const void*,h2,(in),size_t,s));
size_t Omega::string_t::Find(const string_t& str, size_t pos, bool bIgnoreCase) const
{
	if (!bIgnoreCase)
		return string_t_find1(m_handle,str.m_handle,pos);
	else
        return this->ToLower().Find(str.ToLower(),pos,false);
}

OOCORE_EXPORTED_FUNCTION(size_t,string_t_find2,4,((in),const void*,a,(in),Omega::char_t,b,(in),size_t,c,(in),bool,d));
size_t Omega::string_t::Find(char_t c, size_t pos, bool bIgnoreCase) const
{
	if (!bIgnoreCase)
		return string_t_find2(m_handle,c,pos,false);
	else
		return string_t_find2(this->ToLower().m_handle,c,pos,true);
}

OOCORE_EXPORTED_FUNCTION(size_t,string_t_rfind,4,((in),const void*,a,(in),Omega::char_t,b,(in),size_t,c,(in),bool,d));
size_t Omega::string_t::ReverseFind(char_t c, size_t pos, bool bIgnoreCase) const
{
	if (!bIgnoreCase)
		return string_t_rfind(m_handle,c,pos,false);
	else
		return string_t_rfind(this->ToLower().m_handle,c,pos,true);
}

OOCORE_EXPORTED_FUNCTION(void*,string_t_left,2,((in),const void*,a,(in),size_t,b));
Omega::string_t Omega::string_t::Left(size_t length) const
{
	return string_t(static_cast<handle_t>(string_t_left(m_handle,length)));
}

OOCORE_EXPORTED_FUNCTION(void*,string_t_mid,3,((in),const void*,h,(in),size_t,a,(in),size_t,b));
Omega::string_t Omega::string_t::Mid(size_t start, size_t length) const
{
	return string_t(static_cast<handle_t>(string_t_mid(m_handle,start,length)));
}

OOCORE_EXPORTED_FUNCTION(void*,string_t_right,2,((in),const void*,a,(in),size_t,b));
Omega::string_t Omega::string_t::Right(size_t length) const
{
	return string_t(static_cast<handle_t>(string_t_right(m_handle,length)));
}

OOCORE_EXPORTED_FUNCTION_VOID(string_t_clear,1,((in),void*,h));
Omega::string_t& Omega::string_t::Clear()
{
	string_t_clear(m_handle);
	return *this;
}

OOCORE_EXPORTED_FUNCTION(void*,string_t_tolower,1,((in),const void*,h));
Omega::string_t Omega::string_t::ToLower() const
{
	return string_t(static_cast<handle_t>(string_t_tolower(m_handle)));
}

OOCORE_EXPORTED_FUNCTION(void*,string_t_toupper,1,((in),const void*,h));
Omega::string_t Omega::string_t::ToUpper() const
{
	return string_t(static_cast<handle_t>(string_t_toupper(m_handle)));
}

OOCORE_EXPORTED_FUNCTION(void*,string_t_format,2,((in),const Omega::char_t*,s,(in),va_list,a));
Omega::string_t Omega::string_t::Format(const char_t* pszFormat, ...)
{
	va_list list;
	va_start(list,pszFormat);

	handle_t h = static_cast<handle_t>(string_t_format(pszFormat,list));

	va_end(list);

	return string_t(h);
}

inline Omega::string_t operator + (const Omega::string_t& lhs, const Omega::string_t& rhs)
{
	return (Omega::string_t(lhs) += rhs);
}

inline Omega::string_t operator + (const Omega::string_t& lhs, const Omega::char_t* rhs)
{
	return (Omega::string_t(lhs) += rhs);
}

inline Omega::string_t operator + (const Omega::char_t* lhs, const Omega::string_t& rhs)
{
	return (Omega::string_t(lhs) += rhs);
}

OOCORE_EXPORTED_FUNCTION(bool,guid_t_eq,2,((in),const Omega::guid_t&,a,(in),const Omega::guid_t&,b));
bool Omega::guid_t::operator==(const guid_t& rhs) const
{
	return guid_t_eq(*this,rhs);
}

bool Omega::guid_t::operator!=(const guid_t& rhs) const
{
	return !guid_t_eq(*this,rhs);
}

OOCORE_EXPORTED_FUNCTION(bool,guid_t_less,2,((in),const Omega::guid_t&,a,(in),const Omega::guid_t&,b));
bool Omega::guid_t::operator<(const guid_t& rhs) const
{
	return guid_t_less(*this,rhs);
}

Omega::guid_t::operator Omega::string_t() const
{
	return string_t::Format("{%8.8X-%4.4X-%4.4X-%2.2X%2.2X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X}",
        Data1,
        Data2,
        Data3,
        Data4[0],
        Data4[1],
		Data4[2],
		Data4[3],
		Data4[4],
		Data4[5],
		Data4[6],
		Data4[7]);
}

OOCORE_EXPORTED_FUNCTION(Omega::guid_t,guid_t_from_string,1,((in),const Omega::char_t*,s));
Omega::guid_t Omega::guid_t::FromString(const string_t& str)
{
	return guid_t_from_string(str);
}

#if (defined OMEGA_HAS_BUILTIN_ATOMIC_OP_4)

template <class T>
Omega::AtomicOpImpl<T,4>::AtomicOpImpl(const AtomicOpImpl& rhs) :
	m_value(rhs.m_value)
{ }

template <class T>
Omega::AtomicOpImpl<T,4>::AtomicOpImpl(const T& v) :
	m_value(v)
{ }

template <class T>
T Omega::AtomicOpImpl<T,4>::operator ++()
{
	// Prefix
#if defined(OMEGA_WIN32)
	return (T)(static_cast<LONG_PTR>(InterlockedIncrement(reinterpret_cast<LONG_PTR*>(&this->m_value))));
#else
#error  Use funky asm function!
#endif
}

template <class T>
T Omega::AtomicOpImpl<T,4>::operator ++(int)
{
	return ++*this - 1;
}

template <class T>
T Omega::AtomicOpImpl<T,4>::operator --()
{
	// Prefix
#if defined(OMEGA_WIN32)
	return (T)(static_cast<LONG_PTR>(InterlockedDecrement(reinterpret_cast<LONG_PTR*>(&this->m_value))));
#else
#error  Use funky asm function!
#endif
}

template <class T>
T Omega::AtomicOpImpl<T,4>::operator --(int)
{
	return --*this + 1;
}

template <class T>
T* Omega::AtomicOpImpl<T,4>::operator &()
{
	return &m_value;
}

template <class T>
T Omega::AtomicOpImpl<T,4>::exchange(const T& v)
{
#if defined(OMEGA_WIN32)
	return (T)(static_cast<LONG_PTR>(InterlockedExchange(reinterpret_cast<LONG_PTR*>(&this->m_value),static_cast<const LONG>((LONG_PTR)(v)))));
#else
#error  Use funky asm function!
#endif
}

template <class T>
Omega::AtomicOpImpl<T,4>& Omega::AtomicOpImpl<T,4>::operator = (const AtomicOpImpl& rhs)
{
	exchange(rhs.m_value);
	return (*this);
}

template <class T>
Omega::AtomicOpImpl<T,4>& Omega::AtomicOpImpl<T,4>::operator = (const T& rhs)
{
	exchange(rhs);
	return (*this);
}

template <class T>
T Omega::AtomicOpImpl<T,4>::value() const
{
	return m_value;
}

template <class T>
T& Omega::AtomicOpImpl<T,4>::value()
{
	return m_value;
}

#endif

// SOME OLD CRAP THAT WILL BE USEFUL SOON...
#if 0
{
#if (defined (OMEGA_HAS_BUILTIN_ATOMIC_OP) && (ACE_SIZEOF_VOID_P==ACE_SIZEOF_LONG) && !defined(WIN32))
	// Lifted from ACE as there is no good way to reuse...
	public:
		static void init_functions (void);
	private:
		static long (*exchange_fn_) (volatile long *, long);

	#error Still need to implement this!
#endif

#if (defined (OMEGA_HAS_BUILTIN_ATOMIC_OP))
	protected:
		OBJECT* ExchangePtr(OBJECT* ptr)
		{
#if defined(WIN32)
			return reinterpret_cast<OBJECT*>(static_cast<LONG_PTR>(InterlockedExchange(reinterpret_cast<LONG*>(&this->m_ptr), static_cast<LONG>(reinterpret_cast<LONG_PTR>(ptr)))));
#else
			return reinterpret_cast<OBJECT*>((*exchange_fn_)(reinterpret_cast<volatile long*>(&this->m_ptr),reinterpret_cast<long>(ptr));
#endif /* WIN32 */
		}
#else /* OMEGA_HAS_BUILTIN_ATOMIC_OP */
	private:
		CriticalSection m_cs;

	protected:
		OBJECT* ExchangePtr(OBJECT* ptr)
		{
			Guard lock(m_cs);
			OBJECT* old = m_ptr;
			m_ptr = ptr;
			return old;
		}
#endif
}
#endif // 0

OOCORE_EXPORTED_FUNCTION(void*,cs__ctor,0,());
Omega::CriticalSection::CriticalSection()
{
	m_handle = static_cast<handle_t>(cs__ctor());
	if (!m_handle)
		OMEGA_THROW("Out of memory!");
}

OOCORE_EXPORTED_FUNCTION_VOID(cs__dctor,1,((in),void*,h));
Omega::CriticalSection::~CriticalSection()
{
	cs__dctor(m_handle);
}

OOCORE_EXPORTED_FUNCTION_VOID(cs_lock,1,((in),void*,h));
void Omega::CriticalSection::Lock()
{
	cs_lock(m_handle);
}

OOCORE_EXPORTED_FUNCTION_VOID(cs_unlock,1,((in),void*,h));
void Omega::CriticalSection::Unlock()
{
	cs_unlock(m_handle);
}

#endif // OOCORE_TYPES_INL_INCLUDED_
