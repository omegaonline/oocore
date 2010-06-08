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

#ifndef OOSVRBASE_CMDARGS_H_INCLUDED_
#define OOSVRBASE_CMDARGS_H_INCLUDED_

#include "config-base.h"

#include <map>
#include <iostream>

namespace OOSvrBase
{
	class CmdArgs
	{
	public:
		bool add_option(const char* id, char short_opt = 0, bool has_value = false, const char* long_opt = 0);
		bool add_argument(const char* id, int position);

		bool parse(int argc, char* argv[], std::map<std::string,std::string>& results, int skip = 1) const;

	private:
		struct Option
		{
			char        m_short_opt;
			std::string m_long_opt;
			bool        m_has_value;
		};

		mutable std::string               m_name;
		std::multimap<std::string,Option> m_map_opts;
		std::map<std::string,int>         m_map_args;

		bool parse_long_option(std::map<std::string,std::string>& results, char** argv, int& arg, int argc) const;
		bool parse_short_options(std::map<std::string,std::string>& results, char** argv, int& arg, int argc) const;
		void parse_arg(std::map<std::string,std::string>& results, const char* opt, int position) const;
	};
}

#endif // OOSVRBASE_CMDARGS_H_INCLUDED_
