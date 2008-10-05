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

#include <OOCore/OOCore.h>

#if defined(OMEGA_WIN32) && !defined(WIN32)
#define WIN32
#endif

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4355) // 'this' : used in base member initializer list
#if (_MSC_VER == 1310)
#pragma warning(disable : 4244) // 'argument' : conversion from 't1' to 't2', possible loss of data
#endif
#if (_MSC_VER >= 1400)
#pragma warning(disable : 4996) // 'function' was declared deprecated 
#endif
#endif

#include <ace/Get_Opt.h>
#include <ace/OS_NS_stdio.h>
#include <ace/DLL_Manager.h>
#include <ace/DLL.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

static void print_help()
{
	ACE_OS::printf("OORegister - Registers a library with Omega Online.\n\n");
	ACE_OS::printf("Usage: OORegister [-i] [-u] library_name\n");
	ACE_OS::printf("-i\tInstall the library\n");
	ACE_OS::printf("-u\tUninstall the library\n");
	ACE_OS::printf("-s\tSilent, do not output anything\n");
	ACE_OS::printf("\n");
}

typedef Omega::System::MetaInfo::IException_Safe* (OMEGA_CALL *pfnInstallLib)(Omega::System::MetaInfo::marshal_info<Omega::bool_t>::safe_type::type bInstall, Omega::System::MetaInfo::marshal_info<Omega::bool_t>::safe_type::type bLocal, Omega::System::MetaInfo::marshal_info<const Omega::string_t&>::safe_type::type strSubsts);

static void call_fn(pfnInstallLib pfn, Omega::bool_t bInstall, Omega::bool_t bLocal, const Omega::string_t& strSubsts)
{
	Omega::System::MetaInfo::IException_Safe* pSE = pfn(
		Omega::System::MetaInfo::marshal_info<Omega::bool_t>::safe_type::coerce(bInstall),
		Omega::System::MetaInfo::marshal_info<Omega::bool_t>::safe_type::coerce(bLocal),
		Omega::System::MetaInfo::marshal_info<const Omega::string_t&>::safe_type::coerce(strSubsts));

	if (pSE)
		Omega::System::MetaInfo::throw_correct_exception(pSE);
}

static int do_install(bool bInstall, bool bLocal, bool bSilent, ACE_TCHAR* lib_path)
{
	ACE_DLL dll;
	if (dll.open(lib_path,RTLD_NOW)!=0)
	{
		if (bSilent)
			return -1;
		else
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Failed to load library '%s': %m\n\n"),lib_path),-1);
	}	
	
	pfnInstallLib pfnInstall = (pfnInstallLib)dll.symbol(ACE_TEXT("Omega_InstallLibrary_Safe"));
	
	pfnInstallLib pfnRegister = (pfnInstallLib)dll.symbol(ACE_TEXT("Omega_RegisterLibrary_Safe"));
	if (pfnRegister == 0)
	{
		if (bSilent)
			return -1;
		else
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Library missing 'Omega_RegisterLibrary_Safe' function.\n\n"),lib_path),-1);
	}

	try
	{
		Omega::string_t strSubsts = L"LIB_PATH=";
		strSubsts += ACE_TEXT_ALWAYS_WCHAR(lib_path);

		// Call install if found
		if (pfnInstall)
			call_fn(pfnInstall,bInstall,bLocal,strSubsts);
		
		// Call register
		call_fn(pfnRegister,bInstall,bLocal,strSubsts);
	}
	catch (Omega::IException* pE)
	{
		if (!bSilent)
			ACE_ERROR((LM_ERROR,ACE_TEXT("Function failed: %W.\n\n"),pE->GetDescription().c_str()));

		pE->Release();
		return -1;
	}
	catch (...)
	{
		if (bSilent)
			return -1;
		else
			ACE_ERROR_RETURN((LM_ERROR,"Function failed with an unknown C++ exception.\n\n"),-1);
	}

	if (!bSilent)
	{
		if (bInstall)
			ACE_OS::fprintf(stdout,ACE_TEXT("Registration of '%s' successful.\n\n"),lib_path);
		else
			ACE_OS::fprintf(stdout,ACE_TEXT("Unregistration of '%s' successful.\n\n"),lib_path);
	}

	return 0;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
	// Parse cmd line first
	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":iusl"));
	int option;
	bool bInstall = true;
	bool bSilent = false;
	bool bLocal = false;
	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
		case ACE_TEXT('i'):
			break;

		case ACE_TEXT('u'):
			bInstall = false;
			break;

		case ACE_TEXT('s'):
			bSilent = true;
			break;

		case ACE_TEXT('l'):
			bLocal = true;
			break;

		case ACE_TEXT(':'):
			if (bSilent)
				return -1;
			else
			{
				print_help();
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Missing argument for -%c.\n\n"),cmd_opts.opt_opt()),-1);
			}

		default:
			if (bSilent)
				return -1;
			else
			{
				print_help();
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Invalid argument -%c.\n\n"),cmd_opts.opt_opt()),-1);
			}
		}
	}

	if (cmd_opts.opt_ind()==1)
	{
		if (bSilent)
			return -1;
		else
		{
			print_help();
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Missing argument.\n\n")),-1);
		}
	}

	if (cmd_opts.opt_ind()!=(argc-1))
	{
		if (bSilent)
			return -1;
		else
		{
			print_help();
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Invalid number of parameters.\n\n")),-1);
		}
	}

	// This gives a small leak, but allows ACE based DLL's a chance to unload correctly
	ACE_DLL_Manager::instance()->unload_policy(ACE_DLL_UNLOAD_POLICY_LAZY);

	Omega::IException* pE = Omega::Initialize();
	if (pE)
	{
		if (!bSilent)
			ACE_ERROR((LM_ERROR,ACE_TEXT("Function failed: %W.\n\n"),pE->GetDescription().c_str()));

		pE->Release();
		return -1;
	}

	int res = do_install(bInstall,bLocal,bSilent,argv[argc-1]);

	Omega::Uninitialize();

	return res;
}

#if defined(ACE_WIN32) && defined(ACE_USES_WCHAR) && defined(__MINGW32__)
#include <shellapi.h>
int main(int argc, char* /*argv*/[])
{
	// MinGW doesn't understand wmain, so...
	wchar_t** wargv = CommandLineToArgvW(GetCommandLineW(),&argc);

	ACE_Main m;
	return ace_os_wmain_i (m, argc, wargv);   /* what the user calls "main" */
}
#endif
