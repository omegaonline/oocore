///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
// Portions Copyright (c) 1996-2009, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
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
#include "SpawnedProcess.h"
#include "Protocol.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

namespace
{
	bool getenv_OMEGA_DEBUG()
	{
		OOBase::LocalString str;
		str.getenv("OMEGA_DEBUG");
		return (str == "yes");
	}
}

Root::Manager::Manager() :
		m_bUnsafe(false),
		m_sandbox_channel(0),
		m_mapSockets(1)
{
	// Root channel is fixed
	set_channel(0x80000000,0x80000000,0x7F000000,0);
}

Root::Manager::~Manager()
{
}

int Root::Manager::run(const OOBase::CmdArgs::results_t& cmd_args)
{
	m_bUnsafe = (cmd_args.find("unsafe") != cmd_args.npos);

	OOBase::String strPidfile;
	cmd_args.find("pidfile",strPidfile);

	if (!pid_file(strPidfile.empty() ? "/var/run/ooserverd.pid" : strPidfile.c_str()))
		return EXIT_FAILURE;

	// Loop until we quit
	for (bool bQuit=false; !bQuit;)
	{
		// Load the config
		if (!load_config(cmd_args))
			return EXIT_FAILURE;

		// Open the root registry
		if (!init_database())
			return EXIT_FAILURE;

		// Start the proactor pool
		int err = m_proactor_pool.run(run_proactor,NULL,2);
		if (err != 0)
		{
			m_proactor_pool.join();
			LOG_ERROR(("Thread pool create failed: %s",OOBase::system_error_text(err)));
			return EXIT_FAILURE;
		}

		// Start the handler
		if (!start_request_threads(2))
		{
			stop_request_threads();
			return EXIT_FAILURE;
		}

		// Just so we can exit cleanly...
		bool bOk = false;

		// Spawn the sandbox
		if (spawn_sandbox())
		{
			// Start listening for clients
			if (start_client_acceptor())
			{
				bOk = true;

				// Wait for quit
				bQuit = wait_to_quit();

				// Stop accepting new clients
				m_client_acceptor->stop();
			}

			// Stop services
			stop_services();

			// Close all channels
			close_channels();
		}

		// Stop the MessageHandler
		stop_request_threads();

		// Stop any proactor threads
		m_proactor_pool.join();

		if (!bOk)
		{
#if defined(OMEGA_DEBUG)
			// Give us a chance to read the errors!
			OOBase::Thread::sleep(OOBase::timeval_t(5,0));
#endif
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}

bool Root::Manager::init_database()
{
	// Get dir from config
	OOBase::String dir;
	if (!m_config_args.find("regdb_path",dir) || dir.empty())
		LOG_ERROR_RETURN(("Missing 'regdb_path' config setting"),false);

#if defined(_WIN32)
	dir.replace('/','\\');
#else
	dir.replace('\\','/');
#endif

	int err = OOBase::AppendDirSeparator(dir);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to append string: %s",OOBase::system_error_text()),false);

	// Create a new system database
	OOBase::String dir2 = dir;
	if ((err = dir2.append("system.regdb")) != 0)
		LOG_ERROR_RETURN(("Failed to append string: %s",OOBase::system_error_text()),false);

	m_registry = new (std::nothrow) Registry::Hive(this,dir2.c_str());
	if (!m_registry)
		LOG_ERROR_RETURN(("Out of memory"),false);

	if (!m_registry->open(SQLITE_OPEN_READWRITE))
		return false;

	// Create a new System database
	dir2 = dir;
	if ((err = dir2.append("sandbox.regdb")) != 0)
		LOG_ERROR_RETURN(("Failed to append string: %s",OOBase::system_error_text()),false);

	m_registry_sandbox = new (std::nothrow) Registry::Hive(this,dir2.c_str());
	if (!m_registry_sandbox)
		LOG_ERROR_RETURN(("Out of memory"),false);

	return m_registry_sandbox->open(SQLITE_OPEN_READWRITE);
}

bool Root::Manager::load_config_file(const char* pszFile)
{
	// Load simple config file...
	FILE* f = NULL;
#if defined(_MSC_VER)
	int err = fopen_s(&f,pszFile,"r");
#else
	int err = 0;
	f = fopen(pszFile,"r");
	if (!f)
		err = errno;
#endif
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to open file %s: %s",pszFile,OOBase::system_error_text(err)),false);

	OOBase::LocalString strBuffer;
	for (bool bEof=false;!bEof && err==0;)
	{
		// Read some characters
		char szBuf[256] = {0};
		size_t r = fread(szBuf,1,sizeof(szBuf),f);
		if (r != sizeof(szBuf))
		{
			if (feof(f))
				bEof = true;
			else
			{
				err = ferror(f);
				LOG_ERROR(("Failed to read file %s: %s",pszFile,OOBase::system_error_text(err)));
				break;
			}
		}

		// Append to buffer
		err = strBuffer.append(szBuf,r);

		// Split out individual lines
		for (size_t start = 0;err==0 && !strBuffer.empty();)
		{
			// Skip leading whitespace
			while (strBuffer[start] == '\t' || strBuffer[start] == ' ')
				++start;

			if (start == strBuffer.length())
			{
				strBuffer.clear();
				break;
			}

			// Find the next linefeed
			size_t end = strBuffer.find('\n',start);
			if (end == OOBase::String::npos)
			{
				if (!bEof)
				{
					// Incomplete line
					break;
				}

				end = strBuffer.length();
			}

			// Skip everything after #
			size_t hash = strBuffer.find('#',start);

			// Trim trailing whitespace
			size_t valend = (hash != OOBase::String::npos ? hash : end);
			while (valend > start && (strBuffer[valend-1] == '\t' || strBuffer[valend-1] == ' '))
				--valend;

			if (valend > start)
			{
				OOBase::String strKey, strValue;

				// Split on first =
				size_t eq = strBuffer.find('=',start);
				if (eq != OOBase::String::npos)
				{
					// Trim trailing whitespace before =
					size_t keyend = eq;
					while (keyend > start && (strBuffer[keyend-1] == '\t' || strBuffer[keyend-1] == ' '))
						--keyend;

					if (keyend > start)
					{
						if ((err = strKey.assign(strBuffer.c_str() + start,keyend-start)) != 0)
						{
							LOG_ERROR(("Failed to assign string: %s",OOBase::system_error_text(err)));
							break;
						}

						// Skip leading whitespace after =
						size_t valpos = eq+1;
						while (valpos < valend && (strBuffer[valpos] == '\t' || strBuffer[valpos] == ' '))
							++valpos;

						if (valpos < valend)
						{
							if ((err = strValue.assign(strBuffer.c_str() + valpos,valend-valpos)) != 0)
							{
								LOG_ERROR(("Failed to assign string: %s",OOBase::system_error_text(err)));
								break;
							}
						}
					}
				}
				else
				{
					err = strKey.assign(strBuffer.c_str() + start,valend-start);
					if (err == 0)
						err = strValue.assign("true",4);

					if (err != 0)
					{
						LOG_ERROR(("Failed to assign string: %s",OOBase::system_error_text(err)));
						break;
					}
				}

				// Do something with strKey and strValue
				if (!strKey.empty() && (err = m_config_args.replace(strKey,strValue)) != 0)
					LOG_ERROR(("Failed to insert config string: %s",OOBase::system_error_text(err)));
			}

			if (end == OOBase::LocalString::npos)
				break;

			start = end + 1;
		}
	}

	fclose(f);

	return (err == 0);
}

int Root::Manager::run_proactor(void*)
{
	int err = 0;
	return Proactor::instance().run(err);
}

bool Root::Manager::spawn_sandbox()
{
	// Get username from config
	OOBase::String strUName;
	if (!m_config_args.find("sandbox_uname",strUName))
		LOG_ERROR(("Failed to find the 'sandbox_uname' setting in the config"));

	bool bAgain = false;
	OOSvrBase::AsyncLocalSocket::uid_t uid = OOSvrBase::AsyncLocalSocket::uid_t(-1);
	if (strUName.empty())
	{
		if (!m_bUnsafe)
			LOG_ERROR_RETURN(("'sandbox_uname' setting in the config is empty!"),false);

		OOBase::LocalString strOurUName;
		if (!get_our_uid(uid,strOurUName))
			return false;

		// Warn!
		OOSvrBase::Logger::log(OOSvrBase::Logger::Warning,
			"Because the 'unsafe' mode is set the sandbox process will be started under the current user account '%s'.\n\n"
			"This is a security risk and should only be allowed for debugging purposes, and only then if you really know what you are doing.\n",
			strOurUName.c_str());
	}
	else if (!get_sandbox_uid(strUName,uid,bAgain))
	{
		if (bAgain && m_bUnsafe)
		{
			OOBase::LocalString strOurUName;
			if (!get_our_uid(uid,strOurUName))
				return false;

			OOSvrBase::Logger::log(OOSvrBase::Logger::Warning,
								   APPNAME " is running under a user account that does not have the priviledges required to impersonate a different user.\n\n"
								   "Because the 'unsafe' mode is set the sandbox process will be started under the current user account '%s'.\n\n"
								   "This is a security risk and should only be allowed for debugging purposes, and only then if you really know what you are doing.\n",
								   strOurUName.c_str());
		}
		else
			return false;
	}

	OOBase::String strPipe;
	m_sandbox_channel = spawn_user(uid,NULL,m_registry_sandbox,strPipe,bAgain);
	if (m_sandbox_channel == 0 && m_bUnsafe && !strUName.empty() && bAgain)
	{
		OOBase::LocalString strOurUName;
		if (!get_our_uid(uid,strOurUName))
			return false;

		OOSvrBase::Logger::log(OOSvrBase::Logger::Warning,
							   APPNAME " is running under a user account that does not have the priviledges required to create new processes as a different user.\n\n"
							   "Because the 'unsafe' mode is set the sandbox process will be started under the current user account '%s'.\n\n"
							   "This is a security risk and should only be allowed for debugging purposes, and only then if you really know what you are doing.\n",
							   strOurUName.c_str());

		m_sandbox_channel = spawn_user(uid,NULL,m_registry_sandbox,strPipe,bAgain);
	}

#if defined(_WIN32)
	CloseHandle(uid);
#endif

	return (m_sandbox_channel != 0);
}

bool Root::Manager::wait_to_quit()
{
	for (;;)
	{
		switch (wait_for_quit())
		{
#if defined (_WIN32)
			case CTRL_BREAK_EVENT:
				return getenv_OMEGA_DEBUG();

			default:
				return true;
#elif defined(HAVE_UNISTD_H)
			case SIGHUP:
				return getenv_OMEGA_DEBUG();

			case SIGQUIT:
			case SIGTERM:
				return true;

			default:
				break;
#else
#error Fix me!
#endif
		}
	}
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

	m_mapUserProcesses.erase(channel);
}

bool Root::Manager::get_user_process(OOSvrBase::AsyncLocalSocket::uid_t& uid, const char* session_id, UserProcess& user_process)
{
	for (bool bFirst = true;bFirst;bFirst = false)
	{
		// See if we have a process already
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

		OOBase::Stack<Omega::uint32_t,OOBase::LocalAllocator> vecDead;
		bool bFound = false;

		for (size_t i=m_mapUserProcesses.begin(); i!=m_mapUserProcesses.npos; i=m_mapUserProcesses.next(i))
		{
			UserProcess* pU = m_mapUserProcesses.at(i);
			if (!pU->ptrSpawn->IsRunning())
			{
				vecDead.push(*m_mapUserProcesses.key_at(i));
			}
			else if (pU->ptrSpawn->IsSameLogin(uid,session_id))
			{
				user_process = *pU;
				bFound = true;
			}
			else if (!bFound && pU->ptrSpawn->IsSameUser(uid))
			{
				user_process.ptrRegistry = pU->ptrRegistry;
			}
		}

		guard.release();

		if (!vecDead.empty())
		{
			OOBase::Guard<OOBase::RWMutex> guard(m_lock);

			for (Omega::uint32_t i = 0;vecDead.pop(&i);)
				m_mapUserProcesses.erase(i);
		}

		if (bFound)
			return true;

		// Spawn a new user process
		bool bAgain = false;
		if (spawn_user(uid,session_id,user_process.ptrRegistry,user_process.strPipe,bAgain) != 0)
			return true;

		if (bFirst && bAgain && m_bUnsafe)
		{
			OOBase::LocalString strOurUName;
			if (!get_our_uid(uid,strOurUName))
				return false;

			OOSvrBase::Logger::log(OOSvrBase::Logger::Warning,
								   APPNAME " is running under a user account that does not have the priviledges required to create new processes as a different user.\n\n"
								   "Because the 'unsafe' mode is set the new user process will be started under the current user account '%s'.\n\n"
								   "This is a security risk and should only be allowed for debugging purposes, and only then if you really know what you are doing.\n",
								   strOurUName.c_str());
		}
	}

	return false;
}

Omega::uint32_t Root::Manager::spawn_user(OOSvrBase::AsyncLocalSocket::uid_t uid, const char* session_id, const OOBase::SmartPtr<Registry::Hive>& ptrRegistry, OOBase::String& strPipe, bool& bAgain)
{
	// Do a platform specific spawn
	Omega::uint32_t channel_id = 0;
	OOBase::RefPtr<OOServer::MessageConnection> ptrMC;

	UserProcess process;
	process.ptrSpawn = platform_spawn(uid,session_id,strPipe,channel_id,ptrMC,bAgain);
	if (!process.ptrSpawn)
		return 0;

	process.ptrRegistry = ptrRegistry;
	process.strPipe = strPipe;

	// Init the registry, if necessary
	bool bOk = true;
	if (!process.ptrRegistry)
	{
		bOk = false;

		OOBase::String strRegPath, strUsersPath;
		m_config_args.find("regdb_path",strRegPath);
		m_config_args.find("users_path",strUsersPath);

		OOBase::LocalString strHive;
		if (process.ptrSpawn->GetRegistryHive(strRegPath,strUsersPath,strHive))
		{
			// Create a new database
			process.ptrRegistry = new (std::nothrow) Registry::Hive(this,strHive.c_str());
			if (!process.ptrRegistry)
				LOG_ERROR(("Out of memory"));
			else
				bOk = process.ptrRegistry->open(SQLITE_OPEN_READWRITE);
		}
	}

	if (!bOk)
	{
		ptrMC->close();
		return 0;
	}

	// Insert the data into the process map...
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	// Check we haven't created a duplicate while we spawned...
	if (session_id)
	{
		for (size_t i=m_mapUserProcesses.begin(); i!=m_mapUserProcesses.npos; i=m_mapUserProcesses.next(i))
		{
			if (m_mapUserProcesses.at(i)->ptrSpawn->IsSameLogin(uid,session_id))
			{
				ptrMC->close();
				return *m_mapUserProcesses.key_at(i);
			}
		}
	}

	int err = m_mapUserProcesses.insert(channel_id,process);
	if (err != 0)
	{
		ptrMC->close();
		LOG_ERROR_RETURN(("Failed to insert into map: %s",OOBase::system_error_text(err)),0);
	}

	// Now start the read cycle from ptrMC
	return (ptrMC->read() ? channel_id : 0);
}

Omega::uint32_t Root::Manager::bootstrap_user(OOBase::RefPtr<OOSvrBase::AsyncLocalSocket>& ptrSocket, OOBase::RefPtr<OOServer::MessageConnection>& ptrMC, OOBase::String& strPipe)
{
	OOBase::CDRStream stream;
	if (!stream.write(m_sandbox_channel))
		LOG_ERROR_RETURN(("CDRStream::write failed: %s",OOBase::system_error_text(stream.last_error())),0);

	int err = ptrSocket->send(stream.buffer());
	if (err != 0)
		LOG_ERROR_RETURN(("Socket::send failed: %s",OOBase::system_error_text(err)),0);

	// We know a CDRStream writes strings as a 4 byte length followed by the character data
	stream.reset();
	size_t mark = stream.buffer()->mark_rd_ptr();
	if ((err = ptrSocket->recv(stream.buffer(),4)) != 0)
		LOG_ERROR_RETURN(("Socket::recv failed: %s",OOBase::system_error_text(err)),0);

	Omega::uint32_t len = 0;
	if (!stream.read(len))
		LOG_ERROR_RETURN(("CDRStream::read failed: %s",OOBase::system_error_text(stream.last_error())),0);

	if ((err = ptrSocket->recv(stream.buffer(),len)) != 0)
		LOG_ERROR_RETURN(("Socket::recv failed: %s",OOBase::system_error_text(err)),0);

	// Now reset rd_ptr and read the string
	stream.buffer()->mark_rd_ptr(mark);
	OOBase::LocalString strPipeL;
	if (!stream.read(strPipeL))
		LOG_ERROR_RETURN(("CDRStream::read failed: %s",OOBase::system_error_text(stream.last_error())),0);

	if ((err = strPipe.assign(strPipeL.c_str())) != 0)
		LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),0);

	ptrMC = new (std::nothrow) OOServer::MessageConnection(this,ptrSocket);
	if (!ptrMC)
		LOG_ERROR_RETURN(("Out of memory"),0);

	Omega::uint32_t channel_id = register_channel(ptrMC,0);
	if (!channel_id)
	{
		ptrMC->close();
		return 0;
	}

	stream.reset();
	if (!stream.write(channel_id))
	{
		ptrMC->close();
		LOG_ERROR_RETURN(("CDRStream::write failed: %s",OOBase::system_error_text(stream.last_error())),0);
	}

	return (ptrMC->send(stream.buffer()) ? channel_id : 0);
}

void Root::Manager::process_request(OOBase::CDRStream& request, Omega::uint32_t seq_no, Omega::uint32_t src_channel_id, Omega::uint16_t src_thread_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs)
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
	case OOServer::KeyExists:
		registry_key_exists(src_channel_id,request,response);
		break;

	case OOServer::CreateKey:
		registry_create_key(src_channel_id,request,response);
		break;

	case OOServer::DeleteKey:
		registry_delete_key(src_channel_id,request,response);
		break;

	case OOServer::EnumSubKeys:
		registry_enum_subkeys(src_channel_id,request,response);
		break;

	case OOServer::ValueExists:
		registry_value_exists(src_channel_id,request,response);
		break;

	case OOServer::GetValue:
		registry_get_value(src_channel_id,request,response);
		break;

	case OOServer::SetValue:
		registry_set_value(src_channel_id,request,response);
		break;

	case OOServer::GetDescription:
		registry_get_description(src_channel_id,request,response);
		break;

	case OOServer::GetValueDescription:
		registry_get_value_description(src_channel_id,request,response);
		break;

	case OOServer::SetDescription:
		registry_set_description(src_channel_id,request,response);
		break;

	case OOServer::SetValueDescription:
		registry_set_value_description(src_channel_id,request,response);
		break;

	case OOServer::EnumValues:
		registry_enum_values(src_channel_id,request,response);
		break;

	case OOServer::DeleteValue:
		registry_delete_value(src_channel_id,request,response);
		break;

	case OOServer::OpenMirrorKey:
		registry_open_mirror_key(src_channel_id,request,response);
		break;

	case OOServer::ServicesStart:
		services_start(src_channel_id,response);
		break;

	case OOServer::GetServiceKey:
		get_service_key(src_channel_id,request,response);
		break;

	case OOServer::ListenSocket:
		listen_socket(src_channel_id,request,response);
		break;

	case OOServer::SocketRecv:
		socket_recv(src_channel_id,request,response);
		break;

	case OOServer::SocketSend:
		socket_send(src_channel_id,request,response);
		break;

	case OOServer::SocketClose:
		socket_close(src_channel_id,request);
		break;

	default:
		response.write(Omega::int32_t(EINVAL));
		LOG_ERROR(("Bad request op_code: %d",op_code));
		break;
	}

	if (response.last_error() == 0 && !(attribs & 1))
		send_response(seq_no,src_channel_id,src_thread_id,response,deadline,attribs);
}

OOServer::MessageHandler::io_result::type Root::Manager::sendrecv_sandbox(const OOBase::CDRStream& request, OOBase::CDRStream* response, const OOBase::timeval_t* deadline, Omega::uint16_t attribs)
{
	return send_request(m_sandbox_channel,&request,response,deadline,attribs);
}

void Root::Manager::accept_client(void* pThis, OOSvrBase::AsyncLocalSocket* pSocket, int err)
{
	static_cast<Manager*>(pThis)->accept_client_i(pSocket,err);
}
