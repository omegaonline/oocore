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
		m_proactor(NULL),
		m_registry_processes(1)
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
			OOBase::LocalString strThreads(cmd_args.get_allocator());
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
				if (start_registry(cmd_args.get_allocator()))
				{
					// Spawn the sandbox
					if (spawn_sandbox_process(cmd_args.get_allocator()))
					{
						// Start listening for clients
						//if (start_client_acceptor(cmd_args.get_allocator()))
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

bool Root::Manager::get_config_arg(const char* name, OOBase::LocalString& val)
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
			LOG_ERROR_RETURN(("Failed read configuration file %s: Syntax error at line %u, column %u",rpath,error.line,error.col),false);
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

bool Root::Manager::start_registry(OOBase::AllocatorInstance& allocator)
{
	OOBase::Logger::log(OOBase::Logger::Information,"Starting system registry...");

	// Get dir from config
	OOBase::LocalString strRegPath(allocator);
	if (!get_config_arg("regdb_path",strRegPath) || strRegPath.empty())
		LOG_ERROR_RETURN(("Missing 'regdb_path' config setting"),false);

	int err = strRegPath.append("system.regdb");
	if (err)
		LOG_ERROR_RETURN(("Failed to append string: %s",OOBase::system_error_text(err)),false);

	// Get the binary path
	OOBase::LocalString strBinPath(allocator);
	if (!get_config_arg("binary_path",strBinPath))
		LOG_ERROR_RETURN(("Failed to find binary_path configuration parameter"),false);

#if defined(_WIN32)
	err = strBinPath.append("OOSvrReg.exe");
#else
	err = strBinPath.append("oosvrreg");
#endif
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);

	// Write initial message
	OOBase::CDRStream stream;
	size_t mark = stream.buffer()->mark_wr_ptr();
	stream.write(Omega::uint16_t(0));
	stream.write_string(strRegPath);

	OOBase::LocalString strThreads(allocator);
	get_config_arg("registry_concurrency",strThreads);
	Omega::byte_t threads = strtoul(strThreads.c_str(),NULL,10);
	if (threads < 1 || threads > 8)
		threads = 2;

	stream.write(threads);

	for (size_t pos = 0;pos < m_config_args.size();++pos)
	{
		if (!stream.write_string(*m_config_args.key_at(pos)) || !stream.write_string(*m_config_args.at(pos)))
			break;
	}
	stream.write("");

	stream.replace(static_cast<Omega::uint16_t>(stream.buffer()->length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write string: %s",OOBase::system_error_text(stream.last_error())),false);

	// Get our uid
	uid_t uid;
	OOBase::LocalString strOurUName(allocator);
	if (!get_our_uid(uid,strOurUName))
		return false;

	// Spawn the process
	SpawnedProcess p;
	bool bAgain;
	if (!platform_spawn(strBinPath,uid,NULL,OOBase::Environment::env_table_t(allocator),p,bAgain))
		return false;

	// Send the start data - blocking is OK as we have background proactor threads waiting
	err = OOBase::CDRIO::send_and_recv_with_header_blocking<Omega::uint16_t>(stream,p.m_ptrSocket);
	if (err)
		LOG_ERROR_RETURN(("Failed to send registry start data: %s",OOBase::system_error_text(err)),false);

	Omega::int32_t ret_err = 0;
	if (!stream.read(ret_err))
		LOG_ERROR_RETURN(("Failed to read start response from registry: %s",OOBase::system_error_text(stream.last_error())),false);
	if (ret_err)
		LOG_ERROR_RETURN(("Registry failed to start properly: %s",OOBase::system_error_text(ret_err)),false);

	// Add to registry process map
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	// Add to the handle map as ID 1
	err = m_registry_processes.force_insert(1,p);
	if (err)
		LOG_ERROR_RETURN(("Failed to insert registry handle: %s",OOBase::system_error_text(err)),false);

	OOBase::Logger::log(OOBase::Logger::Information,"System registry started successfully");

	return true;
}

OOBase::RefPtr<OOBase::AsyncSocket> Root::Manager::get_root_registry()
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket;
	SpawnedProcess* p = m_registry_processes.at(1);
	if (p)
		ptrSocket = p->m_ptrSocket;

	return ptrSocket;
}

bool Root::Manager::spawn_sandbox_process(OOBase::AllocatorInstance& allocator)
{
	OOBase::Logger::log(OOBase::Logger::Information,"Starting system sandbox...");

	OOBase::LocalString strUnsafe(allocator);
	bool bUnsafe = false;
	if (Root::is_debug() && get_config_arg("unsafe",strUnsafe))
		bUnsafe = (strUnsafe == "true");

	// Get username from config
	OOBase::LocalString strUName(allocator);
	if (!get_config_arg("sandbox_uname",strUName) && !bUnsafe)
		LOG_ERROR_RETURN(("Failed to find the 'sandbox_uname' setting in the config"),false);
	
	bool bAgain = false;
	uid_t uid;
	if (strUName.empty())
	{
		OOBase::LocalString strOurUName(allocator);
		if (!get_our_uid(uid,strOurUName))
			return false;

		// Warn!
		OOBase::Logger::log(OOBase::Logger::Warning,
			"Because the 'unsafe' mode is set the sandbox process will be started under the current user account '%s'.\n\n"
			"This is a security risk and should only be allowed for debugging purposes, and only then if you really know what you are doing.\n",
			strOurUName.c_str());
	}
	else if (!get_sandbox_uid(strUName,uid,bAgain))
	{
		if (bAgain && bUnsafe)
		{
			OOBase::LocalString strOurUName(allocator);
			if (!get_our_uid(uid,strOurUName))
				return false;

			OOBase::Logger::log(OOBase::Logger::Warning,
								   APPNAME " is running under a user account that does not have the privileges required to impersonate a different user.\n\n"
								   "Because the 'unsafe' mode is set the sandbox process will be started under the current user account '%s'.\n\n"
								   "This is a security risk and should only be allowed for debugging purposes, and only then if you really know what you are doing.\n",
								   strOurUName.c_str());
		}
		else
			return false;
	}

	// Get the environment settings
	OOBase::Environment::env_table_t tabSysEnv(allocator);
#if defined(_WIN32)
	int err = OOBase::Environment::get_user(uid,tabSysEnv);
#else
	int err = OOBase::Environment::get_current(tabSysEnv);
#endif
	if (err)
		LOG_ERROR_RETURN(("Failed to load environment variables: %s",OOBase::system_error_text(err)),0);

	// Get sandbox environment from root registry
	OOBase::RefPtr<OOBase::AsyncSocket> ptrRoot = get_root_registry();

	OOBase::CDRStream stream;
	size_t mark = stream.buffer()->mark_wr_ptr();
	stream.write(Omega::uint16_t(0));

	stream.replace(static_cast<Omega::uint16_t>(stream.buffer()->length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write string: %s",OOBase::system_error_text(stream.last_error())),false);

	void* TODO1;
	/*if (err)
		LOG_ERROR_RETURN(("Failed to send registry data: %s",OOBase::system_error_text(err)),false);

	Omega::int32_t ret_err = 0;
	if (!stream.read(ret_err))
		LOG_ERROR_RETURN(("Failed to read registry response from registry: %s",OOBase::system_error_text(stream.last_error())),false);
	if (ret_err)
		LOG_ERROR_RETURN(("Registry failed to respond properly: %s",OOBase::system_error_text(ret_err)),false);

	OOBase::Environment::env_table_t tabEnv(allocator);
	size_t num_vars;
	stream.read(num_vars);
	for (size_t i=0;i<num_vars;++i)
	{
		OOBase::LocalString key(allocator),value(allocator);

		if (!stream.read_string(key) || !stream.read_string(value))
			LOG_ERROR_RETURN(("Registry failed to respond properly: %s",OOBase::system_error_text(stream.last_error())),false);

		err = tabEnv.insert(key,value);
		if (err)
			LOG_ERROR_RETURN(("Failed to insert environment variable into block: %s",OOBase::system_error_text(err)),false);
	}

	err = OOBase::Environment::substitute(tabEnv,tabSysEnv);
	if (err)
		LOG_ERROR_RETURN(("Failed to substitute environment variables: %s",OOBase::system_error_text(err)),0);
*/

	// Get the binary path
	OOBase::LocalString strBinPath(allocator);
	if (!get_config_arg("binary_path",strBinPath))
		LOG_ERROR_RETURN(("Failed to find binary_path configuration parameter"),false);

#if defined(_WIN32)
	err = strBinPath.append("OOSvrUser.exe");
#else
	err = strBinPath.append("oosvruser");
#endif
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);

	// Spawn the process
	SpawnedProcess p;
	bool res = platform_spawn(strBinPath,uid,NULL,tabSysEnv,p,bAgain);
	if (!res && bAgain && bUnsafe && !strUName.empty())
	{
		OOBase::LocalString strOurUName(allocator);
		if (!get_our_uid(uid,strOurUName))
			return false;

		OOBase::Logger::log(OOBase::Logger::Warning,
							   APPNAME " is running under a user account that does not have the privileges required to create new processes as a different user.\n\n"
							   "Because the 'unsafe' mode is set the sandbox process will be started under the current user account '%s'.\n\n"
							   "This is a security risk and should only be allowed for debugging purposes, and only then if you really know what you are doing.\n",
							   strOurUName.c_str());


		void* TODO; // Reload env vars

		res = platform_spawn(strBinPath,uid,NULL,tabSysEnv,p,bAgain);
	}
	if (!res)
		return false;

	// Now tell the root registry to connect to the sandbox process
	if (!connect_root_registry_to_sandbox(uid,ptrRoot,p.m_ptrSocket))
		return false;

	// Add to process map
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	// Add to the handle map as ID 1
	SpawnedProcess* s = m_user_processes.find(p.m_ptrProcess->GetPid());
	if (s)
		*s = p;
	else
	{
		err = m_user_processes.insert(p.m_ptrProcess->GetPid(),p);
		if (err)
			LOG_ERROR_RETURN(("Failed to insert process handle: %s",OOBase::system_error_text(err)),false);
	}

	OOBase::Logger::log(OOBase::Logger::Information,"System sandbox started successfully");

	return true;
}

void Root::Manager::accept_client(void* pThis, OOBase::AsyncSocket* pSocket, int err)
{
	OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket = pSocket;

	if (err)
		LOG_ERROR(("Client acceptor failed: %s",OOBase::system_error_text(err)));
	else
	{
		OOBase::RefPtr<ClientConnection> ptrConn = new (std::nothrow) ClientConnection(static_cast<Manager*>(pThis),ptrSocket);
		if (!ptrConn)
			LOG_ERROR(("Failed to allocate client connection: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)));
		else
			ptrConn->start();
	}
}

bool Root::Manager::find_user_process(ClientConnection* client)
{
	OOBase::RefPtr<ClientConnection> ptrClient = client;
	ptrClient->addref();

	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	// Look for the matching user process
	for (size_t i=m_user_processes.begin(); i!=m_user_processes.npos; i=m_user_processes.next(i))
	{
		SpawnedProcess* p = m_user_processes.at(i);
		if (p->m_ptrProcess->IsSameLogin(ptrClient->get_uid(),ptrClient->get_session_id()))
		{
			SpawnedProcess u = *p;

			guard.release();

			return add_client_to_user(u,ptrClient);
		}
	}

	// Add the client to an internal table...
	int err = m_clients.insert(ptrClient->get_pid(),ptrClient);
	if (err)
		LOG_ERROR_RETURN(("Failed to add client to client set: %s",OOBase::system_error_text(err)),false);

	// Spawn correct registry if not existing
	SpawnedProcess* pUserReg = NULL;
	for (size_t i=m_registry_processes.begin(); i!=m_registry_processes.npos; i=m_registry_processes.next(i))
	{
		SpawnedProcess* p = m_registry_processes.at(i);
		if (p->m_ptrProcess->IsSameUser(ptrClient->get_uid()))
		{
			pUserReg = p;
			break;
		}
	}

	if (!pUserReg)
	{
		guard.release();

		return spawn_user_registry(ptrClient->get_uid());
	}

	// Spawn correct user process
	OOBase::RefPtr<OOBase::AsyncSocket> ptrRegistry = pUserReg->m_ptrSocket;

	guard.release();

	return spawn_user_process(ptrClient,ptrRegistry);
}

bool Root::Manager::spawn_user_registry(const uid_t& uid)
{
	OOBase::Logger::log(OOBase::Logger::Information,"Starting user registry...");

	OOBase::StackAllocator<512> allocator;

	OOBase::LocalString strRegPath(allocator);
	if (!get_config_arg("regdb_path",strRegPath))
		LOG_ERROR_RETURN(("Missing 'regdb_path' config setting"),false);

	OOBase::LocalString strUsersPath(allocator);
	get_config_arg("users_path",strUsersPath);

	OOBase::LocalString strHive(allocator);
	if (!get_registry_hive(uid,strRegPath,strUsersPath,strHive))
		return false;

	// Get the binary path
	OOBase::LocalString strBinPath(allocator);
	if (!get_config_arg("binary_path",strBinPath))
		LOG_ERROR_RETURN(("Failed to find binary_path configuration parameter"),false);

#if defined(_WIN32)
	int err = strBinPath.append("OOSvrReg.exe");
#else
	int err = strBinPath.append("oosvrreg");
#endif
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);

	// Write initial message
	OOBase::CDRStream stream;
	size_t mark = stream.buffer()->mark_wr_ptr();
	stream.write(Omega::uint16_t(0));
	stream.write_string(strRegPath);

	OOBase::LocalString strThreads(allocator);
	get_config_arg("registry_concurrency",strThreads);
	Omega::byte_t threads = strtoul(strThreads.c_str(),NULL,10);
	if (threads < 1 || threads > 8)
		threads = 2;

	stream.write(threads);
	stream.write("");

	stream.replace(static_cast<Omega::uint16_t>(stream.buffer()->length()),mark);
	if (stream.last_error())
		LOG_ERROR_RETURN(("Failed to write string: %s",OOBase::system_error_text(stream.last_error())),false);

	// Spawn the process
	SpawnedProcess p;
	bool bAgain;
	if (!platform_spawn(strBinPath,uid,NULL,OOBase::Environment::env_table_t(allocator),p,bAgain))
		return false;

	// Add to registry process map
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	// Add to the registry map
	size_t handle = 0;
	err = m_registry_processes.insert(p,handle,2,size_t(-1));
	if (err)
		LOG_ERROR_RETURN(("Failed to insert registry handle: %s",OOBase::system_error_text(err)),false);

	guard.release();

	// Send the message
	err = OOBase::CDRIO::send_and_recv_with_header_sync<Omega::uint16_t>(stream,p.m_ptrSocket,this,&Manager::on_registry_spawned);
	if (err)
	{
		guard.acquire();

		m_registry_processes.remove(handle);

		LOG_ERROR_RETURN(("Failed to send registry start data: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

void Root::Manager::on_registry_spawned(OOBase::CDRStream& stream, int err)
{
	if (err)
		LOG_ERROR(("Failed to send and receive from user registry: %s",OOBase::system_error_text(err)));
	else
	{
		Omega::int32_t ret_err = 0;
		if (!stream.read(ret_err))
		{
			err = stream.last_error();
			LOG_ERROR(("Failed to read start response from registry: %s",OOBase::system_error_text(err)));
		}
		else if (ret_err)
		{
			err = ret_err;
			LOG_ERROR(("Registry failed to start properly: %s",OOBase::system_error_text(ret_err)));
		}
		else
		{
			OOBase::Logger::log(OOBase::Logger::Information,"User registry started successfully");
		}
	}

	if (err)
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);
	}
}

/*bool Root::Manager::get_user_process(uid_t& uid, const OOBase::LocalString& session_id, UserProcess& user_process)
{
	for (int attempts = 0;attempts < 2;++attempts)
	{
		// See if we have a process already
		OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

		OOBase::Stack<Omega::uint32_t,OOBase::AllocatorInstance> vecDead(session_id.get_allocator());
		bool bFound = false;

		for (size_t i=m_mapUserProcesses.begin(); i!=m_mapUserProcesses.npos; i=m_mapUserProcesses.next(i))
		{
			UserProcess* pU = m_mapUserProcesses.at(i);
			if (!pU->m_ptrProcess->IsRunning())
			{
				vecDead.push(*m_mapUserProcesses.key_at(i));
			}
			else if (pU->m_ptrProcess->IsSameLogin(uid,session_id.c_str()))
			{
				user_process = *pU;
				bFound = true;
			}
			else if (!bFound && pU->m_ptrProcess->IsSameUser(uid))
			{
				user_process.m_ptrRegistry = pU->m_ptrRegistry;
			}
		}

		read_guard.release();

		if (!vecDead.empty())
		{
			OOBase::Guard<OOBase::RWMutex> guard(m_lock);

			for (Omega::uint32_t i = 0;vecDead.pop(&i);)
				m_mapUserProcesses.remove(i);
		}

		if (bFound)
			return true;

		// Spawn a new user process
		bool bAgain = false;
		if (spawn_user_process(session_id.get_allocator(),uid,session_id.c_str(),user_process.m_ptrRegistry,user_process.m_strPipe,bAgain) != 0)
			return true;

		if (attempts == 0 && bAgain)
		{
			OOBase::LocalString strUnsafe(session_id.get_allocator());
			if (Root::is_debug() && get_config_arg("unsafe",strUnsafe) && strUnsafe == "true")
			{
				OOBase::LocalString strOurUName(session_id.get_allocator());
				if (!get_our_uid(uid,strOurUName))
					return false;

				OOBase::Logger::log(OOBase::Logger::Warning,
									   APPNAME " is running under a user account that does not have the privileges required to create new processes as a different user.\n\n"
									   "Because the 'unsafe' mode is set the new user process will be started under the current user account '%s'.\n\n"
									   "This is a security risk and should only be allowed for debugging purposes, and only then if you really know what you are doing.\n",
									   strOurUName.c_str());
			}
			else
				return false;
		}
	}

	return false;
}*/

bool Root::Manager::load_user_env(OOBase::SmartPtr<Db::Hive> ptrRegistry, OOBase::Environment::env_table_t& tabEnv)
{
	Omega::int64_t key = 0;
	OOBase::LocalString strSubKey(tabEnv.get_allocator()),strLink(tabEnv.get_allocator()),strFullKeyName(tabEnv.get_allocator());
	int err2 = 0;
	const char* key_text = (ptrRegistry ? "Local User" : "System/Sandbox");
	if (!ptrRegistry)
	{
		ptrRegistry = m_registry;
		err2 = strSubKey.assign("/System/Sandbox/Environment");
	}
	else
		err2 = strSubKey.assign("/Environment");

	if (err2)
		LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err2)),false);

	Db::hive_errors err = ptrRegistry->create_key(0,key,strSubKey,0,0,strLink,strFullKeyName);
	if (err)
	{
		if (err != Db::HIVE_NOTFOUND)
			LOG_ERROR_RETURN(("Failed to open the '/%s/Environment' key in the user registry",key_text),false);

		return true;
	}

	Db::Hive::registry_set_t names(tabEnv.get_allocator());
	err = ptrRegistry->enum_values(key,0,names);
	if (err)
		LOG_ERROR_RETURN(("Failed to enumerate the '/%s/Environment' values in the user registry",key_text),false);

	OOBase::LocalString strName(names.get_allocator());
	while (names.pop(&strName))
	{
		OOBase::LocalString strVal(tabEnv.get_allocator());
		err = ptrRegistry->get_value(key,strName.c_str(),0,strVal);
		if (err)
			LOG_ERROR_RETURN(("Failed to get the '/%s/Environment/%s' value from the user registry",key_text,strName.c_str()),false);

		err2 = tabEnv.insert(strName,strVal);
		if (err2)
			LOG_ERROR_RETURN(("Failed to insert environment string: %s",OOBase::system_error_text(err2)),false);
	}

	return true;
}

/*Omega::uint32_t Root::Manager::spawn_user_process(OOBase::AllocatorInstance& allocator, const uid_t& uid, const char* session_id, OOBase::SmartPtr<Db::Hive> ptrRegistry, OOBase::String& strPipe, bool& bAgain)
{
	// Get the binary path
	OOBase::LocalString strBinPath(allocator);
	if (!get_config_arg("binary_path",strBinPath))
		LOG_ERROR_RETURN(("Failed to find binary_path configuration parameter"),0);

	// Do a platform specific spawn
	UserProcess process;
	process.m_ptrRegistry = ptrRegistry;

	// Init the registry, if necessary
	if (session_id && !process.m_ptrRegistry)
	{
		OOBase::LocalString strRegPath(allocator);
		if (!get_config_arg("regdb_path",strRegPath))
			LOG_ERROR_RETURN(("Missing 'regdb_path' config setting"),0);

		OOBase::LocalString strUsersPath(allocator);
		get_config_arg("users_path",strUsersPath);

		OOBase::LocalString strHive(allocator);
		if (!get_registry_hive(uid,strRegPath,strUsersPath,strHive))
			return false;

		// Create a new database
		process.m_ptrRegistry = new (std::nothrow) Db::Hive(this,strHive.c_str());
		if (!process.m_ptrRegistry)
			LOG_ERROR_RETURN(("Failed to allocate hive: %s",OOBase::system_error_text()),0);

		if (!process.m_ptrRegistry->open(SQLITE_OPEN_READWRITE))
			LOG_ERROR_RETURN(("Failed to open hive: %s",strHive.c_str()),0);
	}

	// Get the environment settings
	OOBase::Environment::env_table_t tabSysEnv(allocator);
#if defined(_WIN32)
	int err = OOBase::Environment::get_user(uid,tabSysEnv);
#else
	int err = OOBase::Environment::get_current(tabSysEnv);
#endif
	if (err)
		LOG_ERROR_RETURN(("Failed to load environment variables: %s",OOBase::system_error_text(err)),0);

	OOBase::Environment::env_table_t tabEnv(allocator);
	if (!load_user_env(process.m_ptrRegistry,tabEnv))
		return false;

	err = OOBase::Environment::substitute(tabEnv,tabSysEnv);
	if (err)
		LOG_ERROR_RETURN(("Failed to substitute environment variables: %s",OOBase::system_error_text(err)),0);

#if defined(_WIN32)
	err = strBinPath.append("OOSvrUser.exe");
#else
	err = strBinPath.append("oosvruser");
#endif
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),0);

	OOBase::SmartPtr<Root::Process> ptrSpawn;
	OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket;
	if (!platform_spawn(strBinPath,uid,session_id,tabEnv,ptrSpawn,ptrSocket,bAgain))
		return 0;

	// Bootstrap the user process...
	OOBase::RefPtr<OOServer::MessageConnection> ptrMC;
	Omega::uint32_t channel_id = bootstrap_user(ptrSocket,ptrMC,process.m_strPipe);
	if (!channel_id)
		return false;

	process.m_ptrProcess = ptrSpawn;

	// Insert the data into the process map...
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	// Check we haven't created a duplicate while we spawned...
	if (session_id)
	{
		for (size_t i=m_mapUserProcesses.begin(); i!=m_mapUserProcesses.npos; i=m_mapUserProcesses.next(i))
		{
			UserProcess* p = m_mapUserProcesses.at(i);
			if (p->m_ptrProcess->IsSameLogin(uid,session_id))
			{
				strPipe = p->m_strPipe;
				return *m_mapUserProcesses.key_at(i);
			}
		}
	}

	err = m_mapUserProcesses.insert(channel_id,process);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to insert into map: %s",OOBase::system_error_text(err)),0);

	// Now start the read cycle from ptrMC
	if ((err = ptrMC->recv()) != 0)
	{
		channel_closed(channel_id,0);
		channel_id = 0;
	}

	strPipe = process.m_strPipe;

	return channel_id;
}*/
