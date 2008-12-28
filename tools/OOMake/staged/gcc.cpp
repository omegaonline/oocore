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
#include "./Make.h"

class GCC : public Toolchain
{
public:
	GCC() : use_gxx(false), m_static_libs(false) {}
	virtual ~GCC() {}

	virtual bool Detect();
	virtual const ToolEntry* GetTools() { return tools; }
	virtual bool Premake(const CmdOptions& cmd_opts, const MakeConfig* make, const Project* project);

	virtual bool LinkExe(const CmdOptions& cmd_opts, const MakeConfig* make, const Project* project, const std::set<std::string>& setOutputs);
	virtual bool LinkLib(const CmdOptions& cmd_opts, const MakeConfig* make, const Project* project, const std::set<std::string>& setOutputs);
	virtual bool LinkDll(const CmdOptions& cmd_opts, const MakeConfig* make, const Project* project, const std::set<std::string>& setOutputs);

	bool use_gxx;
	std::string m_strPath;
	unsigned int m_version;

	std::string threading_option(const Project* project);
	std::string PIC_option(const Project* project);

private:
	static const ToolEntry tools[];

	bool m_static_libs;

	std::string parse_link_options(const ToolInfo* tool, const Project* project);
	std::string parse_link_libs(const ToolInfo* tool, const Project* project);
	bool Link(const CmdOptions& cmd_opts, const std::string& strArgs, const std::string& strOutput, const Project* project, const std::set<std::string>& setOutputs);
};

struct GccCompilerCmn : public Tool
{
protected:
	GCC*              m_toolchain;
	const MakeConfig* m_make;

	GccCompilerCmn(const MakeConfig* make, FileRef* file, GCC* pgcc, const char* pszTool);

	virtual std::string parse_options(bool bPreprocess, bool bPrecompile) = 0;

	bool pre_make(const CmdOptions& cmd_opts, const ToolInfo* tool);
	std::string parse_options(std::map<std::string,std::string>& options, bool bPreprocess, bool bPrecompile);
	std::string parse_project_options(bool bPreprocess);
	std::string parse_tool(std::list<XMLElement>& elements, bool bPreprocess);

private:
	std::string m_strTool;

	virtual bool get_deps(const CmdOptions& cmd_opts);
	virtual bool build(const CmdOptions& cmd_opts);
};

struct GccCompiler : public GccCompilerCmn
{
	static Tool* create(const MakeConfig* make, FileRef* file, Toolchain* tc);

private:
	GccCompiler(const MakeConfig* make, FileRef* file, GCC* pgcc);

	virtual bool pre_make(const CmdOptions& cmd_opts);
	virtual std::string parse_options(bool bPreprocess, bool bPrecompile);
	std::string parse_options(const ToolInfo* tool, bool bPreprocess, bool bPrecompile);
};

struct GxxCompiler : public GccCompilerCmn
{
	static Tool* create(const MakeConfig* make, FileRef* file, Toolchain* tc);

private:
	GxxCompiler(const MakeConfig* make, FileRef* file, GCC* pgcc);

	virtual bool pre_make(const CmdOptions& cmd_opts);
	virtual std::string parse_options(bool bPreprocess, bool bPrecompile);
	std::string parse_options(const ToolInfo* tool, bool bPreprocess, bool bPrecompile);
};

struct Windres : public Tool
{
	static Tool* create(const MakeConfig* make, FileRef* file, Toolchain* tc);

private:
	GCC*              m_toolchain;
	const MakeConfig* m_make;

	Windres(const MakeConfig* make, FileRef* file, GCC* pgcc);

	std::string parse_options(bool bPreprocess);
	std::string parse_tool(std::list<XMLElement>& elements, bool bPreprocess);
	std::string parse_options(const ToolInfo* tool, bool bPreprocess);
		
	virtual bool pre_make(const CmdOptions& cmd_opts) { return true; }
	virtual bool get_deps(const CmdOptions& cmd_opts);
	virtual bool build(const CmdOptions& cmd_opts);
};

static std::string QuoteQuotes(const std::string& in)
{
	std::string ret;
	for (size_t pos = 0;;)
	{
		size_t pos2 = in.find('"',pos);
		if (pos2 == std::string::npos)
		{
			ret += in.substr(pos);
			break;
		}

		ret += in.substr(pos,(pos2-pos)) + "\\\"";
		pos = pos2 + 1;
	}

	return ret;
}

static void parse_deps(const std::string& strOutput, const std::string& strIntDir, std::list<std::string>& listOutputs, std::list<std::string>& listInputs)
{
	size_t pos = strOutput.find(':');
	if (pos == std::string::npos)
		return;

	listOutputs.push_back(strIntDir + strOutput.substr(0,pos++));

	for (;;)
	{
		// Skip starting whitespace
		pos = strOutput.find_first_not_of(" \t",pos);
		if (pos == std::string::npos || strOutput[pos] == '\n')
			break;

		// Check backslash + newline
		if (strOutput[pos] == '\\' && strOutput[pos+1] == '\n')
		{
			pos += 2;
			continue;
		}

		size_t pos2 = strOutput.find_first_of(" \t\n",pos);
		if (pos2 == std::string::npos)
		{
			std::string p = strOutput.substr(pos);
			listInputs.push_back(canonical_path(path_from_local(p)));
			break;
		}

		std::string p = strOutput.substr(pos,pos2-pos);
		listInputs.push_back(canonical_path(path_from_local(p)));
		pos = pos2 + 1;
	}
}

GccCompilerCmn::GccCompilerCmn(const MakeConfig* make, FileRef* file, GCC* pgcc, const char* pszTool) :
	Tool(file),
	m_toolchain(pgcc),
	m_make(make),
	m_strTool(pszTool)
{
	m_strTool = "gcc -pipe -x " + m_strTool;
}

bool GccCompilerCmn::pre_make(const CmdOptions& cmd_opts, const ToolInfo* tool)
{
	std::map<std::string,std::string>::const_iterator i=tool->options.find("PrecompiledHeader");
	if (i!=tool->options.end())
	{
		m_file->path = path_from_local(i->second);

		std::string dir,fname;
		split_path(m_file->path,dir,fname);
		std::string pch = m_make->GetInterPath() + fname + ".gch";

		// Make sure we do a header compilation
		m_strTool += "-header";

		if (cmd_opts.clean)
		{
			timespec out;
			if (!get_modtime(pch,out))
				return true;

			std::cout << "Cleaning " << pch << std::endl;
			return unlink(pch.c_str()) == 0;
		}
		else
		{
			bool bBuild = cmd_opts.force;

			// Get and check the dependencies
			get_deps(cmd_opts);

			timespec out;
			if (!get_modtime(pch,out))
				bBuild = true;

			for (std::list<std::string>::iterator k=m_listInputs.begin();k!=m_listInputs.end() && !bBuild;++k)
			{
				timespec dep;
				if (!get_modtime(*k,dep))
					bBuild = true;
				else if (std::less<timespec>()(out,dep) || std::less<timespec>()(out,m_file->project.makefile.mod_time))
					bBuild = true;
			}
			
			if (bBuild)
			{
				std::cout << "Precompiling " << fname << std::endl;

				std::string strOptions = parse_options(false,true);
				std::string strFile = canonical_path(m_file->project.makefile.src_dir + m_file->project.path + m_file->path);
				std::string cmd = m_toolchain->m_strPath + m_strTool + strOptions + " -c " + strFile + " -o " + path_to_local(pch);

				if (cmd_opts.verbose)
					std::cout << cmd << std::endl;

				if (system(cmd.c_str()) != EXIT_SUCCESS)
					return false;
			}
		}
	}

	return true;
}

bool GccCompilerCmn::get_deps(const CmdOptions& cmd_opts)
{
	std::string strOptions = parse_options(true,false);

	std::string strFile = canonical_path(m_file->project.makefile.src_dir + m_file->project.path + m_file->path);
	std::string cmd = m_toolchain->m_strPath + m_strTool + strOptions + " -M -MG " + strFile;
	std::string strOutput;
	if (!popen(cmd.c_str(),strOutput))
		return false;

	parse_deps(strOutput,m_make->GetInterPath(),m_listOutputs,m_listInputs);
	return true;
}

bool GccCompilerCmn::build(const CmdOptions& cmd_opts)
{
	if (m_listOutputs.empty())
		return true;

	std::cout << m_file->name << std::endl;

	std::string strOptions = parse_options(false,false);

	if (cmd_opts.very_verbose)
		strOptions = " -v" + strOptions;

	std::string strFile = canonical_path(m_file->project.makefile.src_dir + m_file->project.path + m_file->path);
	std::string cmd = m_toolchain->m_strPath + m_strTool + strOptions + " -c " + strFile + " -o " + *m_listOutputs.begin();

	if (cmd_opts.verbose)
		std::cout << cmd << std::endl;

	return (system(cmd.c_str()) == EXIT_SUCCESS);
}

std::string GccCompilerCmn::parse_options(std::map<std::string,std::string>& options, bool bPreprocess, bool bPrecompile)
{
	std::string opts;
	std::string opts_pre;

	for (std::map<std::string,std::string>::iterator i=options.begin();i!=options.end();)
	{
		bool bOk = false;

		if (i->first == "Optimize" || i->first == "Optimise")
		{
			bOk = true;
			if (i->second == "max" || i->second == "3")
				opts += " -O3";
			else if (i->second == "1")
				opts += " -O1";
			else if (i->second == "2" || ParseBool_Raw(i->second))
				opts += " -O2";
			else if (ParseNotBool_Raw(i->second))
				opts += " -O0";
			else
				bOk = false;
		}
		else if (i->first == "Warnings")
		{
			bOk = true;
			if (i->second == "max" || i->second == "2" || ParseBool_Raw(i->second))
				opts += " -Wall";
			else if (i->second == "pedantic" || i->second == "3")
				opts += " -Wall -pedantic";
			else if (i->second == "extra" || i->second == "4")
				opts += " -Wall -pedantic -Wextra";
			else if (i->second == "none" || ParseNotBool_Raw(i->second))
				opts += " -w";
			else
				bOk = false;
		}
		else if (i->first == "DebugSymbols")
		{
			bOk = true;
			if (ParseBool(i->second))
				opts += " -g";
			else if (!ParseNotBool_Raw(i->second))
				bOk = false;
		}
		else if (i->first == "PrecompiledHeader")
		{
			bOk = true;

			std::string dir,fname;
			split_path(path_from_local(i->second),dir,fname);
			
			if (!bPrecompile)
				opts += " -Winvalid-pch -include " + path_from_local(m_make->GetInterPath() + fname);
		}
		else if (i->first == "Visibility")
		{
			bOk = true;
			if (m_toolchain->m_version >= 40000)
			{
				// Add extras here...
				if (m_file->project.makefile.target.platform != "mingw32" &&
					m_file->project.makefile.target.platform != "win32")
				{
					opts += " -fvisibility=" + i->second;
				}
			}
		}

		if (bOk)
			options.erase(i++);
		else
			++i;
	}

	if (!bPreprocess)
		return opts_pre + opts;
	else
		return opts_pre;
}

std::string GccCompilerCmn::parse_project_options(bool bPreprocess)
{
	std::string opts_pre,opts;
	if (m_file->project.type == "SharedLibrary")
		opts += m_toolchain->PIC_option(&m_file->project);

	for (std::map<std::string,std::string>::const_iterator i=m_file->project.options.begin();i!=m_file->project.options.end();++i)
	{
		if (i->first == "Multithreaded" && !ParseNotBool_Raw(i->second))
			opts += m_toolchain->threading_option(&m_file->project);
	}

	if (!bPreprocess)
		return opts_pre + opts;
	else
		return opts_pre;
}

std::string GccCompilerCmn::parse_tool(std::list<XMLElement>& elements, bool bPreprocess)
{
	std::set<std::string> setIncludes;

	std::string opts,opts_pre;
	for (std::list<XMLElement>::iterator i=elements.begin();i!=elements.end();)
	{
		bool bOk = false;

		if (i->strName == "Define")
		{
			bOk = true;
			opts_pre += " -D" + QuoteQuotes(ExpandVars(i->strContent,*m_file));
		}
		else if (i->strName == "Include")
		{
			bOk = true;

			std::map<std::string,std::string>::iterator j = i->mapAttribs.find("Find");
			if (j != i->mapAttribs.end())
			{
				// Search for a file...
				std::string inc = ExpandVars(j->second,*m_file);
				if (inc.empty())
					throw std::string("Error: <Include> elements require a non-empty Find attribute");

				std::string dir = path_from_local(ExpandVars(i->mapAttribs["Directory"],*m_file));

				std::list<std::string> listDirs;
				if (!dir.empty())
				{
					if (!is_absolute_path(dir))
						dir = m_file->project.makefile.src_dir + m_file->project.path + dir;

					force_dir(dir);
					listDirs.push_back(path_to_local(canonical_path(dir)));
				}

#ifndef _WIN32
				listDirs.push_back("/usr/include/");
				listDirs.push_back("/usr/local/include/");
#endif

				std::string found = find_file_in_dirs(inc,listDirs);
				if (found.empty())
				{
					if (!ParseBool_Raw(i->mapAttribs["Optional"]))
						throw std::string("Error: Failed to find include file ") + inc;
				}
				else 
				{
					if (!i->strContent.empty())
						opts_pre += " -D" + QuoteQuotes(ExpandVars(i->strContent,*m_file));
					
					setIncludes.insert(path_from_local(found));
				}
			}
			else
			{
				j = i->mapAttribs.find("Directory");
				if (j == i->mapAttribs.end())
					throw std::string("Error: <Include> element must supply either a Find or a Directory attribute");

				std::string dir = path_from_local(ExpandVars(j->second,*m_file));
				if (!dir.empty())
				{
					if (!is_absolute_path(dir))
						dir = m_file->project.makefile.src_dir + m_file->project.path + dir;

					force_dir(dir);
					setIncludes.insert(canonical_path(dir));
				}
			}
		}
		
		if (bOk)
			elements.erase(i++);
		else
			++i;
	}

	for (std::set<std::string>::iterator i=setIncludes.begin();i!=setIncludes.end();++i)
	{
		opts_pre += " -I" + *i;
	}

	if (!bPreprocess)
		return opts_pre + opts;
	else
		return opts_pre;
}

GccCompiler::GccCompiler(const MakeConfig* make, FileRef* file, GCC* pgcc) :
	GccCompilerCmn(make,file,pgcc,"c")
{
}

bool GccCompiler::pre_make(const CmdOptions& cmd_opts)
{
	std::map<stringpair,ToolInfo*>::const_iterator i = m_file->tools.find(stringpair("CCompiler",std::string()));
	if (i != m_file->tools.end())
	{
		if (!GccCompilerCmn::pre_make(cmd_opts,i->second))
			return false;
	}

	i = m_file->tools.find(stringpair("CCompiler","gcc"));
	if (i != m_file->tools.end())
	{
		if (!GccCompilerCmn::pre_make(cmd_opts,i->second))
			return false;
	}

	return true;
}

std::string GccCompiler::parse_options(const ToolInfo* tool, bool bPreprocess, bool bPrecompile)
{
	std::list<XMLElement> elements(tool->elements);
	std::string opts,opts_pre = GccCompilerCmn::parse_tool(elements,bPreprocess);

	for (std::list<XMLElement>::const_iterator i=elements.begin();i!=elements.end();++i)
		std::cout << "Unrecognised child element <" << i->strName << ">" << std::endl;

	// Check the options...
	std::map<std::string,std::string> options(tool->options);
	opts_pre += GccCompilerCmn::parse_options(options,bPreprocess,bPrecompile);

	for (std::map<std::string,std::string>::const_iterator i=options.begin();i!=options.end();++i)
	{
		bool bOk = false;

		// More C only options here...

		if (!bOk)
			std::cout << "Unrecognised option: " << i->first << "=" << i->second << std::endl;
	}

	if (!bPreprocess)
		return opts_pre + opts;
	else
		return opts_pre;
}

std::string GccCompiler::parse_options(bool bPreprocess, bool bPrecompile)
{
	std::string opts = parse_project_options(bPreprocess);

	// C project options here

	std::map<stringpair,ToolInfo*>::const_iterator i = m_file->tools.find(stringpair("CCompiler",std::string()));
	if (i != m_file->tools.end())
		opts += parse_options(i->second,bPreprocess,bPrecompile);

	i = m_file->tools.find(stringpair("CCompiler","gcc"));
	if (i != m_file->tools.end())
		opts += parse_options(i->second,bPreprocess,bPrecompile);

	return opts;
}

Tool* GccCompiler::create(const MakeConfig* make, FileRef* file, Toolchain* tc)
{
	return new GccCompiler(make,file,static_cast<GCC*>(tc));
}

GxxCompiler::GxxCompiler(const MakeConfig* make, FileRef* file, GCC* pgcc) :
	GccCompilerCmn(make,file,pgcc,"c++")
{
}

std::string GxxCompiler::parse_options(const ToolInfo* tool, bool bPreprocess, bool bPrecompile)
{
	std::list<XMLElement> elements(tool->elements);
	std::string opts,opts_pre = GccCompilerCmn::parse_tool(elements,bPreprocess);

	// Check the options...
	std::map<std::string,std::string> options(tool->options);
	opts_pre += GccCompilerCmn::parse_options(options,bPreprocess,bPrecompile);
	for (std::map<std::string,std::string>::const_iterator i=options.begin();i!=options.end();++i)
	{
		bool bOk = false;
		if (i->first == "UseExceptions")
		{
			// NOP
			bOk = true;
		}
		else if (i->first == "UseRtti")
		{
			bOk = true;
			if (!ParseBool(i->second))
				opts += " -fno-rtti";
		}

		if (!bOk)
			std::cout << "Unrecognised option: " << i->first << " = " << i->second << std::endl;
	}

	if (!bPreprocess)
		return opts_pre + opts;
	else
		return opts_pre;
}

bool GxxCompiler::pre_make(const CmdOptions& cmd_opts)
{
	std::map<stringpair,ToolInfo*>::const_iterator i = m_file->tools.find(stringpair("C++Compiler",std::string()));
	if (i != m_file->tools.end())
	{
		if (!GccCompilerCmn::pre_make(cmd_opts,i->second))
			return false;
	}

	i = m_file->tools.find(stringpair("C++Compiler","gcc"));
	if (i != m_file->tools.end())
	{
		if (!GccCompilerCmn::pre_make(cmd_opts,i->second))
			return false;
	}

	return true;
}

std::string GxxCompiler::parse_options(bool bPreprocess, bool bPrecompile)
{
	std::string opts = parse_project_options(bPreprocess);

	// C++ project options here

	std::map<stringpair,ToolInfo*>::const_iterator i = m_file->tools.find(stringpair("C++Compiler",std::string()));
	if (i != m_file->tools.end())
		opts += parse_options(i->second,bPreprocess,bPrecompile);

	i = m_file->tools.find(stringpair("C++Compiler","gcc"));
	if (i != m_file->tools.end())
		opts += parse_options(i->second,bPreprocess,bPrecompile);

	return opts;
}

Tool* GxxCompiler::create(const MakeConfig* make, FileRef* file, Toolchain* tc)
{
	static_cast<GCC*>(tc)->use_gxx = true;
	return new GxxCompiler(make,file,static_cast<GCC*>(tc));
}

Windres::Windres(const MakeConfig* make, FileRef* file, GCC* pgcc) :
	Tool(file),
	m_toolchain(pgcc),
	m_make(make)
{
}

std::string Windres::parse_tool(std::list<XMLElement>& elements, bool bPreprocess)
{
	std::set<std::string> setIncludes;

	std::string opts,opts_pre;
	for (std::list<XMLElement>::iterator i=elements.begin();i!=elements.end();)
	{
		bool bOk = false;

		if (i->strName == "Define")
		{
			bOk = true;
			opts_pre += " -D" + QuoteQuotes(ExpandVars(i->strContent,*m_file));
		}
		else if (i->strName == "Include")
		{
			bOk = true;

			std::map<std::string,std::string>::iterator j = i->mapAttribs.find("Find");
			if (j != i->mapAttribs.end())
			{
				// Search for a file...
				std::string inc = ExpandVars(j->second,*m_file);
				if (inc.empty())
					throw std::string("Error: <Include> elements require a non-empty Find attribute");

				std::string dir = path_from_local(ExpandVars(i->mapAttribs["Directory"],*m_file));

				std::list<std::string> listDirs;
				if (!dir.empty())
				{
					if (!is_absolute_path(dir))
						dir = m_file->project.makefile.src_dir + m_file->project.path + dir;

					force_dir(dir);
					listDirs.push_back(path_to_local(canonical_path(dir)));
				}

				std::string found = find_file_in_dirs(inc,listDirs);
				if (found.empty())
				{
					if (!ParseBool_Raw(i->mapAttribs["Optional"]))
						throw std::string("Error: Failed to find include file ") + inc;
				}
				else 
				{
					if (!i->strContent.empty())
						opts_pre += " -D" + QuoteQuotes(ExpandVars(i->strContent,*m_file));
					
					setIncludes.insert(path_from_local(found));
				}
			}
			else
			{
				j = i->mapAttribs.find("Directory");
				if (j == i->mapAttribs.end())
					throw std::string("Error: <Include> element must supply either a Find or a Directory attribute");

				std::string dir = path_from_local(ExpandVars(j->second,*m_file));
				if (!dir.empty())
				{
					if (!is_absolute_path(dir))
						dir = m_file->project.makefile.src_dir + m_file->project.path + dir;

					force_dir(dir);
					setIncludes.insert(canonical_path(dir));
				}
			}
		}
		
		if (bOk)
			elements.erase(i++);
		else
			++i;
	}

	for (std::set<std::string>::iterator i=setIncludes.begin();i!=setIncludes.end();++i)
	{
		opts_pre += " -I" + *i;
	}

	if (!bPreprocess)
		return opts_pre + opts;
	else
		return opts_pre;
}

std::string Windres::parse_options(bool bPreprocess)
{
	std::string dir,fname;
	split_path(m_file->project.makefile.src_dir + m_file->project.path + m_file->path,dir,fname);
	std::string opts = " -I " + dir;

	// RC project options here

	std::map<stringpair,ToolInfo*>::const_iterator i = m_file->tools.find(stringpair("RCCompiler",std::string()));
	if (i != m_file->tools.end())
		opts += parse_options(i->second,bPreprocess);

	i = m_file->tools.find(stringpair("RCCompiler","gcc"));
	if (i != m_file->tools.end())
		opts += parse_options(i->second,bPreprocess);

	return opts;
}

std::string Windres::parse_options(const ToolInfo* tool, bool bPreprocess)
{
	std::list<XMLElement> elements(tool->elements);
	std::string opts,opts_pre = parse_tool(elements,bPreprocess);

	for (std::list<XMLElement>::const_iterator i=elements.begin();i!=elements.end();++i)
		std::cout << "Unrecognised child element <" << i->strName << ">" << std::endl;

	// Check the options...
	for (std::map<std::string,std::string>::const_iterator i=tool->options.begin();i!=tool->options.end();++i)
	{
		bool bOk = false;

		if (!bOk)
			std::cout << "Unrecognised option: " << i->first << "=" << i->second << std::endl;
	}

	if (!bPreprocess)
		return opts_pre + opts;
	else
		return opts_pre;
}

bool Windres::get_deps(const CmdOptions& cmd_opts)
{
	std::string strOptions = parse_options(true);

	std::string strFile = canonical_path(m_file->project.makefile.src_dir + m_file->project.path + m_file->path);
	std::string cmd = m_toolchain->m_strPath + "gcc -E -xc-header -DRC_INVOKED" + strOptions + " -M -MG " + strFile;
	std::string strOutput;
	if (!popen(cmd.c_str(),strOutput))
		return false;

	parse_deps(strOutput,m_make->GetInterPath(),m_listOutputs,m_listInputs);

	// Fiddle with the outputs...
	m_listOutputs.clear();
	m_listOutputs.push_back(m_make->GetInterPath() + m_file->path + ".o");
	
	return true;
}

bool Windres::build(const CmdOptions& cmd_opts)
{
	if (m_listOutputs.empty())
		return true;

	std::cout << m_file->name << std::endl;

	std::string strOptions = parse_options(false);

	if (cmd_opts.very_verbose)
		strOptions = " -v" + strOptions;

	std::string strFile = canonical_path(m_file->project.makefile.src_dir + m_file->project.path + m_file->path);
	std::string cmd = m_toolchain->m_strPath + "windres " + strOptions + " -i " + strFile + " -o " + *m_listOutputs.begin() + " -O coff";

	if (cmd_opts.verbose)
		std::cout << cmd << std::endl;

	return (system(cmd.c_str()) == EXIT_SUCCESS);
}

Tool* Windres::create(const MakeConfig* make, FileRef* file, Toolchain* tc)
{
	return new Windres(make,file,static_cast<GCC*>(tc));
}

const ToolEntry GCC::tools[] =
{
	{ ".c", &GccCompiler::create },
	{ ".cpp.cxx.cc.cp.CPP.C.c++", &GxxCompiler::create },
	{ ".rc", &Windres::create },
	{ 0, 0 }
};

bool GCC::Detect()
{
#if defined(_WIN32) && defined(_DEBUG)
	// Check if we can use gcc on the cmdline
	find_set fi;
	fi.strFind = "mingw32-gcc.exe";

	std::list<std::string> listDirs;
	listDirs.push_back(".\\");
	listDirs.push_back("C:\\MinGW\\bin\\");

	m_strPath = find_file_in_dirs(fi.strFind,listDirs);
	if (m_strPath.empty())
	{
		std::cout << "Performing full disk search for " << fi.strFind << "..." << std::endl;

		// Find it...
		std::list<find_set> fs;
		fs.push_back(fi);

		search_all_files(fs);

		if (fs.begin()->found.empty())
			return false;

		m_strPath = (*fs.begin()->found.begin());
	}
#endif // _DEBUG

	std::string strCmd = m_strPath + "gcc --version";
	std::string strVersion;
	if (!popen(strCmd.c_str(),strVersion))
		return false;

	size_t pos = strVersion.find(" (GCC) ");
	if (pos == std::string::npos)
	{
		std::cerr << "Found gcc (" << m_strPath << "gcc) returned an unusual version string: " << strVersion << std::endl;
		return false;
	}

	std::istringstream is(strVersion.substr(pos+7));

	unsigned int hi,mid,lo;
	char dot1,dot2;
	is >> hi >> dot1 >> mid >> dot2 >> lo;
	if (is.fail() || dot1!='.' || dot2!='.')
	{
		std::cerr << "Found gcc (" << m_strPath << "gcc) returned an unusual version string: " << strVersion << std::endl;
		return false;
	}

	m_version = (hi * 10000) + (mid * 100) + lo;

	return true;
}

std::string GCC::parse_link_options(const ToolInfo* tool, const Project* project)
{
	std::string opts;

	for (std::map<std::string,std::string>::const_iterator i=tool->options.begin();i!=tool->options.end();++i)
	{
		bool bOk = false;

		if (i->first == "StaticInputs")
		{
			bOk = true;
			if (ParseBool(i->second))
			{
				m_static_libs = true;
				opts += " -static";
			}
		}
		else if (i->first == "ExportLibrary")
		{
			// Picked up elsewhere...
			bOk = true;
		}
		
		if (!bOk)
			std::cout << "Unrecognised option: " << i->first << " = " << i->second << std::endl;
	}

	return opts;
}

std::string GCC::parse_link_libs(const ToolInfo* tool, const Project* project)
{
	std::string opts;
		
	std::list<std::string> listLibPaths;
	std::list<std::string> listLibs;
	for (std::list<XMLElement>::const_iterator i=tool->elements.begin();i!=tool->elements.end();++i)
	{
		bool bOk = false;

		if (i->strName == "Library")
		{
			bOk = true;

			std::map<std::string,std::string>::const_iterator j = i->mapAttribs.find("Name");
			if (j == i->mapAttribs.end())
				throw std::string("Error: <Library> elements require a non-empty Name attribute");

			std::string lib = ExpandVars(j->second,*project);
			if (lib.empty())
				throw std::string("Error: <Library> elements require a non-empty Name attribute");

			listLibs.push_back(lib);

			std::string dir;
			j = i->mapAttribs.find("Directory");
			if (j != i->mapAttribs.end())
				dir = path_from_local(ExpandVars(j->second,*project));

			if (!dir.empty())
			{
				if (!is_absolute_path(dir))
					dir = project->makefile.build_dir + project->path + dir;

				force_dir(dir);

				std::list<std::string> listDirs;
				listDirs.push_back(path_to_local(canonical_path(dir)));

				std::list<std::string> listFind;

				if (project->makefile.target.platform == "mingw32" ||
					project->makefile.target.platform == "win32")
				{
					if (m_static_libs)
					{
						listFind.push_back("lib" + lib + ".a");
						listFind.push_back("lib" + lib + ".dll.a");
					}
					else
					{
						listFind.push_back("lib" + lib + ".dll.a");
						listFind.push_back(lib + ".dll.a");
						listFind.push_back("lib" + lib + ".a");
						listFind.push_back("lib" + lib + ".dll");
						listFind.push_back(lib + ".dll");
					}
				}
				else //if (project->makefile.target.platform == "linux-gnu")
				{
					if (!m_static_libs)
						listFind.push_back("lib" + lib + ".so");

					listFind.push_back("lib" + lib + ".a");
					
					listDirs.push_back("/usr/lib/");
					listDirs.push_back("/usr/local/lib/");
				}
				
				std::string found = find_files_in_dirs(listFind,listDirs);
				if (found.empty())
				{
					j = i->mapAttribs.find("Optional");
					if (j == i->mapAttribs.end() || !ParseBool_Raw(j->second))
						throw std::string("Error: Failed to find library ") + lib;
					else
						listLibs.pop_back();
				}
				else
				{
					// Hack to get static builds to link against the lib*.dll.a
					if (m_static_libs && 
						(project->makefile.target.platform == "mingw32" ||
						project->makefile.target.platform == "win32"))
					{
						timespec out;
						if (!get_modtime(found + "lib" + lib + ".a",out))
						{
							listLibs.pop_back();
							listLibs.push_back(lib + ".dll");
						}
					}

					listLibPaths.push_back(found);
				}
			}
		}

		if (!bOk)
			std::cout << "Unrecognised child element <" << i->strName << ">" << std::endl;
	}

	for (std::list<std::string>::iterator i=listLibPaths.begin();i!=listLibPaths.end();++i)
	{
		opts += " -L" + *i;
	}

	for (std::list<std::string>::iterator i=listLibs.begin();i!=listLibs.end();++i)
	{
		opts += " -l" + *i;
	}

	return opts;
}

bool GCC::Link(const CmdOptions& cmd_opts, const std::string& strArgs, const std::string& strOutput, const Project* project, const std::set<std::string>& setOutputs)
{
	if (cmd_opts.clean)
	{
		timespec out;
		if (!get_modtime(strOutput,out))
			return true;
		
		std::cout << "Cleaning " << strOutput << std::endl;
		return unlink(strOutput.c_str()) == 0;
	}
	else
	{
		std::string cmd = m_strPath;
		if (use_gxx)
			cmd += "g++";
		else
			cmd += "gcc";

		if (cmd_opts.very_verbose)
			cmd += " -v -Wl,-t";

		cmd += strArgs;

		for (std::map<std::string,std::string>::const_iterator j=project->options.begin();j!=project->options.end();++j)
		{
			if (j->first == "Multithreaded" && !ParseNotBool_Raw(j->second))
				cmd += threading_option(project);
		}

		std::map<stringpair,ToolInfo*>::const_iterator i = project->tools.find(stringpair("Linker",std::string()));
		if (i != project->tools.end())
		{
			cmd += parse_link_options(i->second,project);
		}

		i = project->tools.find(stringpair("Linker","gcc"));
		if (i != project->tools.end())
		{
			cmd += parse_link_options(i->second,project);
		}

		cmd += " -o " + strOutput;

		bool bBuild = cmd_opts.force;
		timespec out;
		if (!get_modtime(strOutput,out))
			bBuild = true;

		for (std::set<std::string>::const_iterator j=setOutputs.begin();j!=setOutputs.end();++j)
		{
			timespec dep;
			if (get_modtime(*j,dep))
			{
				size_t pos = j->find_last_of('.');
				if (pos != std::string::npos && j->substr(pos) == ".o")
				{
					if (std::less<timespec>()(out,dep))
						bBuild = true;

					cmd += " " + *j;
				}
			}
		}

		i = project->tools.find(stringpair("Linker",std::string()));
		if (i != project->tools.end())
		{
			cmd += parse_link_libs(i->second,project);
		}

		i = project->tools.find(stringpair("Linker","gcc"));
		if (i != project->tools.end())
		{
			cmd += parse_link_libs(i->second,project);
		}

		bool bSuccess = true;
		if (bBuild)
		{
			std::cout << "Linking..." << std::endl;

			if (cmd_opts.verbose)
				std::cout << cmd << std::endl;

			bSuccess = (system(cmd.c_str()) == EXIT_SUCCESS);
			if (bSuccess)
				std::cout << "Built " << strOutput << std::endl;
		}
		else
			std::cout << strOutput << " is up to date" << std::endl;

		return bSuccess;
	}
}

bool GCC::Premake(const CmdOptions& cmd_opts, const MakeConfig* make, const Project* project)
{
	FileRef fc(*project);
	fc.Merge(*project);
		
	for (const ToolEntry* te = tools;te->suffixes!=0;++te)
	{
		Tool* pTool = te->create_tool(make,&fc,this);
		if (pTool)
		{
			if (!pTool->pre_make(cmd_opts))
				return false;
		}
	}
	
	return true;
}

bool GCC::LinkExe(const CmdOptions& cmd_opts, const MakeConfig* make, const Project* project, const std::set<std::string>& setOutputs)
{
	std::string ext;
	if (project->makefile.target.platform == "mingw32" ||
		project->makefile.target.platform == "win32")
	{
		ext = ".exe";
	}

	return Link(cmd_opts,"",make->GetOutputPath() + make->GetOutputName() + ext,project,setOutputs);
}

bool GCC::LinkLib(const CmdOptions& cmd_opts, const MakeConfig* make, const Project* project, const std::set<std::string>& setOutputs)
{
	throw std::string("Error: Work In Progress!");
}

bool GCC::LinkDll(const CmdOptions& cmd_opts, const MakeConfig* make, const Project* project, const std::set<std::string>& setOutputs)
{
	std::string ext,cmd;
	if (project->makefile.target.platform == "mingw32" ||
		project->makefile.target.platform == "win32")
	{
		ext = ".dll";

		// Check for export lib option...
		std::string strLiba;
		std::map<stringpair,ToolInfo*>::const_iterator i = project->tools.find(stringpair("Linker",std::string()));
		if (i != project->tools.end())
		{
			for (std::map<std::string,std::string>::const_iterator j=i->second->options.begin();j!=i->second->options.end();++j)
			{
				if (j->first == "ExportLibrary")
				{
					if (ParseBool(j->second))
						strLiba = make->GetOutputPath() + "lib" + make->GetOutputName() + ".dll.a";
				}
			}
		}

		i = project->tools.find(stringpair("Linker","gcc"));
		if (i != project->tools.end())
		{
			for (std::map<std::string,std::string>::const_iterator j=i->second->options.begin();j!=i->second->options.end();++j)
			{
				if (j->first == "ExportLibrary")
				{
					if (ParseBool(j->second))
						strLiba = make->GetOutputPath() + "lib" + make->GetOutputName() + ".dll.a";
				}
			}
		}

		if (!strLiba.empty())
		{
			cmd = " -Wl,--out-implib," + strLiba;

			if (cmd_opts.clean)
			{			
				timespec out;
				if (!get_modtime(strLiba,out))
					return true;
				
				std::cout << "Cleaning " << strLiba << std::endl;
				if (unlink(strLiba.c_str()) != 0)
					return false;
			}
		}
	}
	else
	{
		ext = ".so";
	}

	return GCC::Link(cmd_opts,cmd + " -shared",make->GetOutputPath() + "lib" + make->GetOutputName() + ext,project,setOutputs);
}

std::string GCC::threading_option(const Project* project)
{
	if (project->makefile.target.platform == "mingw32" ||
		project->makefile.target.platform == "win32")
		return " -mthreads";
	else if (project->makefile.target.platform == "linux-gnu")
		return " -pthread"; 
	else
		return "";
}

std::string GCC::PIC_option(const Project* project)
{
	if (project->makefile.target.platform == "linux-gnu")
		return " -fPIC"; 
	else
		return "";
}

Toolchain* create_gcc()
{
	return new GCC();
}
