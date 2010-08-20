///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
//
// This file is part of OOHttpd, the Omega Online HTTP Server application.
//
// OOHttpd is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOHttpd is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOHttpd.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOHttpd.h"
#include "HttpService.h"

#ifdef HAVE_VLD_H
#include <vld.h>
#endif

BEGIN_PROCESS_OBJECT_MAP()
	OBJECT_MAP_ENTRY(Http::HttpService)
END_PROCESS_OBJECT_MAP()

int main(int /*argc*/, char* /*argv*/[])
{
	Omega::IException* pE = Omega::Initialize();
	if (pE)
	{
		pE->Release();
		return EXIT_FAILURE;
	}

	int ret = EXIT_SUCCESS;
	try
	{
		OTL::GetModule()->Run();
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
		ret = EXIT_FAILURE;
	}

	Omega::Uninitialize();

	return ret;
}

namespace OOBase
{
	// This is the critical failure hook
	void CriticalFailure(const char* msg)
	{
		throw Omega::IInternalException::Create(msg,"Critical Failure");
	}
}
