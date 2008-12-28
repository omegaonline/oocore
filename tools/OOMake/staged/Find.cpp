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

struct find_internal
{
	std::string strFind;
	std::map<timespec,std::string> found;
};

#if defined(_WIN32)

static void search_files_inner(const std::string& strDir, std::list<find_internal>& find_i)
{
	for (std::list<find_internal>::iterator i=find_i.begin();i!=find_i.end();++i)
	{
		WIN32_FIND_DATAA fdata = {0};
		HANDLE hFind = FindFirstFileA((strDir + i->strFind).c_str(),&fdata);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (!(fdata.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_TEMPORARY)))
				{
					i->found.insert(std::map<timespec,std::string>::value_type(fdata.ftLastWriteTime,strDir));
				}

			} while (FindNextFileA(hFind,&fdata));

			FindClose(hFind);
		}
	}
}

static void search_files(const std::string& strDir, std::list<find_internal>& find_i)
{
	std::string strSub = strDir;
	strSub += "*";

	WIN32_FIND_DATAA fdata = {0};
	HANDLE hFind = FindFirstFileA(strSub.c_str(),&fdata);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
				!(fdata.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_TEMPORARY)) &&
				strcmp(fdata.cFileName,".") != 0 &&
				strcmp(fdata.cFileName,"..") != 0)
			{
				std::string strSubDir = strDir + fdata.cFileName + "\\";

				search_files_inner(strSubDir,find_i);

				search_files(strSubDir,find_i);
			}

		} while (FindNextFileA(hFind,&fdata));

		FindClose(hFind);
	}
}

static void search_files(const std::string& strDir, std::list<find_set>& fs)
{
	std::list<find_internal> find_i;
	for (std::list<find_set>::iterator i=fs.begin();i!=fs.end();++i)
	{
		find_internal fi;
		fi.strFind = i->strFind;
		find_i.push_back(fi);
	}

	search_files(strDir,find_i);

	for (std::list<find_internal>::iterator i=find_i.begin();i!=find_i.end();++i)
	{
		for (std::list<find_set>::iterator j=fs.begin();j!=fs.end();++j)
		{
			if (j->strFind == i->strFind)
			{
				for (std::map<timespec,std::string>::iterator k = i->found.begin();k!=i->found.end();++k)
					j->found.push_front(k->second);

				break;
			}
		}
	}
}

void search_all_files(std::list<find_set>& fs)
{
	char drives[512] = {0};
	if (GetLogicalDriveStringsA(511,drives))
	{
		for (char* drive=drives;*drive!='\0';)
		{
			if (GetDriveTypeA(drive) == DRIVE_FIXED)
				search_files(drive,fs);

			drive += strlen(drive)+1;
		}
	}
}

std::string find_files_in_dirs(const std::list<std::string>& listFind, const std::list<std::string>& listDirs)
{
	for (std::list<std::string>::const_iterator i=listDirs.begin();i!=listDirs.end();++i)
	{
		for (std::list<std::string>::const_iterator j=listFind.begin();j!=listFind.end();++j)
		{
			if (GetFileAttributesA((*i + *j).c_str()) != INVALID_FILE_ATTRIBUTES)
				return *i;
		}
	}

	return std::string();
}

std::string find_file_in_dirs(const std::string& strFind, const std::list<std::string>& listDirs)
{
	for (std::list<std::string>::const_iterator i=listDirs.begin();i!=listDirs.end();++i)
	{
		if (GetFileAttributesA((*i + strFind).c_str()) != INVALID_FILE_ATTRIBUTES)
			return *i;
	}

	return std::string();
}

#else // !_WIN32

static int sel_dirs(const dirent* d)
{
	if (d->d_type != DT_DIR)
		return 0;

	if (strcmp(d->d_name,".")==0 || strcmp(d->d_name,"..")==0)
		return 0;

	return 1;
}

static int cmp(const void* a, const void* b)
{
	const dirent** ad = (const dirent**)a;
	const dirent** bd = (const dirent**)b;

	if ((*ad)->d_fileno < (*bd)->d_fileno)
		return -1;
	else if ((*ad)->d_fileno > (*bd)->d_fileno)
		return 1;
	else
		return 0;
}

static void enum_dirs(const std::string& strDir, std::list<std::string>& dirs)
{
	dirent** dirents = 0;
	for (int c = scandir(strDir.c_str(),&dirents,&sel_dirs,&cmp);c > 0;--c)
	{
		dirs.push_back(strDir + dirents[c-1]->d_name + "/");
	}
	free(dirents);
}

static void search_files(const std::string& strDir, std::list<find_internal>& find_i)
{
	std::list<std::string> dirs;
	enum_dirs(strDir,dirs);

	for (std::list<std::string>::iterator i=dirs.begin();i!=dirs.end();++i)
	{
		for (std::list<find_internal>::iterator j=find_i.begin();j!=find_i.end();++j)
		{
			struct stat s;
			if (stat((*i + j->strFind).c_str(),&s) == 0)
				j->found[s.st_mtim] = *i;
		}

		search_files(*i,find_i);
	}
}

static void search_files(const std::string& strDir, std::list<find_set>& fs)
{
	std::list<find_internal> find_i;
	for (std::list<find_set>::iterator i=fs.begin();i!=fs.end();++i)
	{
		find_internal fi;
		fi.strFind = i->strFind;
		find_i.push_back(fi);
	}

	search_files(strDir,find_i);

	for (std::list<find_internal>::iterator i=find_i.begin();i!=find_i.end();++i)
	{
		for (std::list<find_set>::iterator j=fs.begin();j!=fs.end();++j)
		{
			if (j->strFind == i->strFind)
			{
				for (std::map<timespec,std::string>::iterator k = i->found.begin();k!=i->found.end();++k)
					j->found.push_front(k->second);

				break;
			}
		}
	}
}

void search_all_files(std::list<find_set>& fs)
{
	return search_files("/",fs);
}

std::string find_files_in_dirs(const std::list<std::string>& listFind, const std::list<std::string>& listDirs)
{
	for (std::list<std::string>::const_iterator i=listDirs.begin();i!=listDirs.end();++i)
	{
		for (std::list<std::string>::const_iterator j=listFind.begin();j!=listFind.end();++j)
		{
			struct stat s;
			if (stat((*i + *j).c_str(),&s) == 0)
				return *i;
		}
	}

	return std::string();
}

std::string find_file_in_dirs(const std::string& strFind, const std::list<std::string>& listDirs)
{
	for (std::list<std::string>::const_iterator i=listDirs.begin();i!=listDirs.end();++i)
	{
		struct stat s;
		if (stat((*i + strFind).c_str(),&s) == 0)
			return *i;
	}

	return std::string();
}

#endif
