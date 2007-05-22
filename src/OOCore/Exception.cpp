#include "OOCore_precomp.h"

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	class Exception :
		public ExceptionImpl<IException>
	{
	public:
		BEGIN_INTERFACE_MAP(Exception )
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<IException>)
		END_INTERFACE_MAP()
	};

	class NoInterfaceException :
		public ExceptionImpl<INoInterfaceException>
	{
	public:
		guid_t m_iid;

		BEGIN_INTERFACE_MAP(NoInterfaceException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<INoInterfaceException>)
		END_INTERFACE_MAP()

	// INoInterfaceException members
	public:
		guid_t GetUnsupportedIID();
	};
}

guid_t OOCore::NoInterfaceException::GetUnsupportedIID()
{
	return m_iid;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IException*,IException_Create,3,((in),const char_t*,desc,(in),const char_t*,source,(in),IException*,pCause))
{
	OutputDebugString(string_t::Format("Exception: %s at %s\n",desc,source));

	ObjectImpl<OOCore::Exception>* pExcept = ObjectImpl<OOCore::Exception>::CreateInstance();
	pExcept->m_ptrCause = pCause; 
	pExcept->m_strDesc = desc;
	pExcept->m_strSource = source;
	return pExcept;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(INoInterfaceException*,INoInterfaceException_Create,2,((in),const guid_t&,iid,(in),const char_t*,source))
{
	ObjectImpl<OOCore::NoInterfaceException>* pExcept = ObjectImpl<OOCore::NoInterfaceException>::CreateInstance();
	pExcept->m_strDesc = "Object does not support the requested interface";
	pExcept->m_strSource = source;
	pExcept->m_iid = iid;
	return pExcept;
}
