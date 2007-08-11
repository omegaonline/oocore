#include "OOCore_precomp.h"

namespace OOCore
{
	class UTF_Converter
	{
	public:
		ACE_WString from_utf8(const char* sz);
		ACE_CString to_utf8(const wchar_t* wsz);

	private:
		friend class ACE_Singleton<UTF_Converter,ACE_Thread_Mutex>;

		UTF_Converter() :
			m_conv(0)
		{
			wchar_t szBuf[] = L"This is my exciting text string, that should hopefully be picked up as the correct native wchar_t encoding";
			m_conv = ACE_Encoding_Converter_Factory::create(reinterpret_cast<const ACE_Byte*>(szBuf),sizeof(szBuf));
		}

		~UTF_Converter()
		{
			delete m_conv;
		}

		ACE_Encoding_Converter* m_conv;
	};
	typedef ACE_Singleton<UTF_Converter,ACE_Thread_Mutex> UTF_CONVERTER;

	struct StringNode
	{
		StringNode() : m_refcount(1)
		{}

		StringNode(const char* sz) : m_str(UTF_CONVERTER::instance()->from_utf8(sz)), m_refcount(1)
		{}

		StringNode(const wchar_t* sz) : m_str(sz), m_refcount(1)
		{}

		StringNode(const wchar_t* sz, size_t length) : m_str(sz,length), m_refcount(1)
		{}

		StringNode(const ACE_WString& s) : m_str(s), m_refcount(1)
		{}

		void AddRef()
		{
			++m_refcount;
		}

		void Release()
		{
			if (--m_refcount==0)
				delete this;
		}

		ACE_WString	m_str;

	private:
		ACE_Atomic_Op<ACE_Thread_Mutex,Omega::uint32_t> m_refcount;
	};
}

using namespace OOCore;

ACE_WString OOCore::UTF_Converter::from_utf8(const char* sz)
{
	if (!m_conv)
		OMEGA_THROW(L"Failed to construct utf converter!");

	for (int len=128;;len*=2)
	{
		wchar_t* pszBuf;
		OMEGA_NEW(pszBuf,wchar_t[len]);

		ACE_Encoding_Converter::Result res = m_conv->from_utf8(reinterpret_cast<const ACE_Byte*>(sz),ACE_OS::strlen(sz)+1,pszBuf,len);

		if (res == ACE_Encoding_Converter::CONVERSION_OK)
		{
			ACE_WString strRet = pszBuf;
			delete [] pszBuf;
			return strRet;
		}

		delete [] pszBuf;

		if (res != ACE_Encoding_Converter::TARGET_EXHAUSTED)
			OMEGA_THROW(L"utf8 decoding failed!");
	}
}

ACE_CString OOCore::UTF_Converter::to_utf8(const wchar_t* wsz)
{
	if (!m_conv)
		OMEGA_THROW(L"Failed to construct utf converter!");

	for (int len=256;;len*=2)
	{
		char* pszBuf;
		OMEGA_NEW(pszBuf,char[len]);

		ACE_Encoding_Converter::Result res = m_conv->to_utf8(reinterpret_cast<const ACE_Byte*>(wsz),(ACE_OS::strlen(wsz)+1)*sizeof(wchar_t),(ACE_Byte*)(pszBuf),len);

		if (res == ACE_Encoding_Converter::CONVERSION_OK)
		{
			ACE_CString strRet = pszBuf;
			delete [] pszBuf;
			return strRet;
		}

		delete [] pszBuf;

		if (res != ACE_Encoding_Converter::TARGET_EXHAUSTED)
			OMEGA_THROW(L"utf8 encoding failed!");
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t__ctor1,0,())
{
	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode());
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t__ctor2,2,((in),const char*,sz,(in),Omega::bool_t,bUTF8))
{
	StringNode* pNode;
	if (bUTF8)
		OMEGA_NEW(pNode,StringNode(sz));
	else
		OMEGA_NEW(pNode,StringNode(ACE_Ascii_To_Wide(sz).wchar_rep()));
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t__ctor3,1,((in),const void*,s1))
{
	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(static_cast<const StringNode*>(s1)->m_str));
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t__ctor4,2,((in),const wchar_t*,wsz,(in),size_t,length))
{
	StringNode* pNode;
	if (length == Omega::string_t::npos)
		OMEGA_NEW(pNode,StringNode(wsz));
	else
		OMEGA_NEW(pNode,StringNode(wsz,length));
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(string_t__dctor,1,((in),void*,s1))
{
	static_cast<StringNode*>(s1)->Release();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_assign_1,2,((in),void*,s1,(in),const void*,s2))
{
	static_cast<StringNode*>(s1)->Release();

	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(static_cast<const StringNode*>(s2)->m_str));
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_assign_2,2,((in),void*,s1,(in),const char*,sz))
{
	static_cast<StringNode*>(s1)->Release();

	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(ACE_Ascii_To_Wide(sz).wchar_rep()));
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_assign_3,2,((in),void*,s1,(in),const wchar_t*,wsz))
{
	static_cast<StringNode*>(s1)->Release();

	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(wsz));
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(const wchar_t*,string_t_cast,1,((in),const void*,s1))
{
	return static_cast<const StringNode*>(s1)->m_str.c_str();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(size_t,string_t_toutf8,3,((in),const void*,h,(in),char*,sz,(in),size_t,size))
{
	ACE_CString str = UTF_CONVERTER::instance()->to_utf8(static_cast<const StringNode*>(h)->m_str.c_str());
	if (size < str.length()+1)
	{
		ACE_OS::strncpy(sz,str.c_str(),size-1);
		sz[size-1] = '\0';
	}
	else
	{
		ACE_OS::strcpy(sz,str.c_str());
	}
	return str.length() + 1;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_add1,2,((in),void*,s1,(in),const void*,s2))
{
	StringNode* pOld = static_cast<StringNode*>(s1);

	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(pOld->m_str));
	if (!pNode)
		OOCORE_THROW_ERRNO(ENOMEM);

	pOld->Release();

	pNode->m_str += static_cast<const StringNode*>(s2)->m_str;
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_add2,2,((in),void*,s1,(in),const char*,sz))
{
	StringNode* pOld = static_cast<StringNode*>(s1);

	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(pOld->m_str));
	if (!pNode)
		OOCORE_THROW_ERRNO(ENOMEM);

	pOld->Release();

	pNode->m_str += ACE_Ascii_To_Wide(sz).wchar_rep();
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_add3,2,((in),void*,s1,(in),const wchar_t*,wsz))
{
	StringNode* pOld = static_cast<StringNode*>(s1);

	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(pOld->m_str));
	if (!pNode)
		OOCORE_THROW_ERRNO(ENOMEM);

	pOld->Release();

	pNode->m_str += wsz;
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,string_t_cmp1,2,((in),const void*,s1,(in),const void*,s2))
{
	return ACE_OS::strcmp(static_cast<const StringNode*>(s1)->m_str.c_str(),static_cast<const StringNode*>(s2)->m_str.c_str());
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,string_t_cmp2,2,((in),const void*,s1,(in),const char*,sz))
{
	return ACE_OS::strcmp(static_cast<const StringNode*>(s1)->m_str.c_str(),ACE_Ascii_To_Wide(sz).wchar_rep());
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,string_t_cmp3,2,((in),const void*,s1,(in),const wchar_t*,wsz))
{
	return ACE_OS::strcmp(static_cast<const StringNode*>(s1)->m_str.c_str(),wsz);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,string_t_cnc1,2,((in),const void*,s1,(in),const void*,s2))
{
	return ACE_OS::strcasecmp(static_cast<const StringNode*>(s1)->m_str.c_str(),static_cast<const StringNode*>(s2)->m_str.c_str());
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,string_t_cnc2,2,((in),const void*,s1,(in),const char*,sz))
{
	return ACE_OS::strcasecmp(static_cast<const StringNode*>(s1)->m_str.c_str(),ACE_Ascii_To_Wide(sz).wchar_rep());
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,string_t_cnc3,2,((in),const void*,s1,(in),const wchar_t*,wsz))
{
	return ACE_OS::strcasecmp(static_cast<const StringNode*>(s1)->m_str.c_str(),wsz);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(bool,string_t_isempty,1,((in),const void*,s1))
{
	return (static_cast<const StringNode*>(s1)->m_str.length() == 0);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_tolower,1,((in),const void*,s1))
{
	wchar_t* pszNew = ACE_OS::strdup(static_cast<const StringNode*>(s1)->m_str.c_str());
	if (!pszNew)
		return 0;

	for (wchar_t* p=pszNew;*p!=L'\0';++p)
	{
		*p = static_cast<wchar_t>(ACE_OS::ace_towlower(*p));
	}

	StringNode* s2;
	OMEGA_NEW(s2,StringNode(pszNew));

	ACE_OS::free(pszNew);

	if (!s2)
		OOCORE_THROW_ERRNO(ENOMEM);

	return s2;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_toupper,1,((in),const void*,s1))
{
	wchar_t* pszNew = ACE_OS::strdup(static_cast<const StringNode*>(s1)->m_str.c_str());
	if (!pszNew)
		return 0;

	for (wchar_t* p=pszNew;*p!=L'\0';++p)
	{
		*p = static_cast<wchar_t>(ACE_OS::ace_towupper(*p));
	}

	StringNode* s2;
	OMEGA_NEW(s2,StringNode(pszNew));

	ACE_OS::free(pszNew);

	if (!s2)
		OOCORE_THROW_ERRNO(ENOMEM);

	return s2;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(size_t,string_t_find1,3,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos))
{
	return static_cast<const StringNode*>(s1)->m_str.find(static_cast<const StringNode*>(s2)->m_str,pos);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(size_t,string_t_find2,4,((in),const void*,s1,(in),char,c,(in),size_t,pos,(in),bool,bIgnoreCase))
{
	ACE_WString str2 = ACE_Ascii_To_Wide(ACE_CString(c).c_str()).wchar_rep();
	wchar_t c2 = str2[0];

	if (bIgnoreCase)
		c2 = static_cast<wchar_t>(ACE_OS::ace_towlower(c2));
	
	return static_cast<const StringNode*>(s1)->m_str.find(c2,pos);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(size_t,string_t_find3,4,((in),const void*,s1,(in),wchar_t,c,(in),size_t,pos,(in),bool,bIgnoreCase))
{
	return static_cast<const StringNode*>(s1)->m_str.find(bIgnoreCase ? static_cast<wchar_t>(ACE_OS::ace_towlower(c)) : c,pos);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(size_t,string_t_rfind1,4,((in),const void*,s1,(in),char,c,(in),size_t,pos,(in),bool,bIgnoreCase))
{
	ACE_WString str2 = ACE_Ascii_To_Wide(ACE_CString(c).c_str()).wchar_rep();
	wchar_t c2 = str2[0];

	if (bIgnoreCase)
		c2 = static_cast<wchar_t>(ACE_OS::ace_towlower(c2));

	return static_cast<const StringNode*>(s1)->m_str.rfind(c2,pos);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(size_t,string_t_rfind2,4,((in),const void*,s1,(in),wchar_t,c,(in),size_t,pos,(in),bool,bIgnoreCase))
{
	return static_cast<const StringNode*>(s1)->m_str.rfind(bIgnoreCase ? static_cast<wchar_t>(ACE_OS::ace_towlower(c)) : c,pos);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(size_t,string_t_len,1,((in),const void*,s1))
{
	return static_cast<const StringNode*>(s1)->m_str.length();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_left,2,((in),const void*,s1,(in),size_t,length))
{
	StringNode* s2;
	OMEGA_NEW(s2,StringNode(static_cast<const StringNode*>(s1)->m_str.substr(0,length)));
	return s2;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_mid,3,((in),const void*,s1,(in),size_t,start,(in),size_t,length))
{
	StringNode* s2;
	OMEGA_NEW(s2,StringNode(static_cast<const StringNode*>(s1)->m_str.substr(start,length)));
	return s2;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_right,2,((in),const void*,s1,(in),size_t,length))
{
	size_t start = static_cast<const StringNode*>(s1)->m_str.length();
	if (length > start)
		start = 0;
	else
		start -= length;

	StringNode* s2;
	OMEGA_NEW(s2,StringNode(static_cast<const StringNode*>(s1)->m_str.substr(start)));
	return s2;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_format,2,((in),const wchar_t*,sz,(in),va_list,ap))
{
	ACE_WString s;
	for (int len=256;;len*=2)
	{
		s.fast_resize(static_cast<size_t>(len));

		int len2 = ACE_OS::vsnprintf(const_cast<wchar_t*>(s.fast_rep()),len,sz,ap);
		if (len2 <= len && len2 != -1)
		{
			StringNode* s1;
			OMEGA_NEW(s1,StringNode(ACE_WString(s.c_str(),len2)));
			return s1;
		}
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(string_t_clear,1,((in),void*,s1))
{
	static_cast<StringNode*>(s1)->m_str.clear();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(bool,guid_t_eq,2,((in),const Omega::guid_t&,lhs,(in),const Omega::guid_t&,rhs))
{
	return (lhs.Data1==rhs.Data1 && lhs.Data2==rhs.Data2 && lhs.Data3==rhs.Data3 && ACE_OS::memcmp(lhs.Data4,rhs.Data4,8)==0);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(bool,guid_t_less,2,((in),const Omega::guid_t&,lhs,(in),const Omega::guid_t&,rhs))
{
	return (ACE_OS::memcmp(&lhs,&rhs,sizeof(Omega::guid_t)) < 0);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::guid_t,guid_t_from_string,1,((in),const wchar_t*,sz))
{
	// We use an array here because sscanf reads int's...
	long data0 = 0;
	int data[11] = { 0 };

#if defined (ACE_HAS_TR24731_2005_CRT)
	if (swscanf_s(sz,
#else
	if (swscanf(sz,
#endif
		L"{%8lx-%4x-%4x-%2x%2x-%2x%2x%2x%2x%2x%2x}",
		&data0,
		&data[0],
		&data[1],
		&data[2],
		&data[3],
		&data[4],
		&data[5],
		&data[6],
		&data[7],
		&data[8],
		&data[9]) != 11)
	{
		return Omega::guid_t::Null();
	}

	Omega::guid_t guid;
	guid.Data1 = data0;
	guid.Data2 = static_cast<Omega::uint16_t>(data[0]);
	guid.Data3 = static_cast<Omega::uint16_t>(data[1]);
	guid.Data4[0] = static_cast<Omega::byte_t>(data[2]);
	guid.Data4[1] = static_cast<Omega::byte_t>(data[3]);
	guid.Data4[2] = static_cast<Omega::byte_t>(data[4]);
	guid.Data4[3] = static_cast<Omega::byte_t>(data[5]);
	guid.Data4[4] = static_cast<Omega::byte_t>(data[6]);
	guid.Data4[5] = static_cast<Omega::byte_t>(data[7]);
	guid.Data4[6] = static_cast<Omega::byte_t>(data[8]);
	guid.Data4[7] = static_cast<Omega::byte_t>(data[9]);

	return guid;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::guid_t,guid_t_create,0,())
{
#ifdef OMEGA_WIN32
	UUID uuid = {0,0,0, {0,0,0,0,0,0,0,0} };
	UuidCreate(&uuid);

	Omega::guid_t guid;
	guid = *(Omega::guid_t*)(&uuid);
	return guid;

#else
	static bool bInit = false;
	if (!bInit)
	{
		// We don't care if this gets called twice...
		ACE_Utils::UUID_GENERATOR::instance()->init();
		bInit = true;
	}

	ACE_Utils::UUID uuid;
	ACE_Utils::UUID_GENERATOR::instance()->generateUUID(uuid);

	const ACE_CString* pStr = uuid.to_string();
	if (!pStr)
		OOCORE_THROW_LASTERROR();

	char szBuf[64];
	ACE_OS::sprintf(szBuf,"{%s}",pStr->c_str());

	return Omega::guid_t::FromString(szBuf);
#endif
}
