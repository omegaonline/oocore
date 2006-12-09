#include "OOCore_precomp.h"

using namespace Omega;
using namespace OTL;

class RealExceptionImpl :
	public ExceptionImpl<IException>
{
public:
	BEGIN_INTERFACE_MAP(RealExceptionImpl )
		INTERFACE_ENTRY_CHAIN(ExceptionImpl<IException>)
	END_INTERFACE_MAP()
};

class NoInterfaceExceptionImpl :
	public ExceptionImpl<INoInterfaceException>
{
public:
	guid_t m_iid;

	BEGIN_INTERFACE_MAP(NoInterfaceExceptionImpl)
		INTERFACE_ENTRY_CHAIN(ExceptionImpl<INoInterfaceException>)
	END_INTERFACE_MAP()

// INoInterfaceException members
public:
	guid_t GetUnsupportedIID();
};

guid_t NoInterfaceExceptionImpl::GetUnsupportedIID()
{
	return m_iid;
}

#if (defined(_MSC_VER) && _MSC_VER>=1300)
// These functions contain unreachable code, which we know about, so shut up the warning
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(IException_Throw,3,((in),const char_t*,desc,(in),const char_t*,source,(in),IException*,pCause))
{
	ObjectImpl<RealExceptionImpl>* pExcept = ObjectImpl<RealExceptionImpl>::CreateObject();
	pExcept->m_ptrCause.Attach(pCause); 
	pExcept->m_strDesc = desc;
	pExcept->m_strSource = source;
	throw pExcept;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(INoInterfaceException_Throw,2,((in),const guid_t&,iid,(in),const char_t*,source))
{
	ObjectImpl<NoInterfaceExceptionImpl>* pExcept = ObjectImpl<NoInterfaceExceptionImpl>::CreateObject();
	pExcept->m_strDesc = "Object does not support the requested interface {" + iid + "}.";
	pExcept->m_strSource = source;
	pExcept->m_iid = iid;
	throw pExcept;
}

#if (defined(_MSC_VER) && _MSC_VER>=1300)
// These functions contain unreachable code, which we know about, so shut up the warning
#pragma warning(pop)
#endif
