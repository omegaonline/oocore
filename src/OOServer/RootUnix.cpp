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
//  ***** THIS IS A SECURE MODULE *****
//
//  It will be run as Administrator/setuid root
//
//  Therefore it needs to be SAFE AS HOUSES!
//
//  Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"
#include "RootManager.h"

#if defined(HAVE_UNISTD_H)

#if defined(HAVE_EV_H)
#include <ev.h>
#endif

/*bool Root::Manager::secure_file(const std::string& strFile, bool bPublicRead)
{
    // Make sure the file is owned by root (0)
    if (chown(strFile.c_str(),0,(gid_t)-1) != 0)
        LOG_ERROR_RETURN(("chown(%s) failed: %s",strFile.c_str(),OOSvrBase::Logger::format_error(errno).c_str()),false);

    if (chmod(strFile.c_str(),S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0)
        LOG_ERROR_RETURN(("chmod(%s) failed: %s",strFile.c_str(),OOSvrBase::Logger::format_error(errno).c_str()),false);

    return true;
}*/

bool Root::Manager::load_config()
{
	// Clear current entries
	m_config_args.clear();

	// Insert platform defaults
	m_config_args["regdb_path"] = "/var/lib/omegaonline/";
	m_config_args["users_path"] = m_config_args["regdb_path"] + "users/";

	// Determine conf file
	std::string strFile("/etc/omegaonline.conf");

	std::map<std::string,std::string>::const_iterator f = m_cmd_args.find("conf-file");
	if (f != m_cmd_args.end())
		strFile = f->second;

	return load_config_file(strFile);
}

bool Root::Manager::wait_for_quit()
{
#if defined(HAVE_EV_H)

	// Use libev to wait on the default loop
#if defined (EVFLAG_SIGNALFD)
	struct ev_loop* pLoop = ev_default_loop(EVFLAG_AUTO | EVFLAG_NOENV | EVFLAG_SIGNALFD);
#else
	struct ev_loop* pLoop = ev_default_loop(EVFLAG_AUTO | EVFLAG_NOENV);
#endif

	if (!pLoop)
		LOG_ERROR_RETURN(("ev_default_loop failed: %s",OOSvrBase::Logger::format_error(errno).c_str()),true);

	// Add watchers for SIG_KILL, SIG_HUP, SIG_CHILD etc...
	void* POSIX_TODO;

	// Let ev loop...
	::ev_loop(pLoop,0);

	return true;

#else
#error Some kind of signal mechanism?
#endif
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
