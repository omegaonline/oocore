#include "stdafx.h"

static int Help()
{
	ACE_OS::printf("This is the help string\n\n");
	return 0;
}

static int PrintException(Omega::IException* pE)
{
	ACE_OS::printf("%ls.\n",pE->Description().c_str());
	pE->Release();
	return -1;
}

static int Process(Omega::string_t& strKey, const std::vector<Omega::string_t>& args)
{
	return 0;
}

static int Parse(ACE_TCHAR* szBuf, Omega::string_t& strKey)
{
	std::vector<Omega::string_t> args; 
	ACE_TCHAR* context = 0;
	for(;;)
	{
		ACE_TCHAR* command = ACE_OS::strtok_r(szBuf,ACE_TEXT(" \t\r\n"),&context);
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
		Omega::string_t strKey = L"\\";
		for (;;)
		{
			// Print the prompt
			ACE_OS::printf("%ls > ",strKey.c_str());
			
			// Get the next input...
			ACE_TCHAR szBuf[1024];
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
	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":h"));
	if (cmd_opts.long_option(ACE_TEXT("help"),ACE_TEXT('h'))!=0)
	{
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Error parsing cmdline")),-1);
	}

	int option;
	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
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
		std::vector<Omega::string_t> args; 
		for (int i=next;i<argc;++i)
			args.push_back(argv[i]);

		Omega::string_t strKey = L"\\";
		err = Process(strKey,args);
	}
	else
	{
		err = Interactive();
	}

	Omega::Uninitialize();

	return err;
}