///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 1999 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

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
		inline guid_t GetUnsupportedIID()
		{
			return m_iid;
		}
	};
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IException*,IException_Create,3,((in),const string_t&,desc,(in),const string_t&,source,(in),IException*,pCause))
{
    ObjectImpl<OOCore::Exception>* pExcept = ObjectImpl<OOCore::Exception>::CreateInstance();
	pExcept->m_ptrCause = pCause;
	pExcept->m_strDesc = desc;
	pExcept->m_strSource = source;
	return pExcept;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(INoInterfaceException*,INoInterfaceException_Create,2,((in),const guid_t&,iid,(in),const string_t&,source))
{
	ObjectImpl<OOCore::NoInterfaceException>* pExcept = ObjectImpl<OOCore::NoInterfaceException>::CreateInstance();
	pExcept->m_strDesc = L"Object does not support the requested interface";
	pExcept->m_strSource = source;
	pExcept->m_iid = iid;
	return pExcept;
}
