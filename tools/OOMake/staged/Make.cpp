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

struct ToolchainEntry
{
	const char* platform;
	const char* architecture;
	Toolchain* (*toolchain_fn)();
};

Toolchain* create_gcc();

ToolchainEntry toolchains[] =
{
	{ 0, 0, &create_gcc },
	{ 0,0,0 }
};

MakeConfig::MakeConfig(const Project* project) :
	Config(project),
	m_pToolchain(0)
{
	CheckToolchain();
}

MakeConfig::~MakeConfig()
{
	delete m_pToolchain;
}

void MakeConfig::CheckToolchain()
{
	if (!m_pToolchain)
	{
		// Select the correct toolchain
		for (ToolchainEntry* pTC = toolchains;pTC->toolchain_fn != 0;++pTC)
		{
			if ((!pTC->architecture || pTC->architecture == m_context->makefile.target.architecture) &&
				(!pTC->platform || pTC->platform == m_context->makefile.target.platform))
			{
				m_pToolchain = pTC->toolchain_fn();
				if (m_pToolchain->Detect())
					break;

				delete m_pToolchain;
				m_pToolchain = 0;
			}
		}

		if (!m_pToolchain)
		{
			std::ostringstream os;
			os << "Error: Failed to find a suitable toolchain for " << m_context->makefile.target.platform << "-" << m_context->makefile.target.architecture;
			throw os.str();
		}
	}
}

bool MakeConfig::Prepare(const CmdOptions& cmd_opts)
{
	// Creare intermediate dir if needed
	return make_directories(GetInterPath()) &&
		make_directories(GetOutputPath());
}

bool MakeConfig::PerformAction(const CmdOptions& cmd_opts, const std::string& strAction, const XMLElement& element)
{
	if (strAction == "Make")
		return PerformMake(cmd_opts,element);
	else
	{
		std::ostringstream os;
		os << "Error: Unsupported Action type " << strAction;
		throw os.str();
	}
}

Tool* MakeConfig::FindTool(FileRef* file)
{
	size_t pos = file->name.find_last_of('.');
	if (pos == std::string::npos)
		return 0;

	std::string strExt = file->name.substr(pos);

	// Now find strExt
	Tool* pTool = 0;
	for (const ToolEntry* te = m_pToolchain->GetTools();te->suffixes!=0;++te)
	{
		std::set<std::string> exts;
		std::string x(te->suffixes);

		for (pos=0;;)
		{
			size_t pos2 = x.find('.',pos+1);
			if (pos2 == std::string::npos)
			{
				exts.insert(x.substr(pos));
				break;
			}

			exts.insert(x.substr(pos,pos2-pos));
			pos = pos2;
		}

		if (exts.find(strExt) != exts.end())
			pTool = te->create_tool(this,file,m_pToolchain);
	}

	return pTool;
}

struct Item
{
	std::string strFile;
	Tool* tool;
	bool exists;
	bool built;
};

void MakeConfig::ParseGroups(const std::map<std::string,std::string>& attribs, std::list<FileRef*>& listFiles)
{
	std::map<std::string,std::string>::const_iterator i=attribs.find("Groups");
	if (i == attribs.end())
		return;

	std::set<std::string> groups;
	for (const char* rd_ptr = i->second.c_str();;)
	{
		// Skip the starting whitespace
		rd_ptr += strspn(rd_ptr,";" XML_WHITESPACE);

		const char* p = strpbrk(rd_ptr,";" XML_WHITESPACE);
		if (!p)
		{
			groups.insert(rd_ptr);
			break;
		}
		else
		{
			groups.insert(std::string(rd_ptr,p-rd_ptr));
			rd_ptr = p;
		}
	}

	for (std::set<std::string>::iterator i=groups.begin();i!=groups.end();++i)
	{
		std::map<std::string,GroupInfo*>::const_iterator j = m_context->groups.find(*i);
		if (j != m_context->groups.end())
		{
			for (std::list<FileRef*>::iterator k = j->second->files.begin();k != j->second->files.end();++k)
			{
				listFiles.push_back(*k);
			}
		}
	}
}

bool MakeConfig::PerformMake(const CmdOptions& cmd_opts, const XMLElement& element)
{
	// Do any pre-make actions (such as precompiled headers)
	if (!m_pToolchain->Premake(cmd_opts,this,m_context))
		return false;

	// Get our inputs from the groups
	std::list<FileRef*> listFiles;
	ParseGroups(element.mapAttribs,listFiles);

	// Build a list of input units
	bool bFailure = false;
	std::list<Item> listUnits;
	for (std::list<FileRef*>::const_iterator i=listFiles.begin();i!=listFiles.end();++i)
	{
		(*i)->Merge(*m_context);

		Item u;
		u.strFile = canonical_path(m_context->makefile.src_dir + m_context->path + (*i)->path);
		u.built = false;
		u.tool = FindTool(*i);
		if (!u.tool)
		{
			std::ostringstream os;
			os << "Error: No make tool for " << u.strFile;
			throw os.str();
		}

		timespec dep;
		u.exists = get_modtime(u.strFile,dep);
		if (u.exists)
		{
			if (!u.tool->get_deps(cmd_opts))
				bFailure = true;
		}

		listUnits.push_back(u);
	}

	if (cmd_opts.clean)
	{
		for (std::list<Item>::iterator i=listUnits.begin();i!=listUnits.end();++i)
		{
			std::list<std::string>& listOutputs = i->tool->get_outputs();
			
			for (std::list<std::string>::iterator j=listOutputs.begin();j!=listOutputs.end();++j)
			{
				timespec dep;
				if (get_modtime(*j,dep))
				{
					std::cout << "Cleaning " << *j << std::endl;
					if (unlink(j->c_str()) != 0)
						return false;
				}
			}
		}
	}
	else
	{
		// Now loop trying to build
		bool built_one;
		do
		{
			built_one = false;
			for (std::list<Item>::iterator i=listUnits.begin();i!=listUnits.end();++i)
			{
				if (!i->exists)
				{
					timespec dep;
					i->exists = get_modtime(i->strFile,dep);
					if (i->exists)
						i->tool->get_deps(cmd_opts);
				}

				if (i->exists && !i->built)
				{
					bool bMissing = false;
					bool bBuild = cmd_opts.force;

					std::list<std::string>& listOutputs = i->tool->get_outputs();
					std::list<std::string>& listInputs = i->tool->get_inputs();

					for (std::list<std::string>::iterator j=listOutputs.begin();j!=listOutputs.end() && !bMissing && !bBuild;++j)
					{
						timespec out;
						if (!get_modtime(*j,out))
							bBuild = true;

						for (std::list<std::string>::iterator k=listInputs.begin();k!=listInputs.end() && !bMissing && !bBuild;++k)
						{
							timespec dep;
							if (!get_modtime(*k,dep))
								bMissing = true;
							else if (std::less<timespec>()(out,dep) || std::less<timespec>()(out,i->tool->m_file->project.makefile.mod_time))
								bBuild = true;
						}
					}

					if (!bMissing)
					{
						if (bBuild)
						{
							if (!built_one)
								std::cout << "Compiling..." << std::endl;

							if (!i->tool->build(cmd_opts))
								bFailure = true;

							built_one = true;
						}
						i->built = true;
					}
				}
			}
		} while (built_one);

		// Now build the rest just to output errors, update outputs and delete tools
		for (std::list<Item>::iterator i=listUnits.begin();i!=listUnits.end();++i)
		{
			if (!bFailure && !i->built && !i->tool->build(cmd_opts))
				bFailure = true;

			std::list<std::string>& listOutputs = i->tool->get_outputs();
			for (std::list<std::string>::iterator j=listOutputs.begin();j!=listOutputs.end();++j)
				m_setOutputs.insert(*j);

			delete i->tool;
		}
	}

	return !bFailure;
}

std::string MakeConfig::GetInterPath() const
{
	std::string dir = m_context->get_option("IntermediateDirectory");
	if (dir.empty())
		dir = m_context->config;

	dir = m_context->makefile.build_dir + dir;
	force_dir(dir);

	return canonical_path(dir);
}

std::string MakeConfig::GetOutputPath() const
{
	std::string dir = m_context->get_option("OutputDirectory");
	if (dir.empty())
		dir = m_context->get_option("IntermediateDirectory");

	if (dir.empty())
		dir = m_context->config;

	dir = path_from_local(dir);

	dir = m_context->makefile.build_dir + dir;
	force_dir(dir);

	return canonical_path(dir);
}

std::string MakeConfig::GetOutputName() const
{
	std::string out = m_context->get_option("OutputName");
	if (out.empty())
		out = m_context->name;

	return out;
}

struct ExeConfig : public MakeConfig
{
	ExeConfig(const Project* project) : MakeConfig(project)
	{}

	virtual bool Complete(const CmdOptions& cmd_opts);
};

bool ExeConfig::Complete(const CmdOptions& cmd_opts)
{
	return m_pToolchain->LinkExe(cmd_opts,this,m_context,m_setOutputs);
}

Config* create_exe_config(const Project* project)
{
	return new ExeConfig(project);
}

struct LibConfig : public MakeConfig
{
	LibConfig(const Project* project) : MakeConfig(project)
	{}

	virtual bool Complete(const CmdOptions& cmd_opts);
};

bool LibConfig::Complete(const CmdOptions& cmd_opts)
{
	return m_pToolchain->LinkLib(cmd_opts,this,m_context,m_setOutputs);
}

Config* create_lib_config(const Project* project)
{
	return new LibConfig(project);
}

struct DllConfig : public MakeConfig
{
	DllConfig(const Project* project) : MakeConfig(project)
	{}

	virtual bool Complete(const CmdOptions& cmd_opts);
};

bool DllConfig::Complete(const CmdOptions& cmd_opts)
{
	return m_pToolchain->LinkDll(cmd_opts,this,m_context,m_setOutputs);
}

Config* create_dll_config(const Project* project)
{
	return new DllConfig(project);
}
