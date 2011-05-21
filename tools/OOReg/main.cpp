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
#include "../../include/Omega/OOCore_version.h"
#include "../../include/OTL/Registry.h"

#include <iostream>

#ifdef HAVE_VLD_H
#include <vld.h>
#endif

// Manual externs
typedef std::vector<std::string> vector_string;

extern void report_exception(Omega::IException* pE);
extern bool process_command(const vector_string& line_args, OTL::ObjectPtr<Omega::Registry::IKey>& ptrKey);

#if defined(_WIN32) && !defined(__MINGW32__)
#define APPNAME "OOReg"
#else
#define APPNAME "ooreg"
#endif

static int version()
{
	std::cout << OOCORE_VERSION;
#if defined(OMEGA_DEBUG)
	std::cout << " (Debug build)";
#endif
	std::cout << " " << OMEGA_COMPILER_STRING << std::endl;

	return EXIT_SUCCESS;
}

static int help()
{
	std::cout << APPNAME " - The Omega Online registry editor." << std::endl;
	std::cout << std::endl;
	std::cout << "Please consult the documentation at http://www.omegaonline.org.uk for further information." << std::endl;
	std::cout << std::endl;
	std::cout << "Usage: " APPNAME " [options]" << std::endl;
	std::cout << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "  --help (-h)              Display this help text" << std::endl;
	std::cout << "  --version (-v)           Display version information" << std::endl;
	std::cout << "  --non-interactive (-n)   Do not prompt between commands" << std::endl;

	return EXIT_SUCCESS;
}

static bool parse_args(const std::string& line, vector_string& line_args, bool& bCont)
{
	bCont = false;

	bool bQuote = false;
	bool bAppend = false;
	for (size_t start = 0;start < line.length();)
	{
		size_t pos = line.find_first_of(" \t\"\\",start);
		if (pos == OOSvrBase::CmdArgs::result_t::npos)
		{
			if (bAppend && !line_args.empty())
				line_args.back() += line.substr(start);
			else
				line_args.push_back(line.substr(start));
			return true;
		}

		if (pos != start)
		{
			if (bAppend && !line_args.empty())
				line_args.back() += line.substr(start,pos-start);
			else
				line_args.push_back(line.substr(start,pos-start));
		}

		switch (line[pos])
		{
		case '\\':
			if (line.length() <= pos + 1)
			{
				bCont = true;
				return false;
			}

			switch (line[pos+1])
			{
			case '\"':
			case ' ':
			case '\t':
			case '\\':
				if (bAppend && !line_args.empty())
					line_args.back() += line[pos+1];
				else
					line_args.push_back(line.substr(pos+1,1));
				break;

			default:
				std::cerr << "Unrecognized control code " << line.substr(pos,2) << " at character " << pos+1 << std::endl;
				return false;
			}
			bAppend = true;
			start = pos + 2;
			break;

		case '\"':
			bQuote = !bQuote;
			if (bQuote && pos == start)
				line_args.push_back(std::string());

			bAppend = bQuote;
			start = pos + 1;
			break;

		case ' ':
		case '\t':
		default:
			start = line.find_first_not_of(" \t",pos);
			if (bQuote)
			{
				if (bAppend && !line_args.empty())
					line_args.back() += line.substr(pos,start-pos);
				else
					line_args.push_back(line.substr(pos,start-pos));
			}
			bAppend = bQuote;
			break;
		}
	}

	return !line_args.empty();
}

int main(int argc, char* argv[])
{
	// Set up the command line args
	OOSvrBase::CmdArgs cmd_args;
	cmd_args.add_option("help",'h');
	cmd_args.add_option("version",'v');
	cmd_args.add_option("non-interactive",'n');

	// Parse command line
	OOSvrBase::CmdArgs::results_t args;
	if (!cmd_args.parse(argc,argv,args))
		return EXIT_FAILURE;

	if (args.find("help") != args.end())
		return help();

	if (args.find("version") != args.end())
		return version();

	bool bPrompt = (args.find("non-interactive") == args.end());

	Omega::IException* pE = Omega::Initialize();
	if (pE)
	{
		report_exception(pE);
		return EXIT_FAILURE;
	}

	int result = EXIT_FAILURE;
	try
	{
		// Open root key
		OTL::ObjectPtr<Omega::Registry::IKey> ptrKey(L"/");

		// Now loop processing commands
		vector_string line_args;
		for (bool bCont = false;!std::cin.eof();)
		{
			if (!bCont)
			{
				line_args.clear();

				if (bPrompt)
				{
					std::string s;
					ptrKey->GetName().ToNative(s);
					std::cout << s << "> ";
				}
			}

			// Read a line...
			std::string line;
			std::getline(std::cin,line);

			// Parse it...
			if (parse_args(line,line_args,bCont))
			{
				try
				{
					// Process it...
					if (!process_command(line_args,ptrKey))
					{
						result = EXIT_SUCCESS;
						break;
					}
				}
				catch (Omega::IException* pE)
				{
					report_exception(pE);
				}

				if (bPrompt)
					std::cout << std::endl;
			}
		}
	}
	catch (Omega::IException* pE)
	{
		report_exception(pE);
	}

	Omega::Uninitialize();

	return result;
}

namespace OOBase
{
	// This is the critical failure hook
	void OnCriticalFailure(const char* msg)
	{
		std::cerr << msg << std::endl;

		// Die horribly now!
		abort();
	}
}
