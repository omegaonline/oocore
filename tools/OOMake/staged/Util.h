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

#ifndef MAKE_UTIL_H_INCLUDED_
#define MAKE_UTIL_H_INCLUDED_

std::string ChoosePlatform();
std::string ChooseArchitecture();

#if defined(_WIN32)
#include <direct.h>
#include <windows.h>
typedef FILETIME timespec;
#define popen _popen
#define pclose _pclose
#define chdir _chdir
#define getcwd _getcwd
#define unlink _unlink

std::string path_to_local(const std::string& path);
std::string path_from_local(const std::string& path);

#else
#include <dirent.h>
#include <sys/stat.h>
int _mkdir(const char*);

#define path_to_local(x) x
#define path_from_local(x) x

#endif

namespace std
{
	template <> struct less<timespec> : public std::binary_function<timespec,timespec,bool>
	{
		bool operator()(const timespec& t1, const timespec& t2)
		{
#ifdef _WIN32
			if (t1.dwHighDateTime < t2.dwHighDateTime || (t1.dwHighDateTime == t2.dwHighDateTime && t1.dwLowDateTime < t2.dwLowDateTime))
#else
			if (t1.tv_sec < t2.tv_sec || (t1.tv_sec == t2.tv_sec && t1.tv_nsec < t2.tv_nsec))
#endif
			{
				return true;
			}
			else
				return false;
		}
	};
}

std::string remove_common_prefix(const std::string& str1, const std::string& str2);
std::string canonical_path(const std::string& path);
bool is_absolute_path(const std::string& path);
void force_dir(std::string& dir);
void split_path(const std::string& path, std::string& dir, std::string& fname);
void split_path(const std::string& path, std::list<std::string>& dirs, std::string& fname);
bool make_directories(const std::string& path);

class Cwd
{
public:
	Cwd();
	~Cwd();

	void change(const char* dir);

private:
	Cwd(const Cwd&) {};
	Cwd& operator = (const Cwd&) { return *this; }

#ifdef _WIN32
	char cwd[_MAX_PATH];
#else
    char cwd[PATH_MAX];
#endif
};

bool popen(const char* cmd, std::string& strOutput);
bool get_modtime(const std::string& f, timespec& t);

#endif // MAKE_UTIL_H_INCLUDED_
