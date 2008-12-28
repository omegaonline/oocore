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

struct ParserState
{
	FileRef*    file;
	ToolInfo*   tool;
	Project*    project;
	bool        in_config;
	GroupInfo*  group;
	Makefile*   makefile;
};

GroupInfo::~GroupInfo()
{
	for (std::list<FileRef*>::iterator i=files.begin();i!=files.end();++i)
		delete *i;
}

ToolSet::~ToolSet()
{
	for (std::map<stringpair,ToolInfo*>::iterator i=tools.begin();i!=tools.end();++i)
		delete i->second;
}

void ToolSet::Merge(const ToolSet& base)
{
	// Copy the base tools to new_tools
	std::map<stringpair,ToolInfo*> new_tools;
	for (std::map<stringpair,ToolInfo*>::const_iterator i=base.tools.begin();i!=base.tools.end();++i)
		new_tools[i->first] = new ToolInfo(*i->second);
	
	// Now replace/merge any entries in new_tools with entries from this->tools
	for (std::map<stringpair,ToolInfo*>::iterator i=this->tools.begin();i!=this->tools.end();++i)
	{
		std::map<stringpair,ToolInfo*>::iterator j = new_tools.find(i->first);
		if (j == new_tools.end())
		{
			// Add..
			new_tools[i->first] = new ToolInfo(*i->second);
		}
		else
		{
			// Merge...
			for (std::map<std::string,std::string>::const_iterator k = i->second->options.begin();k != i->second->options.end();++k)
				j->second->options[k->first] = k->second;

			j->second->elements.insert(j->second->elements.end(),i->second->elements.begin(),i->second->elements.end());
		}

		// Delete current contents
		delete i->second;
	}

	// Place current tools with new_tools
	this->tools = new_tools;
}

Project::~Project()
{
	for (std::map<std::string,GroupInfo*>::iterator i=groups.begin();i!=groups.end();++i)
		delete i->second;
}

std::string Project::get_option(const std::string& o) const
{
	std::string r;
	std::map<std::string,std::string>::const_iterator i=options.find(o);
	if (i!=options.end())
		r = ExpandVars(i->second,*this);
	return r;
}

std::string Project::expand_var(const std::string& str) const
{
	std::string ret = makefile.target.expand_var(str);
	if (!ret.empty())
		return ret;

	/*if (str.substr(0,17) == "${Project.Option.")
	{
		std::string opt = str.substr(17,str.length()-18);
		return get_option(opt);
	}
	else*/
	{
		std::map<std::string,std::string> vars;
		vars["${Project.Name}"] = name;
		vars["${Project.Type}"] = type;
		vars["${Configuration.Name}"] = config;

		return vars[str];
	}
}

std::string FileRef::expand_var(const std::string& str) const
{
	std::string ret = project.expand_var(str);
	if (!ret.empty())
		return ret;

	std::map<std::string,std::string> vars;
	vars["${File.Name}"] = name;
	
	return vars[str];
}

Target::~Target()
{
	for (std::list<Project*>::iterator i=projects.begin();i!=projects.end();++i)
		delete (*i);

	for (std::list<Makefile*>::iterator i=makefiles.begin();i!=makefiles.end();++i)
		delete (*i);
}

std::string Target::expand_var(const std::string& str) const
{
	std::map<std::string,std::string> vars;
	vars["${Target.Platform}"] = platform;
	vars["${Target.Architecture}"] = architecture;

	for (std::map<std::string,std::string>::const_iterator i=arguments.begin();i!=arguments.end();++i)
		vars["${Arg." + i->first + "}"] = i->second;
	
	return vars[str];
}

bool ParseBool_Raw(const std::string& strText)
{
	if (strText == "true" ||
		strText == "yes" ||
		strText == "1" ||
		strText == "on")
	{
		return true;
	}
	else
		return false;
}

bool ParseNotBool_Raw(const std::string& strText)
{
	if (strText == "false" ||
		strText == "no" ||
		strText == "0" ||
		strText == "off")
	{
		return true;
	}
	else
		return false;
}

bool ParseBool(const std::string& strText)
{
	if (ParseBool_Raw(strText))
	{
		return true;
	}
	else if (ParseNotBool_Raw(strText))
	{
		return false;
	}
	else
	{
		std::cout << "Warning: Expected boolean attribute value, found '" << strText << "', assuming false" << std::endl;
		return false;
	}
}

static void ParseElement(ParserState& state, Makefile& context, const XMLElement& element, std::list<XMLElement>* unknown);
static void ParseElement(ParserState& state, Project& context, const XMLElement& element, std::list<XMLElement>* unknown);
static void ParseElement(ParserState& state, FileRef& context, const XMLElement& element, std::list<XMLElement>* unknown);

static void ParseAction(ParserState& state, Project& context, const XMLElement& element)
{
	if (state.tool)
		throw std::string("Error: <Action> elements may not be children of <Tool> elements");
	
	if (state.group)
		throw std::string("Error: <Action> elements may not be children of <Group> elements");

	ActionInfo action;
	action.element = element;

	std::map<std::string,std::string>::iterator i = action.element.mapAttribs.find("Type");
	if (i == action.element.mapAttribs.end() || i->second.empty())
		throw std::string("Error: <Action> elements require a Type attribute");
	action.type = i->second;

	action.element.mapAttribs.erase(i);
	
	context.actions.push_back(action);
}

static bool ParseArgument(ParserState& state, Makefile& context, const XMLElement& element)
{
	std::map<std::string,std::string>::const_iterator i = element.mapAttribs.find("Name");
	if (i == element.mapAttribs.end())
		throw std::string("Error: <Argument> elements must have a Name attribute");

	context.target.arguments[i->second] = element.strContent;

	if (!element.listElements.empty())
	{
		std::cout << "Warning: Skipping unexpected child elements of <Argument> element" << std::endl;
		return false;
	}

	return true;
}

static void ParseFile(ParserState& state, Project& context, const XMLElement& element)
{
	if (state.tool)
		throw std::string("Error: <File> elements may not be children of <Tool> elements");

	if (!state.group)
		throw std::string("Error: <File> elements must be children of <Group> elements");

	if (state.file)
		throw std::string("Error: Nested <File> element");

	std::map<std::string,std::string>::const_iterator i = element.mapAttribs.find("Name");
	if (i == element.mapAttribs.end() || i->second.empty())
		throw std::string("Error: <File> elements require a Name attribute");

	for (std::list<FileRef*>::const_iterator j=state.group->files.begin();j!=state.group->files.end();++j)
	{
		if ((*j)->name == i->second)
		{
			std::cout << "Skipping <File> element with duplicate Name=\"" << i->second << "\" attribute" << std::endl;
			return;
		}
	}
	
	state.file = new FileRef(context);
	state.file->name = i->second;
	state.file->path = state.group->path;
	if (!state.file->path.empty())
		force_dir(state.file->path);
	state.file->path += i->second;

	state.group->files.push_back(state.file);

	// Parse the children
	for (std::list<XMLElement>::const_iterator i=element.listElements.begin();i!=element.listElements.end();++i)
	{
		ParseElement(state,*state.file,*i,0);
	}

	state.file = NULL;
}

static void ParseGroup(ParserState& state, Project& context, const XMLElement& element)
{
	if (state.tool)
		throw std::string("Error: <Group> elements may not be children of <Tool> elements");

	if (state.group)
		throw std::string("Error: Nested <Group> elements");

	std::map<std::string,std::string>::const_iterator i = element.mapAttribs.find("Name");
	if (i == element.mapAttribs.end() || i->second.empty())
		throw std::string("Error: <Group> elements require a Name attribute");
	
	std::map<std::string,GroupInfo*>::iterator j = context.groups.find(i->second);
	if (j == context.groups.end())
		j = context.groups.insert(std::map<std::string,GroupInfo*>::value_type(i->second,new GroupInfo)).first;
	
	// Set current project
	state.group = j->second;

	i = element.mapAttribs.find("Directory");
	if (i != element.mapAttribs.end())
	{
		std::string dir = path_from_local(i->second);
		if (!state.group->path.empty() && state.group->path != dir)
			throw std::string("Error: <Group> elements cannot change Directory attribute");
	
		state.group->path = dir;
	}

	// Parse the children
	for (std::list<XMLElement>::const_iterator i=element.listElements.begin();i!=element.listElements.end();++i)
	{
		ParseElement(state,context,*i,0);
	}

	state.group = NULL;
}

static void ParseOption(ParserState& state, FileRef& context, const XMLElement& element)
{
	std::map<std::string,std::string>::const_iterator i = element.mapAttribs.find("Name");
	if (i == element.mapAttribs.end())
		throw std::string("Error: <Option> elements must have a Name attribute");

	// See who we are parented by
	if (state.tool)
		state.tool->options[i->second] = element.strContent;
	else
		std::cout << "Warning: Skipping <Option> element nested in unrecognised element" << std::endl;
	
	if (!element.listElements.empty())
		std::cout << "Warning: Skipping unexpected child elements of <Option> element" << std::endl;
}

static void ParseOption(ParserState& state, Project& context, const XMLElement& element)
{
	std::map<std::string,std::string>::const_iterator i = element.mapAttribs.find("Name");
	if (i == element.mapAttribs.end())
		throw std::string("Error: <Option> elements must have a Name attribute");

	// See who we are parented by
	if (state.tool)
		state.tool->options[i->second] = element.strContent;
	else
		context.options[i->second] = element.strContent;
	
	if (!element.listElements.empty())
		std::cout << "Warning: Skipping unexpected child elements of <Option> element" << std::endl;
}

template <class T>
static void ParseChoice(ParserState& state, T& context, const XMLElement& element, std::list<XMLElement>* unknown)
{
	std::map<std::string,std::string>::const_iterator i = element.mapAttribs.find("Of");
	if (i == element.mapAttribs.end())
		throw std::string("Error: <Choice> elements require a Of attribute");

	std::string strEval = context.expand_var(i->second);
		
	std::list<XMLElement>::const_iterator else_case = element.listElements.end();
	std::list<XMLElement>::const_iterator e;
	for (e=element.listElements.begin();e!=element.listElements.end();++e)
	{
		if (e->strName == "Case")
		{
			std::map<std::string,std::string>::const_iterator j = e->mapAttribs.find("Equals");
			if (j == e->mapAttribs.end() || j->second.empty())
				throw std::string("Error: <Case> elements require an Equals attribute");

			if (strEval == j->second)
				break;
		}
		else if (e->strName == "Else")
		{
			else_case = e;
		}
		else
		{
			std::ostringstream os;
			os << "Error: Unexpected element <" << e->strName << "> in <Choice>";
			throw os.str();
		}
	}

	if (e == element.listElements.end())
		e = else_case;

	if (e != element.listElements.end())
	{
		// Parse the children
		for (std::list<XMLElement>::const_iterator i=e->listElements.begin();i!=e->listElements.end();++i)
		{
			ParseElement(state,context,*i,unknown);
		}
	}
}

static void ParseProject(ParserState& state, Makefile& context, const XMLElement& element)
{
	if (state.project)
		throw std::string("Error: Nested <Project> elements");

	std::map<std::string,std::string>::const_iterator i = element.mapAttribs.find("Name");
	if (i == element.mapAttribs.end() || i->second.empty())
		throw std::string("Error: <Project> elements require a Name attribute");

	for (std::list<Project*>::iterator p=context.target.projects.begin();p!=context.target.projects.end();++p)
	{
		if ((*p)->name == i->second)
			throw std::string("Error: Projects cannot be redefined or extended");
	}

	// Create a new project
	state.project = new Project(context);
	state.project->name = i->second;
	
	i = element.mapAttribs.find("Directory");
	if (i != element.mapAttribs.end())
	{
		state.project->path = path_from_local(i->second);
		force_dir(state.project->path);
	}
	
	context.target.projects.push_back(state.project);
		
	// Parse the children
	for (std::list<XMLElement>::const_iterator i=element.listElements.begin();i!=element.listElements.end();++i)
	{
		ParseElement(state,*state.project,*i,0);
	}

	state.project = NULL;
}

static void ParseConfig(ParserState& state, Project& context, const XMLElement& element)
{
	if (state.tool)
		throw std::string("Error: <Configuration> elements may not be children of <Tool> elements");

	if (state.in_config)
		throw std::string("Error: Nested <Configuration> elements");

	if (state.group)
		throw std::string("Error: <Configuration> elements may not be children of <Group> elements");

	// See if we have a selected config already
	if (!context.config.empty())
		return;

	std::map<std::string,std::string>::const_iterator i = element.mapAttribs.find("Name");
	if (i == element.mapAttribs.end() || i->second.empty())
		throw std::string("Error: <Configuration> elements require a Name attribute");
	std::string strName = i->second;

	i = element.mapAttribs.find("Type");
	if (i == element.mapAttribs.end() || i->second.empty())
		throw std::string("Error: <Configuration> elements require a Type attribute");
	std::string strType = i->second;

	bool bDefault = false;
	i = element.mapAttribs.find("Default");
	if (i != element.mapAttribs.end())
		bDefault = ParseBool(i->second);

	// Now try to work out if we want this configuration or not...
	if (strName == context.makefile.target.config || 
		(bDefault && context.makefile.target.config.empty()))
	{
		// This is the default, and nothing else is specified
		context.config = strName;
	}
	else
		return;

	// Set the project config
	context.type = strType;
		
	// Parse the children
	for (std::list<XMLElement>::const_iterator i=element.listElements.begin();i!=element.listElements.end();++i)
	{
		ParseElement(state,context,*i,0);
	}

	state.in_config = false;
}

template <class T>
static void ParseTool(ParserState& state, T& context, const XMLElement& element)
{
	// Get the name and type
	stringpair tn_pair;
	std::map<std::string,std::string>::const_iterator i = element.mapAttribs.find("Name");
	if (i != element.mapAttribs.end())
		tn_pair.second = i->second;

	// Skip any non-matching tools
	ToolInfo* old_tool = state.tool;
	if (state.tool)
	{
		if (tn_pair.second.empty())
			throw std::string("Error: <Tool> elements nested in <Tool> elements must have a Name attribute");
			
		if (!state.tool->name.empty())
			throw std::string("Error: <Tool> element nested in <Tool> element that already specifies Name attribute");

		tn_pair.first = state.tool->type;
	}
	else
	{
		i = element.mapAttribs.find("Type");
		if (i != element.mapAttribs.end())
			tn_pair.first = i->second;

		if (tn_pair.first.empty())
			throw std::string("Error: <Tool> elements not nested in <Tool> elements must have a Type attribute");
	}
		
	std::map<stringpair,ToolInfo*>::iterator j = context.tools.find(tn_pair);
	if (j != context.tools.end())
		state.tool = j->second;
	else
	{
		state.tool = new ToolInfo;
		state.tool->name = tn_pair.second;
		state.tool->type = tn_pair.first;

		context.tools[tn_pair] = state.tool;
	}

	// Parse the children
	for (std::list<XMLElement>::const_iterator i=element.listElements.begin();i!=element.listElements.end();++i)
	{
		ParseElement(state,context,*i,&state.tool->elements);
	}	
	
	state.tool = old_tool;
}

static void ImportMakefile(ParserState& state, Target& context, std::string strMakefile, const char* pszBuilddir)
{
	// Set up the dirs
	std::string mkd,mkfile;
	split_path(path_from_local(strMakefile),mkd,mkfile);
	force_dir(mkd);

	if (state.makefile)
		strMakefile = canonical_path(state.makefile->src_dir + strMakefile);

	// Open the input file
	std::ostringstream os;
	if (strMakefile == "-" && !state.makefile)
		os << std::cin.rdbuf();
	else
	{
		std::ifstream is(strMakefile.c_str());
		os << is.rdbuf();
	}

	if (os.fail() || os.bad())
		throw "Error opening file " + strMakefile;

	Makefile* makefile = new Makefile(context);
	get_modtime(strMakefile,makefile->mod_time);

	makefile->src_dir = mkd;
	if (state.makefile)
	{
		makefile->src_dir = canonical_path(state.makefile->src_dir + makefile->src_dir);
		makefile->build_dir = remove_common_prefix(makefile->src_dir,state.makefile->src_dir);
		makefile->build_dir = canonical_path(state.makefile->build_dir + makefile->build_dir);
	}

	if (pszBuilddir)
		makefile->build_dir = path_from_local(pszBuilddir);
	force_dir(makefile->build_dir);
		
	// Load the dom
	XMLElement dom;
	ParseXMLDOM(os.str(),dom);

	// Check we have an root element
	if (dom.strName != "OOMake")
		throw "Error: Invalid root element <" + dom.strName + ">, expected <OOMake>";

	Makefile* old_makefile = state.makefile;
	state.makefile = makefile;
	context.makefiles.push_back(makefile);

	// Recursively parse the children
	for (std::list<XMLElement>::const_iterator i=dom.listElements.begin();i!=dom.listElements.end();++i)
	{
		ParseElement(state,*state.makefile,*i,0);
	}

	state.makefile = old_makefile;
}

static void ParseImport(ParserState& state, Makefile& context, const XMLElement& element)
{
	if (state.project)
		throw std::string("Error: <Import> is not allowed within <Project> elements");

	std::map<std::string,std::string>::const_iterator i = element.mapAttribs.find("File");
	if (i == element.mapAttribs.end() || i->second.empty())
		throw std::string("Error: <Import> elements require a File attribute");

	ImportMakefile(state,context.target,i->second,0);	
}

static void ParseElement(ParserState& state, Makefile& context, const XMLElement& element, std::list<XMLElement>* unknown)
{
	if (element.strName == "Argument")
		ParseArgument(state,context,element);
	else if (element.strName == "Choice")
		ParseChoice(state,context,element,unknown);
	else if (element.strName == "Import")
		ParseImport(state,context,element);
	else if (element.strName == "Project")
		ParseProject(state,context,element);	
	else
	{
		if (!unknown)
			std::cout << "Warning: Skipping illegal or unknown element <" << element.strName << ">" << std::endl;
		else
			unknown->push_back(element);
	}
}

static void ParseElement(ParserState& state, Project& context, const XMLElement& element, std::list<XMLElement>* unknown)
{
	if (element.strName == "Action")
		ParseAction(state,context,element);
	else if (element.strName == "Choice")
		ParseChoice(state,context,element,unknown);
	else if (element.strName == "File")
		ParseFile(state,context,element);
	else if (element.strName == "Group")
		ParseGroup(state,context,element);
	else if (element.strName == "Option")
		ParseOption(state,context,element);
	else if (element.strName == "Configuration")
		ParseConfig(state,context,element);
	else if (element.strName == "Tool")
		ParseTool(state,context,element);
	else
	{
		if (!unknown)
			std::cout << "Warning: Skipping illegal or unknown element <" << element.strName << ">" << std::endl;
		else
			unknown->push_back(element);
	}
}

static void ParseElement(ParserState& state, FileRef& context, const XMLElement& element, std::list<XMLElement>* unknown)
{
	bool bOk = true;
	if (element.strName == "Choice")
		ParseChoice(state,context,element,unknown);
	else if (element.strName == "Option")
		ParseOption(state,context,element);
	else if (element.strName == "Tool")
		ParseTool(state,context,element);
	else
	{
		if (!unknown)
			std::cout << "Warning: Skipping illegal or unknown element <" << element.strName << ">" << std::endl;
		else
			unknown->push_back(element);
	}
}

void ParseMakefile(const char* pszMakefile, const char* pszBuilddir, Target& context)
{
	ParserState state;
	state.file = NULL;
	state.group = NULL;
	state.in_config = false;
	state.makefile = NULL;
	state.project = NULL;
	state.tool = NULL;

	// Choose the platform and architecture
	if (context.platform.empty())
		context.platform = ChoosePlatform();

	if (context.architecture.empty())
		context.architecture = ChooseArchitecture();
	
	ImportMakefile(state,context,pszMakefile,pszBuilddir);
}
