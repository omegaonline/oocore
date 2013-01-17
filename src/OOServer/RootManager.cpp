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
		m_sandbox_channel(0)
{
	// Root channel is fixed
	set_channel(0x80000000,0x80000000,0x7F000000,0);
}

int Root::Manager::run(const OOBase::CmdArgs::results_t& cmd_args)
{
	int ret = EXIT_FAILURE;

	// Load the config and open the root registry
	if (load_config(cmd_args) && init_database(cmd_args.get_allocator()))
	{
		// Start the proactor pool
		int err = 0;
		m_proactor = OOBase::Proactor::create(err);
		if (err)
			LOG_ERROR(("Failed to create proactor: %s",OOBase::system_error_text(err)));
		else
		{
			err = m_proactor_pool.run(&run_proactor,m_proactor,2);
			if (err)
				LOG_ERROR(("Thread pool create failed: %s",OOBase::system_error_text(err)));
			else
			{
				// Start the handler
				if (start_request_threads(2))
				{
					// Spawn the sandbox
					if (spawn_sandbox(cmd_args.get_allocator()))
					{
						// Start listening for clients
						if (start_client_acceptor(cmd_args.get_allocator()))
						{
							OOBase::Logger::log(OOBase::Logger::Information,APPNAME " started successfully");

							ret = EXIT_SUCCESS;

							// Wait for quit
							wait_for_quit();
								
							OOBase::Logger::log(OOBase::Logger::Information,APPNAME " closing");

							// Stop all services
							stop_services();

							// Stop accepting new clients
							m_client_acceptor = NULL;
						}

						// Close all channels
						shutdown_channels();

						// Wait for all user processes to terminate
						m_mapUserProcesses.clear();
					}
				}

				// Stop the MessageHandler
				stop_request_threads();

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

bool Root::Manager::init_database(OOBase::AllocatorInstance& allocator)
{
	// Get dir from config
	OOBase::LocalString dir(allocator);
	if (!get_config_arg("regdb_path",dir) || dir.empty())
		LOG_ERROR_RETURN(("Missing 'regdb_path' config setting"),false);

	int err = dir.append("system.regdb");
	if (err)
		LOG_ERROR_RETURN(("Failed to append string: %s",OOBase::system_error_text(err)),false);

	// Create a new system database
	m_registry = new (std::nothrow) Db::Hive(this,dir.c_str());
	if (!m_registry)
		LOG_ERROR_RETURN(("Failed to create registry hive: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	return m_registry->open(SQLITE_OPEN_READWRITE);
}

bool Root::Manager::get_config_arg(const char* name, OOBase::LocalString& val)
{
	OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

	OOBase::String val2;
	if (m_config_args.find(name,val2))
	{
		int err = val.assign(val2);
		if (err)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);
		return true;
	}

	if (m_registry)
	{
		OOBase::LocalString strSubKey(val.get_allocator());
		int err2 = strSubKey.assign("/System/Server/Settings");
		if (err2)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err2)),false);

		Omega::int64_t key = 0;
		Db::hive_errors err = registry_open_key(key,strSubKey,0);
		if (err)
		{
			if (err != Db::HIVE_NOTFOUND)
				LOG_ERROR_RETURN(("Failed to open the '/System/Server/Settings' key in the system registry"),false);
		}
		else
		{
			if ((err = m_registry->get_value(key,name,0,val)) == 0)
				return true;
			
			if (err != Db::HIVE_NOTFOUND)
				LOG_ERROR_RETURN(("Failed to get the '/System/Server/Settings/%s' setting in the system registry",name),false);
		}
	}

	return false;
}

void Root::Manager::get_config_arg(OOBase::CDRStream& request, OOBase::CDRStream& response)
{
	OOBase::StackAllocator<256> allocator;
	OOBase::LocalString strArg(allocator);
	if (!request.read_string(strArg))
		LOG_ERROR(("Failed to read get_config_arg request parameters: %s",OOBase::system_error_text(request.last_error())));

	OOBase::LocalString strValue(allocator);
	get_config_arg(strArg.c_str(),strValue);

	if (!response.write(static_cast<OOServer::RootErrCode_t>(OOServer::Ok)) || !response.write_string(strValue))
		LOG_ERROR(("Failed to write response: %s",OOBase::system_error_text(response.last_error())));
}

bool Root::Manager::load_config(const OOBase::CmdArgs::results_t& cmd_args)
{
	int err = 0;
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

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

		OOBase::Logger::log(OOBase::Logger::Information,"Using config file: %s",rpath);

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

	return true;
}

int Root::Manager::run_proactor(void* p)
{
	int err = 0;
	return static_cast<OOBase::Proactor*>(p)->run(err);
}

bool Root::Manager::spawn_sandbox(OOBase::AllocatorInstance& allocator)
{
	OOBase::LocalString strUnsafe(allocator);
	bool bUnsafe = false;
	if (Root::is_debug() && get_config_arg("unsafe",strUnsafe))
		bUnsafe = (strUnsafe == "true");

	// Get username from config
	OOBase::LocalString strUName(allocator);
	if (!get_config_arg("sandbox_uname",strUName) && !bUnsafe)
		LOG_ERROR_RETURN(("Failed to find the 'sandbox_uname' setting in the config"),false);
	
	bool bAgain = false;
	uid_t uid = uid_t(-1);
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

	OOBase::String strPipe;
	m_sandbox_channel = spawn_user(allocator,uid,NULL,OOBase::SmartPtr<Db::Hive>(),strPipe,bAgain);
	if (m_sandbox_channel == 0 && bUnsafe && !strUName.empty() && bAgain)
	{
		OOBase::LocalString strOurUName(allocator);
		if (!get_our_uid(uid,strOurUName))
			return false;

		OOBase::Logger::log(OOBase::Logger::Warning,
							   APPNAME " is running under a user account that does not have the privileges required to create new processes as a different user.\n\n"
							   "Because the 'unsafe' mode is set the sandbox process will be started under the current user account '%s'.\n\n"
							   "This is a security risk and should only be allowed for debugging purposes, and only then if you really know what you are doing.\n",
							   strOurUName.c_str());

		m_sandbox_channel = spawn_user(allocator,uid,NULL,OOBase::SmartPtr<Db::Hive>(),strPipe,bAgain);
	}

#if defined(_WIN32)
	CloseHandle(uid);
#endif

	return (m_sandbox_channel != 0);
}

bool Root::Manager::can_route(Omega::uint32_t src_channel, Omega::uint32_t dest_channel)
{
	if (!OOServer::MessageHandler::can_route(src_channel,dest_channel))
		return false;

	// Only route to or from the sandbox
	return ((src_channel & 0xFF000000) == (m_sandbox_channel & 0xFF000000) || (dest_channel & 0xFF000000) == (m_sandbox_channel & 0xFF000000));
}

void Root::Manager::on_channel_closed(Omega::uint32_t channel)
{
	// Remove the associated spawned process
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_mapUserProcesses.remove(channel);
}

bool Root::Manager::get_user_process(uid_t& uid, const OOBase::LocalString& session_id, UserProcess& user_process)
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
		if (spawn_user(session_id.get_allocator(),uid,session_id.c_str(),user_process.m_ptrRegistry,user_process.m_strPipe,bAgain) != 0)
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
}

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

Omega::uint32_t Root::Manager::spawn_user(OOBase::AllocatorInstance& allocator, uid_t uid, const char* session_id, OOBase::SmartPtr<Db::Hive> ptrRegistry, OOBase::String& strPipe, bool& bAgain)
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
}

Omega::uint32_t Root::Manager::bootstrap_user(OOBase::RefPtr<OOBase::AsyncSocket>& ptrSocket, OOBase::RefPtr<OOServer::MessageConnection>& ptrMC, OOBase::String& strPipe)
{
	OOBase::CDRStream stream;

	//if (!stream.recv_string(ptrSocket,strPipe))
	//	LOG_ERROR_RETURN(("CDRStream::read failed: %s",OOBase::system_error_text(stream.last_error())),0);

	ptrMC = new (std::nothrow) OOServer::MessageConnection(this,ptrSocket);
	if (!ptrMC)
		LOG_ERROR_RETURN(("Failed to allocate MessageConnection: %s",OOBase::system_error_text()),0);

	Omega::uint32_t channel_id = register_channel(ptrMC,0);
	if (!channel_id)
		return 0;

	stream.reset();
	if (!stream.write(m_sandbox_channel) || !stream.write(channel_id))
	{
		channel_closed(channel_id,0);
		LOG_ERROR_RETURN(("CDRStream::write failed: %s",OOBase::system_error_text(stream.last_error())),0);
	}

	int err = ptrMC->send(stream.buffer(),NULL);
	if (err)
	{
		channel_closed(channel_id,0);
		channel_id = 0;
	}

	return channel_id;
}

void Root::Manager::process_request(OOBase::CDRStream& request, Omega::uint32_t src_channel_id, Omega::uint16_t src_thread_id, Omega::uint32_t attribs)
{
	OOServer::RootOpCode_t op_code;
	request.read(op_code);

	if (request.last_error() != 0)
	{
		LOG_ERROR(("Bad request: %s",OOBase::system_error_text(request.last_error())));
		return;
	}

	OOBase::CDRStream response;
	switch (op_code)
	{
	case OOServer::User_NotifyStarted:
		if (src_channel_id == m_sandbox_channel)
			start_services();
		break;

	case OOServer::User_GetConfigArg:
		get_config_arg(request,response);
		break;

	case OOServer::Service_Start:
		start_service(src_channel_id,request,response);
		break;

	case OOServer::Service_Stop:
		stop_service(src_channel_id,request,response);
		break;

	case OOServer::Service_IsRunning:
		service_is_running(src_channel_id,request,response);
		break;

	case OOServer::Service_ListRunning:
		service_list_running(src_channel_id,response);
		break;

	case OOServer::Registry_OpenKey:
		registry_open_key(src_channel_id,request,response);
		break;

	case OOServer::Registry_DeleteSubKey:
		registry_delete_key(src_channel_id,request,response);
		break;

	case OOServer::Registry_EnumSubKeys:
		registry_enum_subkeys(src_channel_id,request,response);
		break;

	case OOServer::Registry_ValueExists:
		registry_value_exists(src_channel_id,request,response);
		break;

	case OOServer::Registry_GetValue:
		registry_get_value(src_channel_id,request,response);
		break;

	case OOServer::Registry_SetValue:
		registry_set_value(src_channel_id,request,response);
		break;

	case OOServer::Registry_EnumValues:
		registry_enum_values(src_channel_id,request,response);
		break;

	case OOServer::Registry_DeleteValue:
		registry_delete_value(src_channel_id,request,response);
		break;

	default:
		response.write(static_cast<OOServer::RootErrCode_t>(OOServer::Errored));
		LOG_ERROR(("Bad request op_code: %d",op_code));
		break;
	}

	if (!response.last_error() && !(attribs & OOServer::Message_t::asynchronous))
		send_response(src_channel_id,src_thread_id,response,attribs);
}

OOServer::MessageHandler::io_result::type Root::Manager::sendrecv_sandbox(const OOBase::CDRStream& request, OOBase::CDRStream* response, Omega::uint16_t attribs)
{
	return send_request(m_sandbox_channel,&request,response,attribs);
}

void Root::Manager::accept_client(void* pThis, OOBase::AsyncSocket* pSocket, int err)
{
	OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket = pSocket;

	static_cast<Manager*>(pThis)->accept_client_i(ptrSocket,err);
}

#include "../../include/Omega/OOCore_version.h"

void Root::Manager::accept_client_i(OOBase::RefPtr<OOBase::AsyncSocket>& ptrSocket, int err)
{
	if (err != 0)
		LOG_ERROR(("Accept failure: %s",OOBase::system_error_text(err)));
	else
	{
		// Read the version - This forces credential passing
		OOBase::CDRStream stream;
		err = ptrSocket->recv(stream.buffer(),sizeof(Omega::uint32_t));
		if (err != 0)
			LOG_WARNING(("Receive failure: %s",OOBase::system_error_text(err)));
		else
		{
			// Check the versions are correct
			Omega::uint32_t version = 0;
			if (!stream.read(version) || version < ((OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16)))
				LOG_WARNING(("Unsupported version received: %u",version));
			else
			{
				OOBase::StackAllocator<512> allocator;
				OOBase::LocalString strSid(allocator);
				//if (!stream.recv_string(ptrSocket,strSid))
				//	LOG_ERROR(("Failed to retrieve client session id: %s",OOBase::system_error_text(stream.last_error())));
				//else
				{
					uid_t uid;
					//err = ptrSocket->get_uid(uid);
					if (err != 0)
						LOG_ERROR(("Failed to retrieve client token: %s",OOBase::system_error_text(err)));
					else
					{
						UserProcess user_process;
						if (get_user_process(uid,strSid,user_process))
						{
							if (!stream.write_string(user_process.m_strPipe))
								LOG_ERROR(("Failed to write to client: %s",OOBase::system_error_text(stream.last_error())));
							else
								ptrSocket->send(stream.buffer());
						}

					#if defined(_WIN32)
						// Make sure the handle is closed
						CloseHandle(uid);
					#endif
					}
				}
			}
		}
	}
}
