///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOSvrBase, the Omega Online Base library.
//
// OOSvrBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOSvrBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOSvrBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "CmdArgs.h"

bool OOSvrBase::CmdArgs::add_option(const char* id, char short_opt, bool has_value, const char* long_opt)
{
	assert(id);
	assert(m_map_args.find(id) == m_map_args.end());

	Option opt;
	opt.m_short_opt = short_opt;
	if (long_opt)
		opt.m_long_opt = long_opt;
	else
		opt.m_long_opt = id;

	opt.m_has_value = has_value;

	try
	{
		m_map_opts.insert(std::multimap<std::string,Option>::value_type(id,opt));;
		return true;
	}
	catch (std::exception& e)
	{
		OOBase_CallCriticalFailure(e.what());
		return false;
	}
}

bool OOSvrBase::CmdArgs::add_argument(const char* id, int position)
{
	assert(id);
	assert(m_map_opts.find(id) == m_map_opts.end());
	assert(m_map_args.find(id) == m_map_args.end());

	try
	{
		m_map_args[id] = position;
		return true;
	}
	catch (std::exception& e)
	{
		OOBase_CallCriticalFailure(e.what());
		return false;
	}
}

bool OOSvrBase::CmdArgs::parse(int argc, char* argv[], std::map<std::string,std::string>& results, int skip) const
{
	bool bEndOfOpts = false;
	int pos = 0;
	for (int i=skip;i<argc;++i)
	{
		if (strcmp(argv[i],"--") == 0)
		{
			// -- Terminator
			bEndOfOpts = true;
			continue;
		}

		if (!bEndOfOpts && argv[i][0]=='-' && argv[i][1] != '\0')
		{
			// Options
			if (argv[i][1] == '-')
			{
				// Long option
				if (!parse_long_option(results,argv,i,argc))
					return false;
			}
			else
			{
				// Short options
				if (!parse_short_options(results,argv,i,argc))
					return false;
			}
		}
		else
		{
			// Arguments
			parse_arg(results,argv[i],pos++);
		}
	}

	return true;
}

bool OOSvrBase::CmdArgs::parse_long_option(std::map<std::string,std::string>& results, char** argv, int& arg, int argc) const
{
	for (std::multimap<std::string,Option>::const_iterator i=m_map_opts.begin();i!=m_map_opts.end();++i)
	{
		std::string value = "true";
		if (i->second.m_long_opt == argv[arg]+2)
		{
			if (i->second.m_has_value)
			{
				if (arg >= argc-1)
				{
					OOBase_CallCriticalFailure((std::string("Missing argument for option ") + argv[arg]).c_str());
					return false;
				}
				value = argv[++arg];
			}

			results[i->first] = value;
			return true;
		}

		if (strncmp(i->second.m_long_opt.c_str(),argv[arg]+2,i->second.m_long_opt.length())==0 && argv[arg][i->second.m_long_opt.length()+2]=='=')
		{
			if (i->second.m_has_value)
				value = &argv[arg][i->second.m_long_opt.length()+3];
			
			results[i->first] = value;
			return true;
		}
	}

	OOBase_CallCriticalFailure((std::string("Unrecognised option ") + argv[arg]).c_str());
	return false;
}

bool OOSvrBase::CmdArgs::parse_short_options(std::map<std::string,std::string>& results, char** argv, int& arg, int argc) const
{
	for (char* c = argv[arg]+1;*c!='\0';++c)
	{
		std::multimap<std::string,Option>::const_iterator i;
		for (i=m_map_opts.begin();i!=m_map_opts.end();++i)
		{
			if (i->second.m_short_opt == *c)
			{
				if (i->second.m_has_value)
				{
					std::string value;
					if (c[1] == '\0')
					{
						// Next arg is the value
						if (arg >= argc-1)
						{
							OOBase_CallCriticalFailure((std::string("Missing argument for option -") + c).c_str());
							return false;
						}
						value = argv[++arg];
					}
					else
						value = &c[1];

					// No more for this arg...
					results[i->first] = value;
					return true;
				}
				else
				{
					results[i->first] = "true";
					break;
				}
			}
		}

		if (i == m_map_opts.end())
		{
			OOBase_CallCriticalFailure((std::string("Unrecognised option -") + c).c_str());
			return false;
		}
	}

	return true;
}

void OOSvrBase::CmdArgs::parse_arg(std::map<std::string,std::string>& results, const char* arg, int position) const
{
	for (std::map<std::string,int>::const_iterator i=m_map_args.begin();i!=m_map_args.end();++i)
	{
		if (position == i->second)
		{
			results[i->first] = arg;
			return;
		}
	}
	
	std::ostringstream ss;
	ss.imbue(std::locale::classic());
	ss << "arg" << position;
	results[ss.str()] = arg;
}
