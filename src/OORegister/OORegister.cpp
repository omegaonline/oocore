///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OORegister, the Omega Online registration tool
//
// OORegister is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OORegister is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OORegister.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "../OOBase/DLL.h"

#include <OOCore/OOCore.h>

#include <iostream>

static void print_help()
{
	std::cout << "OORegister - Registers a library with Omega Online." << std::endl << std::endl;
	std::cout << "Usage: OORegister [-i|u] [-s] library_name" << std::endl;
	std::cout << "--install (-i)\tInstall the library" << std::endl;
	std::cout << "--uninstall (-u)\tUninstall the library"  << std::endl;
	std::cout << "--silent (-s)\tSilent, do not output anything" << std::endl << std::endl;
	std::cout << "--current (-c)\tInstall for current user only" << std::endl << std::endl;
}

typedef Omega::System::MetaInfo::IException_Safe* (OMEGA_CALL *pfnRegisterLib)(Omega::System::MetaInfo::marshal_info<Omega::bool_t>::safe_type::type bInstall, Omega::System::MetaInfo::marshal_info<Omega::bool_t>::safe_type::type bLocal, Omega::System::MetaInfo::marshal_info<const Omega::string_t&>::safe_type::type strSubsts);

static void call_fn(pfnRegisterLib pfn, Omega::bool_t bInstall, Omega::bool_t bLocal, const Omega::string_t& strSubsts)
{
	Omega::System::MetaInfo::IException_Safe* pSE = pfn(
		Omega::System::MetaInfo::marshal_info<Omega::bool_t>::safe_type::coerce(bInstall),
		Omega::System::MetaInfo::marshal_info<Omega::bool_t>::safe_type::coerce(bLocal),
		Omega::System::MetaInfo::marshal_info<const Omega::string_t&>::safe_type::coerce(strSubsts));

	if (pSE)
		Omega::System::MetaInfo::throw_correct_exception(pSE);
}

static int do_install(bool bInstall, bool bLocal, bool bSilent, const char* lib_path)
{
	OOBase::DLL dll;
	int err = dll.load(lib_path);
	if (err != 0)
	{
		if (!bSilent)
			std::cerr << "Failed to load library '" << lib_path << "', error code: " << err << std::endl;

		return EXIT_FAILURE;
	}

	pfnRegisterLib pfnRegister = (pfnRegisterLib)dll.symbol("Omega_RegisterLibrary_Safe");
	if (pfnRegister == 0)
	{
		if (!bSilent)
			std::cerr << "Library missing 'Omega_RegisterLibrary_Safe' function" << err << std::endl;

		return EXIT_FAILURE;
	}

	try
	{
		// Call register
		call_fn(pfnRegister,bInstall,bLocal,L"LIB_PATH=" + Omega::string_t(lib_path,false));
	}
	catch (Omega::IException* pE)
	{
		if (!bSilent)
			std::cerr << "Function failed: " << pE->GetDescription().ToUTF8().c_str() << std::endl;

		pE->Release();
		return EXIT_FAILURE;
	}
	catch (...)
	{
		if (!bSilent)
			std::cerr << "Function failed with an unknown C++ exception" << std::endl;

		return EXIT_FAILURE;
	}

	if (!bSilent)
	{
		if (bInstall)
			std::cout << "Registration of '" << lib_path << "' successful" << std::endl << std::endl;
		else
			std::cout << "Unregistration of '" << lib_path << "' successful" << std::endl << std::endl;
	}

	return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
	// Parse cmd line first
	if (argc < 3)
	{
		std::cerr << "Invalid arguments" << std::endl << std::endl;
		print_help();
		return EXIT_FAILURE;
	}

	int nInstall = -1;
	bool bCurrent = false;
	bool bSilent = false;
	const char* pszLib = 0;
	for (int i=1;i<argc;++i)
	{
		if (strcmp(argv[i],"-i")==0 || strcmp(argv[i],"--install")==0)
		{
			if (nInstall == 0)
			{
				if (!bSilent)
				{
					std::cerr << "Do not mix --install and --uninstall" << std::endl << std::endl;
					print_help();
				}

				return EXIT_FAILURE;
			}
			nInstall = 1;
		}
		else if (strcmp(argv[i],"-u")==0 || strcmp(argv[i],"--uninstall")==0)
		{
			if (nInstall == 1)
			{
				if (!bSilent)
				{
					std::cerr << "Do not mix --uninstall and --install" << std::endl << std::endl;
					print_help();
				}

				return EXIT_FAILURE;
			}
			nInstall = 0;
		}
		else if (strcmp(argv[i],"-s")==0 || strcmp(argv[i],"--silent")==0)
		{
			bSilent = true;
		}
		else if (strcmp(argv[i],"-c")==0 || strcmp(argv[i],"--current")==0)
		{
			bCurrent = true;
		}
		else
		{
			if (i != argc-1 && !bSilent)
				std::cout << "Warning: Only the first library on the command line will be registered" << std::endl;
			
			pszLib = argv[i];
			break;
		}
	}

	if (!pszLib)
	{
		if (!bSilent)
		{
			std::cerr << "No library filename supplied" << std::endl << std::endl;
			print_help();
		}

		return EXIT_FAILURE;
	}
	else if (nInstall == -1)
	{
		if (!bSilent)
		{
			std::cerr << "You must supply either --install or --uninstall" << std::endl << std::endl;
			print_help();
		}

		return EXIT_FAILURE;
	}

	Omega::IException* pE = Omega::Initialize();
	if (pE)
	{
		if (!bSilent)
			std::cerr << "Omega::Initialize failed: " << pE->GetDescription().ToUTF8().c_str() << std::endl << std::endl;

		pE->Release();
		return EXIT_FAILURE;
	}

	int res = do_install((nInstall == 1),bCurrent,bSilent,pszLib);

	Omega::Uninitialize();

	return res;
}
