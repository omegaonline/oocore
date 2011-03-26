///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
//
// This file is part of OOReg, the Omega Online Registry editor application.
//
// OOReg is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOReg is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOReg.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include <OOBase/STLAllocator.h>

#include <iostream>

#include "../../include/Omega/Omega.h"

static void exception_details(Omega::IException* pE);

static void report_cause(Omega::IException* pE)
{
	Omega::IException* pCause = pE->GetCause();
	if (pCause)
	{
		std::cerr << "Cause: ";
		exception_details(pCause);
	}
}

void report_exception(Omega::IException* pE)
{
	std::cerr << "Exception: ";
	exception_details(pE);
}

static void exception_details(Omega::IException* pOrig)
{
	try
	{
		pOrig->Rethrow();
	}
	catch (Omega::IInternalException* pE)
	{
		std::cerr << "Omega::IInternalException - ";

		OOBase::local_string s;
		pE->GetDescription().ToNative(s);

		std::cerr << s << std::endl;

		Omega::string_t strSource = pE->GetSource();
		if (!strSource.IsEmpty())
		{
			strSource.ToNative(s);
			std::cerr << "At: " << s << std::endl;
		}

		report_cause(pE);
		pE->Release();
	}
	catch (Omega::IException* pE)
	{
		OOBase::local_string s;
		pE->GetDescription().ToNative(s);

		std::cerr << s << std::endl;
		report_cause(pE);
		pE->Release();
	}
}
