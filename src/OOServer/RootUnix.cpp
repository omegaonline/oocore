///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"
#include "RootManager.h"

#if defined(HAVE_UNISTD_H)

bool Root::Manager::platform_install(const std::map<std::string,std::string>& /*args*/)
{
	return true;
}

bool Root::Manager::platform_uninstall()
{
	return true;
}

bool Root::Manager::install_sandbox(const std::map<std::string,std::string>& args)
{
	std::string strUName = "omega_sandbox";
	std::map<std::string,std::string>::const_iterator a = args.find("arg0");
	if (a != args.end())
		strUName = a->second;

	OOSvrBase::pw_info pw(strUName.c_str());
	if (!pw)
	{
		if (errno)
			LOG_ERROR_RETURN(("getpwnam(%s) failed: %s",strUName.c_str(),OOSvrBase::Logger::format_error(errno).c_str()),false);
		else
			LOG_ERROR_RETURN(("You must add a user account for 'omega_sandbox' or supply a valid user name on the command line"),false);
	}

	// Set the sandbox uid
	Omega::int64_t key = 0;
	if (m_registry->open_key(0,key,"System\\Server\\Sandbox",0) != 0)
		LOG_ERROR_RETURN(("Failed to open server registry key: %s",OOSvrBase::Logger::format_error(errno).c_str()),false);

	int err = m_registry->set_integer_value(key,"Uid",0,pw->pw_uid);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to set sandbox uid in registry: %s",OOSvrBase::Logger::format_error(errno).c_str()),false);

	return true;
}

bool Root::Manager::uninstall_sandbox()
{
	return true;
}

bool Root::Manager::secure_file(const std::string& strFile, bool bPublicRead)
{
	// Make sure the file is owned by root (0)
	if (chown(strFile.c_str(),0,(gid_t)-1) != 0)
		LOG_ERROR_RETURN(("chown(%s) failed: %s",strFile.c_str(),OOSvrBase::Logger::format_error(errno).c_str()),false);

	if (chmod(strFile.c_str(),S_IRWXU | S_IRGRP) != 0)
		LOG_ERROR_RETURN(("chmod(%s) failed: %s",strFile.c_str(),OOSvrBase::Logger::format_error(errno).c_str()),false);

	return true;
}

bool Root::Manager::get_db_directory(std::string& dir)
{
	dir = "/var/lib/omegaonline";

	if (mkdir(dir.c_str(),S_IRWXU | S_IRGRP) != 0)
		LOG_ERROR_RETURN(("mkdir(%s) failed: %s",dir.c_str(),OOSvrBase::Logger::format_error(errno).c_str()),false);

	return true;
}

void Root::Manager::wait_for_quit()
{
	// Use libev or just a plain signal handler?
	void* TODO;
}

namespace OOBase
{
	// This is the critical failure hook
	void CriticalFailure(const char* msg)
	{
		OOSvrBase::Logger::log(OOSvrBase::Logger::Error,msg);

		// Die horribly now!
		abort();
	}
}

#endif
