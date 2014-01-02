///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
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
//  Do not include anything unnecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"
#include "RootManager.h"

#include <signal.h>
#include <stdlib.h>
#include <limits.h>

#if defined(_WIN32)
#include <shlwapi.h>
#include <shlobj.h>
#endif

template class OOBase::Singleton<OOBase::Proactor,Root::Manager>;

Root::Manager::Manager() :
		m_proactor(NULL)
{
}

int Root::Manager::run(const OOBase::CmdArgs::results_t& cmd_args)
{
	int ret = EXIT_FAILURE;

	// Load the config and open the root registry
	if (load_config(cmd_args))
	{
		// Start the proactor pool
		int err = 0;
		m_proactor = OOBase::Proactor::create(err);
		if (err)
			LOG_ERROR(("Failed to create proactor: %s",OOBase::system_error_text(err)));
		else
		{
			OOBase::String strThreads;
			get_config_arg("concurrency",strThreads);
			size_t threads = atoi(strThreads.c_str());
			if (threads < 1 || threads > 8)
				threads = 2;

			err = m_proactor_pool.run(&run_proactor,m_proactor,threads);
			if (err)
				LOG_ERROR(("Thread pool create failed: %s",OOBase::system_error_text(err)));
			else
			{
				// Start the registry process
				if (start_system_registry())
				{
					// Spawn the sandbox
					if (spawn_sandbox_process())
					{
						// Start listening for clients
						if (start_client_acceptor())
						{
							ret = EXIT_SUCCESS;

							// Wait for quit
							wait_for_quit();
								
							OOBase::Logger::log(OOBase::Logger::Information,APPNAME " closing");

							// Stop all services
							stop_services();

							// Stop accepting new clients
							m_client_acceptor = NULL;
						}

						// Close the connections to all the user processes
						m_user_processes.clear();
						m_sandbox_process = NULL;
					}

					// Close the connections to all the registry processes
					m_registry_processes.clear();
				}

				// Stop any proactor threads
				m_proactor->stop();
				m_proactor_pool.join();
			}

			OOBase::Proactor::destroy(m_proactor);
		}
	}

	if (is_debug() && ret != EXIT_SUCCESS)
	{
		OOBase::Logger::log(OOBase::Logger::Debug,"Pausing to let you read the messages...");

		// Give us a chance to read the errors!
		OOBase::Thread::sleep(15000);
	}

	return ret;
}

int Root::Manager::run_proactor(void* p)
{
	int err = 0;
	return static_cast<OOBase::Proactor*>(p)->run(err);
}

bool Root::Manager::get_config_arg(const char* name, OOBase::String& val)
{
	OOBase::String val2;
	if (m_config_args.find(name,val2))
	{
		int err = val.assign(val2);
		if (err)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);
		return true;
	}

	return false;
}

bool Root::Manager::load_config(const OOBase::CmdArgs::results_t& cmd_args)
{
	OOBase::Logger::log(OOBase::Logger::Information,"Loading configuration...");

	int err = 0;

	// Clear current entries
	m_config_args.clear();

	// Copy command line args
	for (size_t i=0; i < cmd_args.size(); ++i)
	{
		OOBase::String strKey,strValue;
		err = strKey.assign(*cmd_args.key_at(i));
		if (!err)
			err = strValue.assign(*cmd_args.at(i));
		if (!err)
			err = m_config_args.insert(strKey,strValue);

		if (err)
			LOG_ERROR_RETURN(("Failed to copy command args: %s",OOBase::system_error_text(err)),false);
	}

#if defined(_WIN32)
	// Read from WIN32 registry
	err = OOBase::ConfigFile::load_registry(HKEY_LOCAL_MACHINE,"Software\\Omega Online\\OOServer",m_config_args);
	if (err && err != ERROR_FILE_NOT_FOUND)
		LOG_ERROR_RETURN(("Failed read system registry: %s",OOBase::system_error_text(err)),false);
#endif

	// Determine conf file
	OOBase::String strFile;
	if (!m_config_args.find("conf-file",strFile))
	{
#if !defined(_WIN32)
		err = strFile.assign(CONFIG_DIR "/ooserver.conf");
		if (err)
			LOG_ERROR_RETURN(("Failed assign string: %s",OOBase::system_error_text(err)),false);
#endif
	}

	// Load from config file
	if (!strFile.empty())
	{
#if defined(__linux)
		char rpath[PATH_MAX] = {0};
		if (!realpath(strFile.c_str(),rpath))
			strncpy(rpath,strFile.c_str(),sizeof(rpath)-1);
#else
		const char* rpath = strFile.c_str();
#endif

		OOBase::Logger::log(OOBase::Logger::Information,"Using config file: '%s'",rpath);

		OOBase::ConfigFile::error_pos_t error = {0};
		err = OOBase::ConfigFile::load(strFile.c_str(),m_config_args,&error);
		if (err == EINVAL)
			LOG_ERROR_RETURN(("Failed read configuration file %s: Syntax error at line %lu, column %lu",rpath,(unsigned long)error.line,(unsigned long)error.col),false);
		else if (err)
			LOG_ERROR_RETURN(("Failed load configuration file %s: %s",rpath,OOBase::system_error_text(err)),false);
	}

	// Now set some defaults
	if (!m_config_args.exists("regdb_path"))
	{
		OOBase::String k,v;
		if ((err = k.assign("regdb_path")) != 0)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);

#if defined(_WIN32)
		wchar_t wszPath[MAX_PATH] = {0};
		HRESULT hr = SHGetFolderPathW(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_DEFAULT,wszPath);
		if FAILED(hr)
			LOG_ERROR_RETURN(("SHGetFolderPathW failed: %s",OOBase::system_error_text()),false);

		if (!PathAppendW(wszPath,L"Omega Online"))
			LOG_ERROR_RETURN(("PathAppendW failed: %s",OOBase::system_error_text()),false);

		if (!PathFileExistsW(wszPath))
			LOG_ERROR_RETURN(("%ls does not exist.",wszPath),false);

		if (!PathAddBackslashW(wszPath))
			LOG_ERROR_RETURN(("PathAddBackslash failed: %s",OOBase::system_error_text()),false);

		if ((err = OOBase::Win32::wchar_t_to_utf8(wszPath,v,cmd_args.get_allocator())) != 0)
			LOG_ERROR_RETURN(("WideCharToMultiByte failed: %s",OOBase::system_error_text(err)),false);
#else
		err = v.assign(REGDB_PATH);
		if (err)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);
#endif
		if ((err = m_config_args.insert(k,v)) != 0)
			LOG_ERROR_RETURN(("Failed to insert string: %s",OOBase::system_error_text(err)),false);
	}

	if (!m_config_args.exists("binary_path"))
	{
		OOBase::String v,k;
		if ((err = k.assign("binary_path")) != 0)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);

#if defined(_WIN32)
		// Get our module name
		wchar_t wszPath[MAX_PATH] = {0};
		if (!GetModuleFileNameW(NULL,wszPath,MAX_PATH))
			LOG_ERROR_RETURN(("GetModuleFileName failed: %s",OOBase::system_error_text()),false);

		// Strip off our name
		PathUnquoteSpacesW(wszPath);
		PathRemoveFileSpecW(wszPath);
		if (!PathAddBackslashW(wszPath))
			LOG_ERROR_RETURN(("PathAddBackslash failed: %s",OOBase::system_error_text()),false);

		if ((err = OOBase::Win32::wchar_t_to_utf8(wszPath,v,cmd_args.get_allocator())) != 0)
			LOG_ERROR_RETURN(("WideCharToMultiByte failed: %s",OOBase::system_error_text(err)),false);
#else
		err = v.assign(LIBEXEC_DIR);
		if (err)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);
#endif

		if ((err = m_config_args.insert(k,v)) != 0)
			LOG_ERROR_RETURN(("Failed to insert string: %s",OOBase::system_error_text(err)),false);
	}

	OOBase::Logger::log(OOBase::Logger::Information,"Configuration loaded successfully");

	return true;
}

void Root::Manager::accept_client(void* pThis, OOBase::AsyncSocket* pSocket, int err)
{
	OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket = pSocket;

	if (err)
		LOG_ERROR(("Client acceptor failed: %s",OOBase::system_error_text(err)));
	else
	{
		OOBase::RefPtr<ClientConnection> ptrConn = new ClientConnection(static_cast<Manager*>(pThis),ptrSocket);
		if (!ptrConn)
			LOG_ERROR(("Failed to allocate client connection: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)));
		else
			ptrConn->start();
	}
}

bool Root::Manager::connect_client(ClientConnection* client)
{
	OOBase::RefPtr<ClientConnection> ptrClient = client;
	ptrClient->addref();

	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	// Look for the matching user process
	for (OOBase::HashTable<pid_t,OOBase::RefPtr<UserConnection> >::iterator i=m_user_processes.begin(); i!=m_user_processes.end(); ++i)
	{
		if (i->value->same_login(ptrClient->get_uid(),ptrClient->get_session_id()))
		{
			OOBase::RefPtr<UserConnection> p = i->value;

			guard.release();

			return p->add_client(ptrClient->get_pid());
		}
	}

	// Add the client to an internal table...
	int err = m_clients.insert(ptrClient->get_pid(),ptrClient);
	if (err)
		LOG_ERROR_RETURN(("Failed to add client to client set: %s",OOBase::system_error_text(err)),false);

	// Spawn correct registry if not existing
	OOBase::RefPtr<RegistryConnection> ptrRegistry;
	for (OOBase::HandleTable<size_t,OOBase::RefPtr<RegistryConnection> >::iterator i=m_registry_processes.begin(); i!=m_registry_processes.end(); ++i)
	{
		if (i->value->same_user(ptrClient->get_uid()))
		{
			ptrRegistry = i->value;
			break;
		}
	}

	guard.release();

	if (!ptrRegistry)
		return spawn_user_registry(ptrClient);

	// Spawn correct user process
	return spawn_user_process(ptrClient,ptrRegistry);
}
