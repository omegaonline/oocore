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

#if !defined(_WIN32)

#if defined(HAVE_SIGNAL_H)
#include <signal.h>
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

namespace
{
	struct cond_pair_t
	{
		OOBase::Condition::Mutex m_lock;
		OOBase::Condition        m_condition;
		bool                     m_quit;
	};
	static OOBase::SmartPtr<cond_pair_t> s_ptrQuit;

#if defined(HAVE_SIGNAL_H)

	void on_sigterm(int)
	{
		if (s_ptrQuit)
		{
			s_ptrQuit->m_quit = true;
			s_ptrQuit->m_condition.signal();
		}
	}

	void on_sighup(int)
	{
		if (s_ptrQuit)
			s_ptrQuit->m_condition.signal();
	}

#endif

}

bool Root::Manager::wait_for_quit()
{
#if defined(HAVE_SIGNAL_H)
	// Catch SIGTERM
	if (signal(SIGTERM,&on_sigterm) == SIG_ERR)
		LOG_ERROR_RETURN(("signal() failed: %s",OOBase::strerror(errno).c_str()),false);

	// Catch SIGHUP
	if (signal(SIGHUP,&on_sighup) == SIG_ERR)
		LOG_ERROR_RETURN(("signal() failed: %s",OOBase::strerror(errno).c_str()),false);

	// Ignore SIGPIPE
	if (signal(SIGPIPE,SIG_IGN) == SIG_ERR)
		LOG_ERROR_RETURN(("signal() failed: %s",OOBase::strerror(errno).c_str()),false);

#else
#error Fix me!
#endif

	OOBASE_NEW(s_ptrQuit,cond_pair_t());
	if (!s_ptrQuit)
		LOG_ERROR_RETURN(("Out of memory"),false);

	s_ptrQuit->m_quit = false;

	// Wait for the event to be signalled
	OOBase::Guard<OOBase::Condition::Mutex> guard(s_ptrQuit->m_lock);

	s_ptrQuit->m_condition.wait(s_ptrQuit->m_lock);
	bool bReturn = s_ptrQuit->m_quit;
	cond_pair_t* c = s_ptrQuit.detach();

	guard.release();

	delete c;

	LOG_DEBUG(("ooserverd exiting..."));

	return bReturn;
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
