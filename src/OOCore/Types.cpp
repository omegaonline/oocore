#include "OOCore_precomp.h"

namespace OOCore
{
	struct StringNode
	{
		StringNode() : m_refcount(1)
		{}

		StringNode(const Omega::char_t* sz) : m_str(sz), m_refcount(1)
		{}

		StringNode(const ACE_String_Base<Omega::char_t>& s) : m_str(s), m_refcount(1)
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

		ACE_String_Base<Omega::char_t>	m_str;

	private:
		Omega::AtomicOp<Omega::uint32_t>::type	m_refcount;
	};
}

using namespace OOCore;

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t__ctor1,0,())
{
	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode());
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t__ctor2,1,((in),const Omega::char_t*,sz))
{
	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(sz));
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t__ctor3,1,((in),const void*,s1))
{
	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(static_cast<const StringNode*>(s1)->m_str));
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

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_assign_2,2,((in),void*,s1,(in),const Omega::char_t*,sz))
{
	static_cast<StringNode*>(s1)->Release();
	
	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(sz));
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(const Omega::char_t*,string_t_cast,1,((in),const void*,s1))
{
	return static_cast<const StringNode*>(s1)->m_str.c_str();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(bool,string_t_eq1,2,((in),const void*,s1,(in),const void*,s2))
{
	return (static_cast<const StringNode*>(s1)->m_str == static_cast<const StringNode*>(s2)->m_str);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(bool,string_t_eq2,2,((in),const void*,s1,(in),const Omega::char_t*,sz))
{
	return (static_cast<const StringNode*>(s1)->m_str == sz);
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

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_add2,2,((in),void*,s1,(in),const Omega::char_t*,sz))
{
	StringNode* pOld = static_cast<StringNode*>(s1);
	
	StringNode* pNode;
	OMEGA_NEW(pNode,StringNode(pOld->m_str));
	if (!pNode)
		OOCORE_THROW_ERRNO(ENOMEM);	

	pOld->Release();

	pNode->m_str += sz;
	return pNode;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,string_t_cmp1,2,((in),const void*,s1,(in),const void*,s2))
{
	return ACE_OS::strcmp(static_cast<const StringNode*>(s1)->m_str.c_str(),static_cast<const StringNode*>(s2)->m_str.c_str());
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,string_t_cmp2,2,((in),const void*,s1,(in),const Omega::char_t*,sz))
{
	return ACE_OS::strcmp(static_cast<const StringNode*>(s1)->m_str.c_str(),sz);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,string_t_cnc1,2,((in),const void*,s1,(in),const void*,s2))
{
	return ACE_OS::strcasecmp(static_cast<const StringNode*>(s1)->m_str.c_str(),static_cast<const StringNode*>(s2)->m_str.c_str());
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int,string_t_cnc2,2,((in),const void*,s1,(in),const Omega::char_t*,sz))
{
	return ACE_OS::strcasecmp(static_cast<const StringNode*>(s1)->m_str.c_str(),sz);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(bool,string_t_isempty,1,((in),const void*,s1))
{
	return static_cast<const StringNode*>(s1)->m_str.empty();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_tolower,1,((in),const void*,s1))
{
	Omega::char_t* pszNew = ACE_OS::strdup(static_cast<const StringNode*>(s1)->m_str.c_str());
	if (!pszNew)
		return 0;

	for (Omega::char_t* p=pszNew;*p!='\0';++p)
	{
		*p = static_cast<Omega::char_t>(ACE_OS::ace_tolower(*p));
	}

	StringNode* s2;
	OMEGA_NEW(s2,StringNode(pszNew));

	free(pszNew);

	if (!s2)
		OOCORE_THROW_ERRNO(ENOMEM);	
	
	return s2;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_toupper,1,((in),const void*,s1))
{
	Omega::char_t* pszNew = ACE_OS::strdup(static_cast<const StringNode*>(s1)->m_str.c_str());
	if (!pszNew)
		return 0;

	for (Omega::char_t* p=pszNew;*p!='\0';++p)
	{
		*p = static_cast<Omega::char_t>(ACE_OS::ace_toupper(*p));
	}

	StringNode* s2;
	OMEGA_NEW(s2,StringNode(pszNew));
	
	free(pszNew);
		
	if (!s2)
		OOCORE_THROW_ERRNO(ENOMEM);	

	return s2;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(size_t,string_t_find1,3,((in),const void*,s1,(in),const void*,s2,(in),size_t,pos))
{
	return static_cast<const StringNode*>(s1)->m_str.find(static_cast<const StringNode*>(s2)->m_str,pos);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(size_t,string_t_find2,4,((in),const void*,s1,(in),Omega::char_t,c,(in),size_t,pos,(in),bool,bIgnoreCase))
{
	return static_cast<const StringNode*>(s1)->m_str.find(bIgnoreCase ? static_cast<Omega::char_t>(ACE_OS::ace_towlower(c)) : c,pos);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(size_t,string_t_rfind,4,((in),const void*,s1,(in),Omega::char_t,c,(in),size_t,pos,(in),bool,bIgnoreCase))
{
	return static_cast<const StringNode*>(s1)->m_str.rfind(bIgnoreCase ? static_cast<Omega::char_t>(ACE_OS::ace_towlower(c)) : c,pos);
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

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,string_t_format,2,((in),const Omega::char_t*,sz,(in),va_list,ap))
{
	for (int len=64;;len*=2)
	{
		const ACE_String_Base<Omega::char_t> s(static_cast<size_t>(len));
		
		int len2 = ACE_OS::vsnprintf(const_cast<Omega::char_t*>(s.fast_rep()),len,sz,ap);
		if (len2 <= len && len2 != -1)
		{
			StringNode* s1;
			OMEGA_NEW(s1,StringNode(ACE_String_Base<Omega::char_t>(s.c_str(),len2)));
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

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::guid_t,guid_t_from_string,1,((in),const Omega::char_t*,sz))
{
	// We use an array here because sscanf reads int's...
	int data[11] = { 0 };

	if (::sscanf(sz,
		"{%8x-%4x-%4x-%2x%2x-%2x%2x%2x%2x%2x%2x}",
		&data[0],
		&data[1],
		&data[2],
		&data[3],
		&data[4],
		&data[5],
		&data[6],
		&data[7],
		&data[8],
		&data[9],
		&data[10]) != 11)
	{
		return Omega::guid_t::NIL;
	}

	Omega::guid_t guid;
	guid.Data1 = data[0];
	guid.Data2 = static_cast<Omega::uint16_t>(data[1]);
	guid.Data3 = static_cast<Omega::uint16_t>(data[2]);
	guid.Data4[0] = static_cast<Omega::byte_t>(data[3]);
	guid.Data4[1] = static_cast<Omega::byte_t>(data[4]);
	guid.Data4[2] = static_cast<Omega::byte_t>(data[5]);
	guid.Data4[3] = static_cast<Omega::byte_t>(data[6]);
	guid.Data4[4] = static_cast<Omega::byte_t>(data[7]);
	guid.Data4[5] = static_cast<Omega::byte_t>(data[8]);
	guid.Data4[6] = static_cast<Omega::byte_t>(data[9]);
	guid.Data4[7] = static_cast<Omega::byte_t>(data[10]);

	return guid;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,cs__ctor,0,())
{
	ACE_Recursive_Thread_Mutex* pm;
	OMEGA_NEW(pm,ACE_Recursive_Thread_Mutex);
	return pm;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(cs__dctor,1,((in),void*,m1))
{
	delete static_cast<ACE_Recursive_Thread_Mutex*>(m1);
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(cs_lock,1,((in),void*,m1))
{
	static_cast<ACE_Recursive_Thread_Mutex*>(m1)->acquire();
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(cs_unlock,1,((in),void*,m1))
{
	static_cast<ACE_Recursive_Thread_Mutex*>(m1)->release();
}
