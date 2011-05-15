///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
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

#include "IPS.h"

#if defined(_WIN32)
// For the Windows path functions
#include <shlwapi.h>
#include <shlobj.h>
#endif

using namespace Omega;
using namespace OTL;

namespace
{
	class BadNameException :
			public ExceptionImpl<Registry::IBadNameException>
	{
	public:
		BEGIN_INTERFACE_MAP(BadNameException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<IBadNameException>)
		END_INTERFACE_MAP()

		string_t m_strName;

	public:
		string_t GetName()
		{
			return m_strName;
		}

		static void Throw(const string_t& name)
		{
			ObjectImpl<BadNameException>* pRE = ObjectImpl<BadNameException>::CreateInstance();
			pRE->m_strName = name;
			pRE->m_strDesc = L"Invalid name for registry key or value: '{0}'." % name;
			throw static_cast<Registry::IBadNameException*>(pRE);
		}
	};
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Registry::IKey*,OOCore_IRegistryKey_OpenKey,2,((in),const string_t&,key,(in),Registry::IKey::OpenFlags_t,flags))
{
	if (key.Left(1) != L"/")
		BadNameException::Throw(key);

	ObjectPtr<OOCore::IInterProcessService> ptrIPS = OOCore::GetInterProcessService();
	if (!ptrIPS)
		throw IInternalException::Create("Omega::Initialize not called","OOCore");

	ObjectPtr<Registry::IKey> ptrKey;
	ptrKey.Attach(static_cast<Registry::IKey*>(ptrIPS->GetRegistry()));

	if (key == L"/")
		return ptrKey.AddRef();
	else
		return ptrKey->OpenSubKey(key.Mid(1),flags);
}
