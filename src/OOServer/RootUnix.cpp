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

#if !defined(_WIN32)

#include "RootManager.h"

#include <signal.h>
#include <stdlib.h>

/*bool Root::Manager::secure_file(const std::string& strFile, bool bPublicRead)
{
    // Make sure the file is owned by root (0)
    if (chown(strFile.c_str(),0,(gid_t)-1) != 0)
        LOG_ERROR_RETURN(("chown(%s) failed: %s",strFile.c_str(),OOBase::system_error_text(errno).c_str()),false);

    if (chmod(strFile.c_str(),S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0)
        LOG_ERROR_RETURN(("chmod(%s) failed: %s",strFile.c_str(),OOBase::system_error_text(errno).c_str()),false);

    return true;
}*/

bool Root::Manager::load_config(const OOBase::CmdArgs::results_t& cmd_args)
{
	// Clear current entries
	m_config_args.clear();

	// Determine conf file
	OOBase::String strFile;
	if (cmd_args.find("conf-file",strFile))
	{
		char* rpath = realpath(strFile.c_str(),NULL);
		OOBase::Logger::log(OOBase::Logger::Information,"Using config file: %s",rpath);
		free(rpath);
	}

	return load_config_file(strFile.empty() ? "/etc/omegaonline.conf" : strFile.c_str());
}

bool Root::Manager::start_client_acceptor()
{
	const char* pipe_name = "/tmp/omegaonline";

	m_sa.mode = 0777;

	int err = 0;
	m_client_acceptor = Proactor::instance().accept_local(this,&Manager::accept_client,pipe_name,err,&m_sa);
	if (err == EADDRINUSE)
	{
		unlink(pipe_name);
		m_client_acceptor = Proactor::instance().accept_local(this,&Manager::accept_client,pipe_name,err,&m_sa);
	}

	if (err != 0)
		LOG_ERROR_RETURN(("Proactor::accept_local failed: '%s' %s",pipe_name,OOBase::system_error_text(err)),false);

	return true;
}

#endif
