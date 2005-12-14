#include <ace/Get_Opt.h>
#include <ace/OS_NS_stdio.h>
#include <ace/DLL_Manager.h>

#include <OOCore/OOCore.h>

static void print_help()
{
	ACE_OS::printf(ACE_TEXT("OORegister - Registers a library with OmegaOnline.\n\n" \
		"Usage: OORegister [-i] [-u] library_name\n" \
		"-i\tInstall the library\n" \
		"-u\tUninstall the library\n" \
		"\n"));
}

static int do_install(bool bInstall, ACE_TCHAR* lib_path)
{
	ACE_DLL_Handle dll;
	if (dll.open(lib_path,RTLD_NOW,ACE_SHLIB_INVALID_HANDLE)!=0)
	{
		print_help();
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Failed to load library %s.\n\n"),lib_path),-1);
	}

	OOCore::RegisterLib_Function fn=(OOCore::RegisterLib_Function)dll.symbol(ACE_TEXT("RegisterLib"));
	if (fn==0)
	{
		print_help();
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Library missing 'RegisterLib' function.\n\n"),lib_path),-1);
	}

	if ((fn)(bInstall)!=0)
	{
		print_help();
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("'RegisterLib' function failed: %m\n\n")),-1);
	}

	ACE_OS::printf(ACE_TEXT("Registration of %s successful.\n\n"),dll.dll_name());

	return 0;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
	// Parse cmd line first
	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":iu"));
	int option;
	bool bInstall = true;
	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
		case ACE_TEXT('i'):
			break;

		case ACE_TEXT('u'):
			bInstall = false;
			break;

		case ACE_TEXT(':'):
			print_help();
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Missing argument for -%c.\n\n"),cmd_opts.opt_opt()),-1);
			
		default:
			print_help();
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Invalid argument -%c.\n\n"),cmd_opts.opt_opt()),-1);
		}
	}

	if (cmd_opts.opt_ind()==1)
	{
		print_help();
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Missing argument.\n\n")),-1);
	}

	if (cmd_opts.opt_ind()!=(argc-1))
	{
		print_help();
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Invalid number of parameters.\n\n")),-1);
	}

	return do_install(bInstall,argv[argc-1]);
}