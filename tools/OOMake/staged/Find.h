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

#ifndef MAKE_FIND_H_INCLUDED_
#define MAKE_FIND_H_INCLUDED_

struct find_set
{
	std::string strFind;
	std::list<std::string> found;
};

void search_all_files(std::list<find_set>& fs);
std::string find_file_in_dirs(const std::string& strFind, const std::list<std::string>& listDirs);
std::string find_files_in_dirs(const std::list<std::string>& listFind, const std::list<std::string>& listDirs);

#endif // MAKE_FIND_H_INCLUDED_
