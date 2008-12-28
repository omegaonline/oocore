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

#ifndef MAKE_XML_H_INCLUDED_
#define MAKE_XML_H_INCLUDED_

struct Tool
{
	virtual ~Tool() 
	{}

	virtual bool pre_make(const CmdOptions& cmd_opts) = 0;
	virtual bool get_deps(const CmdOptions& cmd_opts) = 0;
	virtual bool build(const CmdOptions& cmd_opts) = 0;

	std::list<std::string>& get_inputs() 
	{ return m_listInputs; }

	std::list<std::string>& get_outputs() 
	{ return m_listOutputs; }

	FileRef* m_file;

protected:
	Tool(FileRef* file) : m_file(file) 
	{}

	std::list<std::string> m_listInputs;
	std::list<std::string> m_listOutputs;
};

struct Toolchain;
struct MakeConfig;

struct ToolEntry
{
	const char* suffixes;
	Tool* (*create_tool)(const MakeConfig*,FileRef*,Toolchain*);
};

struct Toolchain
{
	virtual bool Detect() = 0;
	virtual const ToolEntry* GetTools() = 0;
	virtual bool Premake(const CmdOptions& cmd_opts, const MakeConfig* make, const Project* project) = 0;

	virtual bool LinkExe(const CmdOptions& cmd_opts, const MakeConfig* make, const Project* project, const std::set<std::string>& setOutputs) = 0;
	virtual bool LinkLib(const CmdOptions& cmd_opts, const MakeConfig* make, const Project* project, const std::set<std::string>& setOutputs) = 0;
	virtual bool LinkDll(const CmdOptions& cmd_opts, const MakeConfig* make, const Project* project, const std::set<std::string>& setOutputs) = 0;
};

struct MakeConfig : public Config
{
	MakeConfig(const Project* project);
	virtual ~MakeConfig();

	virtual bool Prepare(const CmdOptions& cmd_opts);
	virtual bool PerformAction(const CmdOptions& cmd_opts, const std::string& strAction, const XMLElement& element);
	virtual bool Complete(const CmdOptions& cmd_opts) = 0;

	std::string GetOutputName() const;
	std::string GetOutputPath() const;
	std::string GetInterPath() const;

protected:
	Toolchain*            m_pToolchain;
	std::set<std::string> m_setOutputs;

	bool PerformMake(const CmdOptions& cmd_opts, const XMLElement& element);
	
private:
	MakeConfig(const MakeConfig& rhs) : Config(rhs) {}
	MakeConfig& operator = (const MakeConfig&) { return *this; }

	void CheckToolchain();
	Tool* FindTool(FileRef* file);
	void ParseGroups(const std::map<std::string,std::string>& attribs, std::list<FileRef*>& listFiles);
};

#endif // MAKE_XML_H_INCLUDED_
