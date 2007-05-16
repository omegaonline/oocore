#include "stdafx.h"

static int Help()
{
	ACE_OS::printf("This is the help string\n\n");
	return 0;
}

static int Version()
{
	ACE_OS::printf("Platform: %s\nCompiler: %s\nACE %s\n",OMEGA_PLATFORM_STRING,OMEGA_COMPILER_STRING,ACE_VERSION);
	return 0;
}

static int PrintException(Omega::IException* pE)
{
	ACE_OS::printf("%s.\n",(const char*)pE->Description());
	pE->Release();
	return -1;
}

static int Process(Omega::string_t& strKey, const std::vector<std::string>& args)
{
	return 0;
}

static int Parse(char* szBuf, Omega::string_t& strKey)
{
	std::vector<std::string> args; 
	char* context = 0;
	for(;;)
	{
		char* command = ACE_OS::strtok_r(szBuf," \t\r\n",&context);
		if (command == NULL)
			break;

		args.push_back(command);
		szBuf = NULL;
	}

	return Process(strKey,args);
}

static int Interactive()
{
	try
	{
		Omega::string_t strKey = "\\";
		for (;;)
		{
			// Print the prompt
			ACE_OS::printf("%s > ",(const char*)strKey);
			
			// Get the next input...
			char szBuf[1024];
			ACE_OS::fgets(szBuf,1024,stdin);

			// And process it...
			int err = Parse(szBuf,strKey);
			if (err != 0)
				return err;
		}
	}
	catch (Omega::IException* pE)
	{
		return PrintException(pE);
	}
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
	// Check command line options
	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":vh"));
	if (cmd_opts.long_option(ACE_TEXT("version"),ACE_TEXT('v'))!=0 ||
		cmd_opts.long_option(ACE_TEXT("help"),ACE_TEXT('h'))!=0)
	{
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error parsing cmdline")),-1);
	}

	int option;
	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
		case ACE_TEXT('v'):
			return Version();

		case ACE_TEXT('h'):
			return Help();

		case ACE_TEXT(':'):
			ACE_OS::printf("Missing argument for %s.\n\n",cmd_opts.last_option());
			return Help();

		default:
			ACE_OS::printf("Unrecognized argument '%s'.\n\n",cmd_opts.last_option());
			break;
		}
	}

	Omega::IException* pE = Omega::Initialize();
	if (pE)
		return PrintException(pE);

	int err = 0;
	int next = cmd_opts.opt_ind();
	if (next < argc)
	{
		std::vector<std::string> args; 
		for (int i=next;i<argc;++i)
			args.push_back(argv[i]);

		Omega::string_t strKey = "\\";
		err = Process(strKey,args);
	}
	else
	{
		err = Interactive();
	}

	Omega::Uninitialize();

	return err;
}