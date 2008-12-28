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

#ifndef OOMAKE_PARSE_H_INCLUDED_
#define OOMAKE_PARSE_H_INCLUDED_

struct Target;
struct FileRef;
struct Project;
struct Makefile;

struct ToolInfo
{
	std::string                       type;
	std::string                       name;
	std::map<std::string,std::string> options;
	std::list<XMLElement>             elements;
};

struct GroupInfo
{
	std::string         path;
	std::list<FileRef*> files;

	~GroupInfo();
};

typedef std::pair<std::string,std::string> stringpair;

struct ToolSet
{
	std::map<stringpair,ToolInfo*> tools;
	
	ToolSet() {}

	virtual ~ToolSet();
	void Merge(const ToolSet& base);
	
protected:
	ToolSet(const ToolSet&) {}
	ToolSet& operator = (const ToolSet&) { return *this; }
};

struct ActionInfo
{
	std::string type;
	XMLElement  element;
};

struct Target
{
	std::string                       platform;
	std::string                       architecture;
	std::string                       config;
	std::map<std::string,std::string> arguments;
	std::list<Project*>               projects;
	std::list<Makefile*>              makefiles;

	std::string expand_var(const std::string& str) const;

	~Target();
};

struct Makefile
{
	Target&     target;
	timespec    mod_time;
	std::string src_dir;
	std::string build_dir;
	
	Makefile(Target& t) : target(t)
	{}

	std::string expand_var(const std::string& str) const
	{
		return target.expand_var(str);
	}

private:
	Makefile(const Makefile& rhs) : target(rhs.target) {}
	Makefile& operator = (const Makefile&) { return *this; }
};

struct Project : public ToolSet
{
	const Makefile&                   makefile;
	std::string                       name;
	std::string                       path;
	std::string                       config;
	std::string                       type;
	std::map<std::string,std::string> options;
	std::list<ActionInfo>             actions;
	std::map<std::string,GroupInfo*>  groups;
	
	Project(const Makefile& m) : makefile(m)
	{}

	~Project();

	std::string get_option(const std::string& o) const;
	std::string expand_var(const std::string& str) const;

private:
	Project(const Project& rhs) : makefile(rhs.makefile) {}
	Project& operator = (const Project&) { return *this; }
};

struct FileRef : public ToolSet
{
	std::string    name;
	std::string    path;
	const Project& project;

	FileRef(const Project& pc) : 
		project(pc)
	{}

	std::string expand_var(const std::string& str) const;

private:
	FileRef(const FileRef& rhs) : project(rhs.project) {}
	FileRef& operator = (const FileRef&) { return *this; }
};

void ParseMakefile(const char* pszMakefile, const char* pszBuilddir, Target& context);
bool ParseBool(const std::string& strText);
bool ParseBool_Raw(const std::string& strText);
bool ParseNotBool_Raw(const std::string& strText);

#endif // OOMAKE_PARSE_H_INCLUDED_
