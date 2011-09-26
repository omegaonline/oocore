///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011 Rick Taylor
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

#include <OOBase/CmdArgs.h>

#include "../../include/Omega/Omega.h"
#include "../../include/Omega/OOCore_version.h"
#include "../../include/OTL/Registry.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(HAVE_VLD_H)
#include <vld.h>
#endif

static void exception_details(Omega::IException* pE);

static void report_cause(Omega::IException* pE)
{
	Omega::IException* pCause = pE->GetCause();
	if (pCause)
	{
		fputs("\nCause: ",stderr);
		exception_details(pCause);
	}
}

static void report_exception(Omega::IException* pE)
{
	fputs("Exception: ",stderr);
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
		fputs(pE->GetDescription().c_nstr(),stderr);

		Omega::string_t strSource = pE->GetSource();
		if (!strSource.IsEmpty())
		{
			fputs("\nAt: ",stderr);
			fputs(strSource.c_nstr(),stderr);
		}

		report_cause(pE);
		pE->Release();
	}
	catch (Omega::IException* pE)
	{
		fputs(pE->GetDescription().c_nstr(),stderr);

		report_cause(pE);
		pE->Release();
	}
}

static int version()
{
	printf("ooreg version %s",OOCORE_VERSION);

#if !defined(NDEBUG)
	printf(" (Debug build)");
#endif

	return EXIT_SUCCESS;
}

static int help()
{
	printf("ooreg - The Omega Online registry editor.\n\n");
	printf("Please consult the documentation at http://www.omegaonline.org.uk for further information.\n\n");
	printf("Usage: ooreg [options] <mode> <arguments> ... <arguments> \n");
	printf("\nOptions:\n");
	printf("  --help (-h)              Display this help text\n");
	printf("  --version (-v)           Display version information\n");
	printf("  --args=<args>            Pass <args> to Omega::Initialize\n");
	printf("\nMode, one of:\n");
	printf("  get <value_path>         Display the value of <value_path>\n");
	printf("  set <value_path> <value> Set the value of <value_path> to <value>\n");
	printf("  delete <key_path>        Delete the key <key_path> and all sub-items\n");
	printf("  delete <value_path>      Delete the value <value_path>\n");
	printf("  list <key_path>          List the subkeys and values of the key <key_path>\n");
	printf("  exists <key_path>        Exit with code EXIT_SUCCESS if the key <key_path>\n");
	printf("                            exists, EXIT_FAILURE if not\n");
	printf("  exists <value_path>      Exit with code EXIT_SUCCESS if the value <value_path>\n");
	printf("	                        exists, EXIT_FAILURE if not\n");
	printf("\nArguments:\n");
	printf("  <key_path>     The full path to a key, separated by /, and ending with /,\n");
	printf("                  e.g. \"All Users/Objects/\"\n");
	printf("  <value_path>   The full path to the value of a key, not ending with /\n");

	return EXIT_SUCCESS;
}

static bool key_path(const OOBase::String& str, Omega::string_t& key)
{
	if (str.empty() || str[str.length()-1] != '/')
		return false;

	key = Omega::string_t(str.c_str(),false,str.length()-1);
	return true;
}

static bool value_path(const OOBase::String& str, Omega::string_t& key, Omega::string_t& value)
{
	if (str.empty() || str[str.length()-1] == '/')
		return false;

	const char* r = strrchr(str.c_str(),'/');
	if (r)
	{
		key = Omega::string_t(str.c_str(),false,r-str.c_str());
		value = Omega::string_t(r+1,false);
	}
	else
	{
		key.Clear();
		value = Omega::string_t(str.c_str(),false);
	}

	return true;
}

int main(int argc, char* argv[])
{
	// Set up the command line args
	OOBase::CmdArgs cmd_args;
	cmd_args.add_option("help",'h');
	cmd_args.add_option("version",'v');
	cmd_args.add_option("args",0,true);

	// Parse command line
	OOBase::CmdArgs::results_t args;
	int err = cmd_args.parse(argc,argv,args);
	if (err	!= 0)
	{
		OOBase::String strErr;
		if (args.find("missing",strErr))
		{
			fputs("Missing value for option ",stderr);
			fputs(strErr.c_str(),stderr);
		}
		else if (args.find("unknown",strErr))
		{
			fputs("Unknown option ",stderr);
			fputs(strErr.c_str(),stderr);
		}
		else
		{
			fputs("Failed to parse comand line: ",stderr);
			fputs(OOBase::system_error_text(err),stderr);
		}
		return EXIT_FAILURE;
	}

	if (args.find("help") != args.npos)
		return help();

	if (args.find("version") != args.npos)
		return version();

	OOBase::String oo_args;
	args.find("args",oo_args);

	OOBase::String method;
	if (!args.find("@0",method))
	{
		fputs("Mode expected, use --help for information.",stderr);
		return EXIT_FAILURE;
	}

	OOBase::String params[2];
	if (!args.find("@1",params[0]))
	{
		fputs("Too few arguments to '",stderr);
		fputs(method.c_str(),stderr);
		fputs("', use --help for information.",stderr);
		return EXIT_FAILURE;
	}
	else if (args.find("@3") != args.npos)
	{
		fputs("Too many arguments to '",stderr);
		fputs(method.c_str(),stderr);
		fputs("', use --help for information.",stderr);
		return EXIT_FAILURE;
	}
	args.find("@2",params[1]);

	Omega::IException* pE = Omega::Initialize(Omega::string_t(oo_args.c_str(),false));
	if (pE)
	{
		report_exception(pE);
		return EXIT_FAILURE;
	}

	int result = EXIT_FAILURE;
	try
	{
		Omega::string_t key,value;

		if (method == "set")
		{
			if (!value_path(params[0],key,value))
				fputs("set requires a value_path, use --help for information.",stderr);
			else
			{
				OTL::ObjectPtr<Omega::Registry::IKey>(key)->SetValue(value,Omega::string_t(params[1].c_str(),false));
				result = EXIT_SUCCESS;
			}
		}
		else if (!params[1].empty())
		{
			fputs("Too many arguments to '",stderr);
			fputs(method.c_str(),stderr);
			fputs("', use --help for information.",stderr);
		}
		else if (method == "get")
		{
			if (!value_path(params[0],key,value))
				fputs("get requires a value_path, use --help for information.",stderr);
			else
			{
				fputs(OTL::ObjectPtr<Omega::Registry::IKey>(key)->GetValue(value).cast<Omega::string_t>().c_nstr(),stdout);
				result = EXIT_SUCCESS;
			}
		}
		else if (method == "delete")
		{
			if (key_path(params[0],key))
			{
				OTL::ObjectPtr<Omega::Registry::IKey>(L"")->DeleteKey(key);
				result = EXIT_SUCCESS;
			}
			if (value_path(params[0],key,value))
			{
				OTL::ObjectPtr<Omega::Registry::IKey>(key)->DeleteValue(value);
				result = EXIT_SUCCESS;
			}
			else
				fputs("delete requires a key_path or a value_path, use --help for information.",stderr);
		}
		else if (method == "exists")
		{
			if (key_path(params[0],key))
				result = (OTL::ObjectPtr<Omega::Registry::IKey>(L"")->IsSubKey(key) ? EXIT_SUCCESS : EXIT_FAILURE);
			if (value_path(params[0],key,value))
				result = (OTL::ObjectPtr<Omega::Registry::IKey>(key)->IsValue(value) ? EXIT_SUCCESS : EXIT_FAILURE);
			else
				fputs("exists requires a key_path or a value_path, use --help for information.",stderr);
		}
		else if (method == "list")
		{
			if (!key_path(params[0],key))
				fputs("list requires a key_path, use --help for information.",stderr);
			else
			{
				OTL::ObjectPtr<Omega::Registry::IKey> ptrKey(key);
				if (ptrKey)
				{
					std::set<Omega::string_t> v = ptrKey->EnumSubKeys();
					for (std::set<Omega::string_t>::const_iterator i=v.begin();i!=v.end();++i)
					{
						if (i != v.begin())
							fputs("\n",stdout);
						fputs(i->c_nstr(),stdout);
						fputs("/",stdout);
					}
					v = ptrKey->EnumValues();
					for (std::set<Omega::string_t>::const_iterator i=v.begin();i!=v.end();++i)
					{
						if (i != v.begin())
							fputs("\n",stdout);
						fputs(i->c_nstr(),stdout);
					}
					result = EXIT_SUCCESS;
				}
			}
		}
		else
		{
			fputs("Unknown mode '",stderr);
			fputs(method.c_str(),stderr);
			fputs("', use --help for information.",stderr);
		}
	}
	catch (Omega::IException* pE)
	{
		report_exception(pE);
	}

	Omega::Uninitialize();

	return result;
}
