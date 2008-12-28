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

#ifndef OOMAKE_H_INCLUDED_
#define OOMAKE_H_INCLUDED_

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <map>
#include <set>
#include <list>

#if defined(HAVE_CONFIG_H)
#include <autoconf.h>
#endif

#include "./Util.h"
#include "./Xml.h"
#include "./Parse.h"
#include "./Find.h"

struct CmdOptions
{
	bool verbose;
	bool very_verbose;
	bool clean;
	bool force;
};

struct Config
{
	Config(const Project* project) : m_context(project) {}

	virtual ~Config() {}

	virtual bool Prepare(const CmdOptions& cmd_opts) = 0;
	virtual bool PerformAction(const CmdOptions& cmd_opts, const std::string& strAction, const XMLElement& element) = 0;
	virtual bool Complete(const CmdOptions& cmd_opts) = 0;

protected:
	const Project* m_context;
};

template <class T>
std::string ExpandVars(const std::string& str, const T& context)
{
	std::string ret;
	for (size_t pos = 0;;)
	{
		size_t pos2 = str.find("${",pos);
		if (pos2 == std::string::npos)
		{
			ret += str.substr(pos);
			break;
		}
		else if (pos2 > 0 && str[pos2-1] == '$')
		{
			ret += str.substr(pos,pos2-pos + 2);
			pos = pos2 + 2;
		}
		else
		{
			ret += str.substr(pos,pos2-pos);
			pos = pos2;
			pos2 = str.find('}',pos+2);
			if (pos2 == std::string::npos)
				throw std::string("Unterminated ${ in " + str);

			ret += context.expand_var(str.substr(pos,pos2-pos+1));
			pos = pos2 + 1;
		}
	}

	return ret;
}

#endif // OOMAKE_H_INCLUDED_
