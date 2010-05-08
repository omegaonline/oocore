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
#include "../OOBase/CmdArgs.h"

#include "../include/Omega/Omega.h"

#include <iostream>

static void print_help()
{
	std::cout << "OORegister - Registers a library with Omega Online." << std::endl << std::endl;
	std::cout << "Usage: OORegister [options] <lib1> ... <libN>" << std::endl << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "  --help (-h)      Display this help text" << std::endl;
	std::cout << "  --version (-v)   Display version information" << std::endl;
	std::cout << "  --install (-i)   Install the libraries" << std::endl;
	std::cout << "  --uninstall (-u) Uninstall the libraries"  << std::endl;
	std::cout << "  --silent (-s)    Silent, do not output anything" << std::endl;
	std::cout << "  --current (-c)   Install for current user only" << std::endl;
	std::cout << std::endl;
}

static void print_version()
{
	std::cout << "OORegister version information:" << std::endl;
#if defined(OMEGA_DEBUG)
	std::cout << "Version: " << OOCORE_VERSION << " (Debug build)" << std::endl;
#else
	std::cout << "Version: " << OOCORE_VERSION << std::endl;
#endif
	std::cout << "Compiler: " << OMEGA_COMPILER_STRING << std::endl;
	std::cout << "OOCore library version: " << OOCore::GetVersion() << std::endl;
	std::cout << std::endl;
}

typedef Omega::System::Internal::SafeShim* (OMEGA_CALL *pfnRegisterLib)(Omega::System::Internal::marshal_info<Omega::bool_t>::safe_type::type bInstall, Omega::System::Internal::marshal_info<Omega::bool_t>::safe_type::type bLocal, Omega::System::Internal::marshal_info<const Omega::string_t&>::safe_type::type strSubsts);

static void call_fn(pfnRegisterLib pfn, Omega::bool_t bInstall, Omega::bool_t bLocal, const Omega::string_t& strSubsts)
{
	Omega::System::Internal::SafeShim* pSE = pfn(
				Omega::System::Internal::marshal_info<Omega::bool_t>::safe_type::coerce(bInstall),
				Omega::System::Internal::marshal_info<Omega::bool_t>::safe_type::coerce(bLocal),
				Omega::System::Internal::marshal_info<const Omega::string_t&>::safe_type::coerce(strSubsts));

	if (pSE)
		Omega::System::Internal::throw_correct_exception(pSE);
}

static bool do_install(bool bInstall, bool bLocal, bool bSilent, const char* lib_path)
{
	OOBase::DLL dll;
	int err = dll.load(lib_path);
	if (err != 0)
	{
		if (!bSilent)
			std::cerr << "Failed to load library '" << lib_path << "', error code: " << err << std::endl;

		return false;
	}

	pfnRegisterLib pfnRegister = (pfnRegisterLib)dll.symbol("Omega_RegisterLibrary_Safe");
	if (pfnRegister == 0)
	{
		if (!bSilent)
			std::cerr << "Library missing 'Omega_RegisterLibrary_Safe' function" << err << std::endl;

		return false;
	}

	try
	{
		// Call register
		call_fn(pfnRegister,bInstall,bLocal,L"LIB_PATH=" + Omega::string_t(lib_path,false));
	}
	catch (Omega::IException* pE)
	{
		if (!bSilent)
			std::cerr << "Registration failed: " << pE->GetDescription().ToUTF8().c_str() << std::endl;

		pE->Release();
		return false;
	}
	catch (...)
	{
		if (!bSilent)
			std::cerr << "Registration failed with an unknown C++ exception" << std::endl;

		return false;
	}

	if (!bSilent)
	{
		if (bInstall)
			std::cout << "Registration of '" << lib_path << "' successful" << std::endl << std::endl;
		else
			std::cout << "Unregistration of '" << lib_path << "' successful" << std::endl << std::endl;
	}

	return true;
}

namespace OOBase
{
	// This is the critical failure hook - only called by the arg parser
	void CriticalFailure(const char* msg)
	{
		std::cerr << msg << std::endl << std::endl;
		abort();
	}
}

int main(int argc, char* argv[])
{
	// Set up the command line args
	OOSvrBase::CmdArgs cmd_args;
	cmd_args.add_option("install",'i');
	cmd_args.add_option("uninstall",'u');
	cmd_args.add_option("help",'h');
	cmd_args.add_option("silent",'s');
	cmd_args.add_option("current",'c');
	cmd_args.add_option("version",'v');

	// Parse command line
	std::map<std::string,std::string> args;
	if (!cmd_args.parse(argc,argv,args))
		return EXIT_FAILURE;

	// Check for help first
	if (!args["help"].empty())
	{
		print_help();
		return EXIT_SUCCESS;
	}

	if (args["version"] == "true")
	{
		print_version();
		return EXIT_SUCCESS;
	}

	bool bSilent = (args["silent"] == "true");

	if ((args["install"].empty() && args["uninstall"].empty()) ||
			(!args["install"].empty() && !args["uninstall"].empty()))
	{
		if (!bSilent)
		{
			std::cerr << "You must supply either --install or --uninstall" << std::endl << std::endl;
			print_help();
		}
		return EXIT_FAILURE;
	}

	if (args["arg0"].empty())
	{
		if (!bSilent)
		{
			std::cerr << "No libraries supplied" << std::endl << std::endl;
			print_help();
		}
		return EXIT_FAILURE;
	}

	// Allow standalone startup
	Omega::IException* pE = Omega::Initialize(true);
	if (pE)
	{
		if (!bSilent)
			std::cerr << "Omega::Initialize failed: " << pE->GetDescription().ToUTF8().c_str() << std::endl << std::endl;

		pE->Release();
		return EXIT_FAILURE;
	}

	bool bCurrent = (args["current"] == "true");
	bool bInstall = !args["install"].empty();

	bool bOk = true;
	for (int i=0; bOk; ++i)
	{
		std::ostringstream ss;
		ss.imbue(std::locale::classic());
		ss << "arg" << i;
		std::string strLib = args[ss.str()];
		if (strLib.empty())
			break;

		bOk = do_install(bInstall,bCurrent,bSilent,strLib.c_str());
	}

	Omega::Uninitialize();

	return bOk ? EXIT_SUCCESS : EXIT_FAILURE;
}
