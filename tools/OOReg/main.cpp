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
#include <OOBase/StackAllocator.h>

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
		OOBase::stderr_write("\nCause: ");
		exception_details(pCause);
	}
}

static void report_exception(Omega::IException* pE)
{
	OOBase::stderr_write("Exception: ");
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
		OOBase::stderr_write(pE->GetDescription().c_str());

		Omega::string_t strSource = pE->GetSource();
		if (!strSource.IsEmpty())
		{
			OOBase::stderr_write("\nAt: ");
			OOBase::stderr_write(strSource.c_str());
		}

		report_cause(pE);
		pE->Release();
	}
	catch (Omega::IException* pE)
	{
		OOBase::stderr_write(pE->GetDescription().c_str());

		report_cause(pE);
		pE->Release();
	}
}

static int version()
{
	OOBase::stdout_write("ooreg version " OOCORE_VERSION);

#if !defined(NDEBUG)
	OOBase::stdout_write(" (Debug build)");
#endif

	return EXIT_SUCCESS;
}

static int help()
{
	OOBase::stdout_write(
			"ooreg - The Omega Online registry editor.\n\n"
			"Please consult the documentation at http://www.omegaonline.org.uk for further information.\n\n"
			"Usage: ooreg [options] <mode> <arguments> ... <arguments> \n"
			"\nOptions:\n"
			"  --help (-h)              Display this help text\n"
			"  --version (-v)           Display version information\n"
			"  --args=<args>            Pass <args> to Omega::Initialize\n"
			"\nMode, one of:\n"
			"  get <value_path>         Display the value of <value_path>\n"
			"  set <value_path> <value> Set the value of <value_path> to <value>\n"
			"  delete <key_path>        Delete the key <key_path> and all sub-items\n"
			"  delete <value_path>      Delete the value <value_path>\n"
			"  list <key_path>          List the subkeys and values of the key <key_path>\n"
			"  exists <key_path>        Exit with code EXIT_SUCCESS if the key <key_path>\n"
			"                            exists, EXIT_FAILURE if not\n"
			"  exists <value_path>      Exit with code EXIT_SUCCESS if the value <value_path>\n"
			"	                        exists, EXIT_FAILURE if not\n"
			"\nArguments:\n"
			"  <key_path>     The full path to a key, separated by /, and ending with /,\n"
			"                  e.g. \"All Users/Objects/\"\n"
			"  <value_path>   The full path to the value of a key, not ending with /\n");

	return EXIT_SUCCESS;
}

static bool key_path(const OOBase::LocalString& str, Omega::string_t& key)
{
	if (str == "/")
		return true;
	
	if (str.length() < 2 || str[str.length()-1] != '/')
		return false;

	key = Omega::string_t(str.c_str(),str.length()-1);
	return true;
}

static bool value_path(const OOBase::LocalString& str, Omega::string_t& key, Omega::string_t& value)
{
	if (str.empty() || str[str.length()-1] == '/')
		return false;

	const char* r = strrchr(str.c_str(),'/');
	if (r)
	{
		key = Omega::string_t(str.c_str(),r-str.c_str());
		value = Omega::string_t(r+1);
	}
	else
	{
		key.Clear();
		value = str.c_str();
	}

	return true;
}

int main(int argc, char* argv[])
{
	// Declare a local stack allocator
	OOBase::StackAllocator<1024> allocator;

	// Set up the command line args
	OOBase::CmdArgs cmd_args(allocator);
	cmd_args.add_option("help",'h');
	cmd_args.add_option("version",'v');
	cmd_args.add_option("args",0,true);

	// Parse command line
	OOBase::CmdArgs::results_t args(allocator);
	int err = cmd_args.parse(argc,argv,args);
	if (err	!= 0)
	{
		OOBase::LocalString strErr(allocator);
		if (args.find("missing",strErr))
		{
			OOBase::stderr_write("Missing value for option ");
			OOBase::stderr_write(strErr.c_str());
		}
		else if (args.find("unknown",strErr))
		{
			OOBase::stderr_write("Unknown option ");
			OOBase::stderr_write(strErr.c_str());
		}
		else
		{
			OOBase::stderr_write("Failed to parse comand line: ");
			OOBase::stderr_write(OOBase::system_error_text(err));
		}
		return EXIT_FAILURE;
	}

	if (args.exists("help"))
		return help();

	if (args.exists("version"))
		return version();

	OOBase::LocalString method(allocator);
	if (!args.find("@0",method))
	{
		OOBase::stderr_write("Mode expected, use --help for information.");
		return EXIT_FAILURE;
	}

	OOBase::LocalString param0(allocator),param1(allocator),param2(allocator);
	if (!args.find("@1",param0))
	{
		OOBase::stderr_write("Too few arguments to '");
		OOBase::stderr_write(method.c_str());
		OOBase::stderr_write("', use --help for information.");
		return EXIT_FAILURE;
	}
	else if (args.exists("@3"))
	{
		OOBase::stderr_write("Too many arguments to '");
		OOBase::stderr_write(method.c_str());
		OOBase::stderr_write("', use --help for information.");
		return EXIT_FAILURE;
	}
	args.find("@2",param1);

	int result = EXIT_FAILURE;
	try
	{
		Omega::Initialize();

		Omega::string_t key,value;

		if (method == "set")
		{
			if (!value_path(param0,key,value))
				OOBase::stderr_write("set requires a value_path, use --help for information.");
			else
			{
				OTL::ObjectPtr<Omega::Registry::IKey>(key)->SetValue(value,param1.c_str());
				result = EXIT_SUCCESS;
			}
		}
		else if (!param1.empty())
		{
			OOBase::stderr_write("Too many arguments to '");
			OOBase::stderr_write(method.c_str());
			OOBase::stderr_write("', use --help for information.");
		}
		else if (method == "get")
		{
			if (!value_path(param0,key,value))
				OOBase::stderr_write("get requires a value_path, use --help for information.");
			else
			{
				OOBase::stdout_write(OTL::ObjectPtr<Omega::Registry::IKey>(key)->GetValue(value).cast<Omega::string_t>().c_str());
				result = EXIT_SUCCESS;
			}
		}
		else if (method == "delete")
		{
			if (key_path(param0,key))
			{
				OTL::ObjectPtr<Omega::Registry::IKey>("")->DeleteSubKey(key);
				result = EXIT_SUCCESS;
			}
			if (value_path(param0,key,value))
			{
				OTL::ObjectPtr<Omega::Registry::IKey>(key)->DeleteValue(value);
				result = EXIT_SUCCESS;
			}
			else
				OOBase::stderr_write("delete requires a key_path or a value_path, use --help for information.");
		}
		else if (method == "exists")
		{
			if (key_path(param0,key))
				result = (OTL::ObjectPtr<Omega::Registry::IKey>("")->IsKey(key) ? EXIT_SUCCESS : EXIT_FAILURE);
			if (value_path(param0,key,value))
				result = (OTL::ObjectPtr<Omega::Registry::IKey>(key)->IsValue(value) ? EXIT_SUCCESS : EXIT_FAILURE);
			else
				OOBase::stderr_write("exists requires a key_path or a value_path, use --help for information.");
		}
		else if (method == "list")
		{
			if (!key_path(param0,key))
				OOBase::stderr_write("list requires a key_path, use --help for information.");
			else
			{
				OTL::ObjectPtr<Omega::Registry::IKey> ptrKey(key);
				if (ptrKey)
				{
					Omega::Registry::IKey::string_set_t v = ptrKey->EnumSubKeys();
					for (Omega::Registry::IKey::string_set_t::const_iterator i=v.begin();i!=v.end();++i)
					{
						OOBase::stdout_write(i->c_str());
						OOBase::stdout_write("/\n");
					}
					v = ptrKey->EnumValues();
					for (Omega::Registry::IKey::string_set_t::const_iterator i=v.begin();i!=v.end();++i)
					{
						OOBase::stdout_write(i->c_str());
						OOBase::stdout_write("\n");
					}
					result = EXIT_SUCCESS;
				}
			}
		}
		else
		{
			OOBase::stderr_write("Unknown mode '");
			OOBase::stderr_write(method.c_str());
			OOBase::stderr_write("', use --help for information.");
		}
	}
	catch (Omega::IException* pE2)
	{
		report_exception(pE2);
	}

	Omega::Uninitialize();

	return result;
}
