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

bool popen(const char* cmd, std::string& strOutput)
{
	FILE* file = popen(cmd,"r");
	if (!file)
		return false;

	char szBuf[1024];
	for (;;)
	{
		size_t ret = fread(szBuf,sizeof(char),sizeof(szBuf),file);
		strOutput += std::string(szBuf,ret);
		if (ret	!= sizeof(szBuf))
			break;
	}

	return (pclose(file) == EXIT_SUCCESS);
}

void force_dir(std::string& dir)
{
	if (dir.empty())
		dir = "./";
	else if (dir[dir.length()-1] != '/')
		dir += '/';
}

Cwd::Cwd()
{
	if (!getcwd(cwd,sizeof(cwd)-1))
        throw std::string("Current working directory is too deep to handle");
}

Cwd::~Cwd()
{
	chdir(cwd);
}

void Cwd::change(const char* dir)
{
	chdir(dir);
}

bool make_directories(const std::string& path)
{
	std::list<std::string> dirs;
	std::string fname;
	split_path(path,dirs,fname);

	Cwd cwd;
	for (std::list<std::string>::const_iterator i=dirs.begin();i!=dirs.end();++i)
	{
		if (*i == "../")
			cwd.change("..");
		else
		{
			if (_mkdir(i->c_str()) != 0 && errno != EEXIST)
				return false;

			cwd.change(i->c_str());
		}
	}

	return true;
}

std::string remove_common_prefix(const std::string& str1, const std::string& str2)
{
	std::string str1_fname;
	std::list<std::string> str1_dirs;
	split_path(str1,str1_dirs,str1_fname);

	std::string str2_fname;
	std::list<std::string> str2_dirs;
	split_path(str2,str2_dirs,str2_fname);

	for (std::list<std::string>::const_iterator i=str2_dirs.begin();i!=str2_dirs.end();++i)
	{
		if (*i == str1_dirs.front())
			str1_dirs.pop_front();
		else
			break;
	}

	std::string ret;
	for (std::list<std::string>::const_iterator i=str1_dirs.begin();i!=str1_dirs.end();++i)
	{
		ret += *i;
	}
	force_dir(ret);
	ret += str1_fname;
	return ret;
}

std::string canonical_path(const std::string& path)
{
	// Split path removes .. and . where it can
	std::string fname;
	std::list<std::string> dirs;
	split_path(path,dirs,fname);

	std::string ret;
	for (std::list<std::string>::const_iterator i=dirs.begin();i!=dirs.end();++i)
	{
		ret += *i;
	}
	force_dir(ret);
	ret += fname;
	return ret;
}

void split_path(const std::string& path, std::string& dir, std::string& fname)
{
	size_t pos = path.find_last_of('/');
	if (pos == std::string::npos)
		fname = path;
	else
	{
		dir = path.substr(0,pos+1);
		if (pos < path.length())
			fname = path.substr(pos+1);
	}
}

void split_path(const std::string& path, std::list<std::string>& dirs, std::string& fname)
{
	for (size_t pos = 0;;)
	{
		size_t pos2 = path.find_first_of('/',pos);
		if (pos2 == std::string::npos)
		{
			fname = path.substr(pos);
			break;
		}

		std::string dir = path.substr(pos,pos2-pos+1);
		if (dir != "./")
		{
			if (dir == "../" && !dirs.empty() && dirs.back() != "../")
				dirs.pop_back();
			else
				dirs.push_back(dir);
		}

		pos = pos2 + 1;
	}
}

#if defined(_WIN32)

std::string ChoosePlatform()
{
#if defined(OOMAKE_PLATFORM)
	return OOMAKE_PLATFORM;
#else
	return "win32";
#endif
}

std::string ChooseArchitecture()
{
#if defined(OOMAKE_CPU)
	return OOMAKE_CPU;
#else
	void* TODO; // Use the registry to find this...
	return "i686";
#endif
}

bool is_absolute_path(const std::string& path)
{
	if (path.length() > 2 && path.substr(1,2) == ":/")
		return true;
	else if (path.length() > 0 && path[0] == '/')
		return true;
	else
		return false;
}

std::string path_to_local(const std::string& path)
{
	std::string ret;
	for (size_t pos = 0;;)
	{
		size_t pos2 = path.find('/',pos);
		if (pos2 == std::string::npos)
		{
			ret += path.substr(pos);
			break;
		}

		ret += path.substr(pos,pos2-pos) + '\\';
		pos = pos2 + 1;
	}
	return ret;
}

std::string path_from_local(const std::string& path)
{
	std::string ret;
	for (size_t pos = 0;;)
	{
		size_t pos2 = path.find('\\',pos);
		if (pos2 == std::string::npos)
		{
			ret += path.substr(pos);
			break;
		}

		ret += path.substr(pos,pos2-pos) + '/';
		pos = pos2 + 1;
	}
	return ret;
}

bool get_modtime(const std::string& file, timespec& t)
{
	std::string f = path_to_local(file);

	t.dwHighDateTime = 0;
	t.dwLowDateTime = 0;
	BOOL bRet = 0;

	HANDLE hF = CreateFileA(f.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hF)
	{
		bRet = GetFileTime(hF,NULL,NULL,&t);
		CloseHandle(hF);
	}
	return (bRet != 0);
}

#else // !_WIN32

std::string ChoosePlatform()
{
#if defined(OOMAKE_PLATFORM)
	return OOMAKE_PLATFORM;
#else
	throw std::string("Failed to determine host operating system");
#endif
}

std::string ChooseArchitecture()
{
#if defined(OOMAKE_CPU)
	return OOMAKE_CPU;
#else
	throw std::string("Failed to determine host cpu");
#endif
}

bool is_absolute_path(const std::string& path)
{
	return (path.length() > 0 && path[0] == '/');
}

int _mkdir(const char* dir)
{
    return mkdir(dir,S_IRWXU | S_IRWXG | S_IRWXO);
}

bool get_modtime(const std::string& f, timespec& t)
{
	struct stat s;
	if (stat(f.c_str(),&s) == 0)
	{
		t = s.st_mtim;
		return true;
	}

	t.tv_sec = 0;
	t.tv_nsec = 0;
	return false;
}

#endif
