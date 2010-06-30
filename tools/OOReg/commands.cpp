///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
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

#include <OOSvrBase/CmdArgs.h>

#include "../../include/Omega/Omega.h"
#include "../../include/OTL/Registry.h"

#include <iostream>
#include <algorithm>

static bool help(int argc, char* argv[], OTL::ObjectPtr<Omega::Registry::IKey>&);

static std::string canonicalise_key(const std::string& strIn, const std::string& strStart)
{
	// Try to work out where key is...
	std::string strKey;
	if (strIn.at(0) == '/')
	{
		// Absolute key...
		strKey = strIn;
	}
	else
	{
		// Relative key
		strKey = strStart + strIn;
	}

	// Now, make key canonical...
	std::vector<std::string> key_parts;
	while (!strKey.empty())
	{
		assert(strKey[0] == '/');

		size_t next = strKey.find("/",1);
		if (next == std::string::npos)
		{
			key_parts.push_back(strKey.substr(1));
			break;
		}

		key_parts.push_back(strKey.substr(1,next-1));
		strKey = strKey.substr(next);
	}

	// Walk the vector, resolving . and ..
	for (std::vector<std::string>::reverse_iterator i=key_parts.rbegin();i!=key_parts.rend();)
	{
		if (*i == "..")
		{
			(i++)->clear();
			if (i!=key_parts.rend())
				(i++)->clear();
		}
		else if (*i == ".")
			(i++)->clear();
		else
			++i;
	}

	// Build absolute key path...
	strKey.clear();
	for (std::vector<std::string>::iterator i=key_parts.begin();i!=key_parts.end();++i)
	{
		if (!i->empty())
			strKey += "/" + *i;
	}

	if (strKey.empty())
		strKey = "/";

	return strKey;
}

static bool chkey(int argc, char* argv[], OTL::ObjectPtr<Omega::Registry::IKey>& ptrKey)
{
	// Set up the command line args
	OOSvrBase::CmdArgs cmd_args;
	cmd_args.add_option("help",'h');
	cmd_args.add_argument("key",0);
		
	// Parse command line
	std::map<std::string,std::string> args;
	if (!cmd_args.parse(argc,argv,args))
		return true;

	if (args.find("help") != args.end())
	{
		std::cout << "Change the current key" << std::endl;
		std::cout << std::endl;
		std::cout << "Usage: " << argv[0] << " <key name>" << std::endl;
		std::cout << "Options:" << std::endl;
		std::cout << "  --help (-h)              Display this help text" << std::endl;

		return true;
	}

	std::map<std::string,std::string>::iterator key = args.find("key");
	if (key == args.end() || key->second.empty())
	{
		std::cout << "Missing key argument";
		return true;
	}

	std::string strKey = canonicalise_key(key->second,ptrKey->GetName().ToNative());

	ptrKey = OTL::ObjectPtr<Omega::Registry::IKey>(Omega::string_t(strKey.c_str(),false));

	return true;
}

static bool list(int argc, char* argv[], OTL::ObjectPtr<Omega::Registry::IKey>& ptrKey)
{
	// Set up the command line args
	OOSvrBase::CmdArgs cmd_args;
	cmd_args.add_option("help",'h');
		
	// Parse command line
	std::map<std::string,std::string> args;
	if (!cmd_args.parse(argc,argv,args))
		return true;

	if (args.find("help") != args.end())
	{
		std::cout << "List the contents of the current key" << std::endl;
		std::cout << std::endl;
		std::cout << "Options:" << std::endl;
		std::cout << "  --help (-h)              Display this help text" << std::endl;

		return true;
	}
	
	std::set<Omega::string_t> keys = ptrKey->EnumSubKeys();
	for (std::set<Omega::string_t>::const_iterator i=keys.begin();i!=keys.end();++i)
	{
		std::cout << "[Key]  " << i->ToNative() << std::endl;
	}

	std::set<Omega::string_t> vals = ptrKey->EnumValues();
	for (std::set<Omega::string_t>::const_iterator i=vals.begin();i!=vals.end();++i)
	{
		std::cout << i->ToNative() << std::endl;
	}

	return true;
}

static bool quit(int argc, char* argv[], OTL::ObjectPtr<Omega::Registry::IKey>&)
{
	// Set up the command line args
	OOSvrBase::CmdArgs cmd_args;
	cmd_args.add_option("help",'h');
		
	// Parse command line
	std::map<std::string,std::string> args;
	if (!cmd_args.parse(argc,argv,args))
		return true;
	
	if (args.find("help") != args.end())
	{
		std::cout << "Exit the program" << std::endl;
		std::cout << std::endl;
		std::cout << "Options:" << std::endl;
		std::cout << "  --help (-h)              Display this help text" << std::endl;

		return true;
	}
	
	return false;
}

struct Command
{
	const char* pszName;
	bool (*pfn)(int argc, char* argv[], OTL::ObjectPtr<Omega::Registry::IKey>& ptrKey);
};

static const Command cmds[] =
{
	{ "chkey", &chkey },
	{ "ck", &chkey },
	{ "help", &help },
	{ "list", &list },
	{ "ls", &list },
	{ "quit", &quit },
	{ 0,0 }
};

bool process_command(const std::vector<std::string>& line_args, OTL::ObjectPtr<Omega::Registry::IKey>& ptrKey);

static bool help(int argc, char* argv[], OTL::ObjectPtr<Omega::Registry::IKey>& ptrKey)
{
	OOSvrBase::CmdArgs cmd_args;
	cmd_args.add_option("help",'h');

	std::map<std::string,std::string> args;
	if (cmd_args.parse(argc,argv,args) && !args.empty())
	{
		if (args.find("help") != args.end())
		{
			std::cout << "Display help on the available commands" << std::endl;
			std::cout << std::endl << "For more information on an individual command, type 'help <command>'" << std::endl;
		}
		else
		{
			for (size_t i=0;;++i)
			{
				std::ostringstream ss;
				ss.imbue(std::locale::classic());
				ss << "$" << i;

				std::map<std::string,std::string>::const_iterator j=args.find(ss.str());
				if (j == args.end())
					break;

				if (i)
					std::cout << std::endl;

				std::vector<std::string> args2;
				args2.push_back(j->second);
				args2.push_back("--help");
				process_command(args2,ptrKey);
			}
		}
	}
	else
	{
		std::cout << "Available commands are:" << std::endl;

		for (const Command* cmd=cmds;cmd->pszName;++cmd)
			std::cout << "   " << cmd->pszName << std::endl;

		std::cout << std::endl << "For more information on an individual command, type 'help <command>'" << std::endl;
	}
	return true;
}

bool process_command(const std::vector<std::string>& line_args, OTL::ObjectPtr<Omega::Registry::IKey>& ptrKey)
{
	// Search for the command
	typedef bool (*PFN)(int,char*[],OTL::ObjectPtr<Omega::Registry::IKey>&);
	PFN pfn = 0;
	for (const Command* cmd=cmds;cmd->pszName;++cmd)
	{
		if (line_args[0] == cmd->pszName)
		{
			pfn = cmd->pfn;
			break;
		}
	}
	
	if (pfn)
	{
		// Build cmd args
		char** argv = new char*[line_args.size()];
		if (!argv)
		{
			std::cerr << "Out of memory";
			return false;
		}

		int argc = 0;
		for (std::vector<std::string>::const_iterator i=line_args.begin();i!=line_args.end();++i)
			argv[argc++] = strdup(i->c_str());
		
		// Call the function
		bool ret = (*pfn)(argc,argv,ptrKey);

		argc = 0;
		for (std::vector<std::string>::const_iterator i=line_args.begin();i!=line_args.end();++i)
			free(argv[argc++]);
		
		delete [] argv;

		return ret;
	}
	
	// Unknown command
	std::cout << line_args[0] << ": Unknown command.  Type 'help' for a list of commands." << std::endl;
	return true;
}
