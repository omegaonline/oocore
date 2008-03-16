#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#endif

#include <ace/Get_Opt.h>
#include <ace/OS_NS_stdio.h>
#include <ace/DLL_Manager.h>
#include <ace/DLL.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <OOCore/OOCore.h>

static void print_help()
{
	ACE_OS::fprintf(stdout,ACE_TEXT("OORegister - Registers a library with OmegaOnline.\n\n"));
	ACE_OS::fprintf(stdout,ACE_TEXT("Usage: OORegister [-i] [-u] library_name\n"));
	ACE_OS::fprintf(stdout,ACE_TEXT("-i\tInstall the library\n"));
	ACE_OS::fprintf(stdout,ACE_TEXT("-u\tUninstall the library\n"));
	ACE_OS::fprintf(stdout,ACE_TEXT("-s\tSilent, do not output anything\n"));
	ACE_OS::fprintf(stdout,ACE_TEXT("\n"));
}

static int do_install(bool bInstall, bool bSilent, ACE_TCHAR* lib_path)
{
	ACE_DLL dll;
	if (dll.open(lib_path,RTLD_NOW)!=0)
	{
		if (bSilent)
			return -1;
		else
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Failed to load library '%s': %m\n\n"),lib_path),-1);
	}

	typedef Omega::System::MetaInfo::IException_Safe* (OMEGA_CALL *pfnRegisterLib)(Omega::System::MetaInfo::marshal_info<Omega::bool_t>::safe_type::type bInstall, Omega::System::MetaInfo::marshal_info<const Omega::string_t&>::safe_type::type strSubsts);

	pfnRegisterLib pfn=(pfnRegisterLib)dll.symbol(ACE_TEXT("Omega_RegisterLibrary_Safe"));
	if (pfn == 0)
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

		Omega::System::MetaInfo::IException_Safe* pSE = pfn(
			Omega::System::MetaInfo::marshal_info<Omega::bool_t>::safe_type::coerce(bInstall),
			Omega::System::MetaInfo::marshal_info<const Omega::string_t&>::safe_type::coerce(strSubsts));

		if (pSE)
			Omega::System::MetaInfo::throw_correct_exception(pSE);
	}
	catch (Omega::IException* pE)
	{
		if (!bSilent)
			ACE_ERROR((LM_ERROR,ACE_TEXT("Function failed: %W\n\n"),pE->Description().c_str()));

		pE->Release();
		return -1;
	}
	catch (...)
	{
		if (bSilent)
			return -1;
		else
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Function failed with an unknown C++ exception\n\n")),-1);
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
	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":ius"));
	int option;
	bool bInstall = true;
	bool bSilent = false;
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
			ACE_ERROR((LM_ERROR,ACE_TEXT("Function failed: %W\n\n"),pE->Description().c_str()));

		pE->Release();
		return -1;
	}

	int res = do_install(bInstall,bSilent,argv[argc-1]);

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
