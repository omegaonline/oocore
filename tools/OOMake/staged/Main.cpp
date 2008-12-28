///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OOMake, the Omega Online Make application.
//
// OOMake is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOMake is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOMake.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "./Main.h"

struct ConfigEntry
{
	const char* type;
	Config* (*config_fn)(const Project*);
};

Config* create_exe_config(const Project*);
Config* create_lib_config(const Project*);
Config* create_dll_config(const Project*);

ConfigEntry configs[] = 
{
	{ "Program", &create_exe_config },
	{ "SharedLibrary", &create_dll_config },
	{ "StaticLibrary", &create_lib_config },
	{ 0, 0 }
};

static void MergeArgs(Target& context, const std::map<std::string,std::string>& cmd_args)
{
	for (std::map<std::string,std::string>::iterator i=context.arguments.begin();i!=context.arguments.end();++i)
	{
		std::map<std::string,std::string>::const_iterator j = cmd_args.find(i->first);
		if (j != cmd_args.end())
			i->second = j->second;
	}
}

static int ProcessProjects(const CmdOptions& cmd_opts, const Target& context)
{
	// Now we have a full context we can process...
	int res = EXIT_SUCCESS;

	size_t nSucceeded = 0;
	size_t nFailed = 0;

	for (std::list<Project*>::const_iterator i=context.projects.begin();i!=context.projects.end();++i)
	{
		std::cout << std::endl << "----- " << (*i)->name << " " << (*i)->config << " -----" << std::endl;

		Config* pConfig = 0;
		try
		{
			// Find the config
			for (ConfigEntry* pTE = configs;pTE->type != 0;++pTE)
			{
				if ((*i)->type == pTE->type)
				{
					pConfig = pTE->config_fn(*i);
					break;
				}
			}
			if (!pConfig)
			{
				std::ostringstream os;
				os << "Error: Unsupported configuration type " << (*i)->type;
				throw os.str();
			}

			// Prepare
			bool bSuccess = pConfig->Prepare(cmd_opts);
			
			// Now perform actions...
			std::set<std::string> setOutputs;
			for (std::list<ActionInfo>::iterator j=(*i)->actions.begin();bSuccess && j!=(*i)->actions.end();++j)
			{
				bSuccess = pConfig->PerformAction(cmd_opts,j->type,j->element);
			}

			// Now perform the config task...
			if (bSuccess)
				bSuccess = pConfig->Complete(cmd_opts);
			
			if (bSuccess)
				++nSucceeded;
			else
			{
				++nFailed;
				res = EXIT_FAILURE;
			}
		}
		catch (std::string& err)
		{
			std::cerr << err << std::endl;
		}

		delete pConfig;
	}

	std::cout << std::endl << "========== Done: " << nSucceeded << " succeeded, " << nFailed << " failed ==========" << std::endl;

	return res;
}

int main(int argc, char* argv[])
{
	// Set up a default context
	Target target;

	// Parse the command line
	CmdOptions cmd_opts;
	cmd_opts.clean = false;
	cmd_opts.force = false;
	cmd_opts.verbose = false;
	cmd_opts.very_verbose = false;

	const char* pszMakefile = "OOMake.xml";
	const char* pszBuilddir = "";
	std::map<std::string,std::string> arguments;
	for (int i=1;i<argc;++i)
	{
		if (strcmp(argv[i],"-f") == 0)
		{
			// Makefile name
			if (argc == i)
			{
				std::cerr << "-f requires a filename" << std::endl;
				return EXIT_FAILURE;
			}
				
			pszMakefile = argv[++i];
		}
		else if (strcmp(argv[i],"-v") == 0 || strcmp(argv[i],"--verbose") == 0)
			cmd_opts.verbose = true;
		else if (strcmp(argv[i],"-vv") == 0 || strcmp(argv[i],"--very_verbose") == 0)
			cmd_opts.very_verbose = true;
		else if (strcmp(argv[i],"--clean") == 0)
			cmd_opts.clean = true;
		else if (strcmp(argv[i],"--force") == 0)
			cmd_opts.force = true;
		else if (strcmp(argv[i],"--builddir") == 0)
		{
			// Makefile name
			if (argc == i)
			{
				std::cerr << "--builddir requires a directory" << std::endl;
				return EXIT_FAILURE;
			}
				
			pszBuilddir = argv[++i];
		}
		else if (strcmp(argv[i],"-c") == 0 || strcmp(argv[i],"--config") == 0 || strcmp(argv[i],"--configuration") == 0)
		{
			// Makefile name
			if (argc == i)
			{
				std::cerr << "--configuration requires a name" << std::endl;
				return EXIT_FAILURE;
			}
				
			target.config = argv[++i];
		}
		else if (strcmp(argv[i],"-p") == 0 || strcmp(argv[i],"--platform") == 0)
		{
			// Makefile name
			if (argc == i)
			{
				std::cerr << "--platform requires a name" << std::endl;
				return EXIT_FAILURE;
			}
				
			target.platform = argv[++i];
		}
		else if (strcmp(argv[i],"-a") == 0 || strcmp(argv[i],"--arch") == 0 || strcmp(argv[i],"--architecture") == 0)
		{
			// Makefile name
			if (argc == i)
			{
				std::cerr << "--architecture requires a name" << std::endl;
				return EXIT_FAILURE;
			}
				
			target.architecture = argv[++i];
		}
		else if (strncmp(argv[i],"--",2) == 0)
		{
			char* start = argv[i]+2;
			char* p = strchr(start,'=');
			if (!p)
			{
				std::cerr << argv[i] << "requires a value" << std::endl;
				return EXIT_FAILURE;
			}

			arguments[std::string(start,p-start)] = std::string(p+1);
		}
		else
		{
			std::cerr << argv[i] << " is not a valid command line option" << std::endl;
			return EXIT_FAILURE;
		}
	}

	int res = EXIT_SUCCESS;
	try
	{
		// Parse the makefile
		ParseMakefile(pszMakefile,pszBuilddir,target);

		// Merge in the arguments from the command line
		MergeArgs(target,arguments);

		if (cmd_opts.clean)
			std::cout << "===== Cleaning ";
		else
			std::cout << "===== Building ";
				
		std::cout << target.config << (target.config.empty() ? "" : " ") << target.platform << " "  << target.architecture << " =====" << std::endl;
		
		// Now process each projects actions
		res = ProcessProjects(cmd_opts,target);
	}
	catch (std::string& err)
	{
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}

	return res;
}
