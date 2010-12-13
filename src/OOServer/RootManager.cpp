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
//  Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"
#include "RootManager.h"
#include "SpawnedProcess.h"
#include "Protocol.h"

#include <signal.h>

static std::string getenv_i(const char* val)
{
#if defined(_MSC_VER) && defined(_CRT_INSECURE_DEPRECATE)
	char* buf = 0;
	size_t len = 0;
	std::string ret;
	if (!_dupenv_s(&buf,&len,val))
	{
		if (len)
			ret = std::string(buf,len-1);
		free(buf);
	}
	return ret;
#else
	return getenv(val);
#endif
}

Root::Manager::Manager(const std::map<std::string,std::string>& args) :
		m_cmd_args(args),
		m_sandbox_channel(0),
		m_uNextSocketId(0)
{
	// Root channel is fixed
	set_channel(0x80000000,0x80000000,0x7F000000,0);
}

Root::Manager::~Manager()
{
}

int Root::Manager::run()
{
#if !defined(_WIN32)
	// Ignore SIGPIPE
	if (::signal(SIGPIPE,SIG_IGN) == SIG_ERR)
		LOG_ERROR(("signal(SIGPIPE) failed: %s",OOBase::strerror(errno).c_str()));
#endif

	std::string strPidfile = "/var/run/ooserverd.pid";
	std::map<std::string,std::string>::const_iterator f = m_cmd_args.find("pidfile");
	if (f != m_cmd_args.end())
		strPidfile = f->second;

	if (!pid_file(strPidfile.c_str()))
		return EXIT_FAILURE;

	try
	{
		// Loop until we quit
		for (bool bQuit=false; !bQuit;)
		{
			// Load the config
			if (!load_config())
				return EXIT_FAILURE;

			// Open the root registry
			if (!init_database())
				return EXIT_FAILURE;

			// Start the handler
			if (!start_request_threads())
				return EXIT_FAILURE;

			// Just so we can exit cleanly...
			bool bOk = false;

			// Spawn the sandbox
			if (spawn_sandbox())
			{
				// Start listening for clients
				if (m_client_acceptor.start(this))
				{
					bOk = true;

					// Wait for quit
					bQuit = wait_to_quit();

					// Stop accepting new clients
					m_client_acceptor.stop();
				}

				// Stop services
				stop_services();

				// Close all channels
				close_channels();
			}

			// Stop the MessageHandler
			stop_request_threads();

			if (!bOk)
			{
#if defined(OMEGA_DEBUG)
				// Give us a chance to read the errors!
				OOBase::Thread::sleep(OOBase::timeval_t(5,0));
#endif
				return EXIT_FAILURE;
			}
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_RETURN(("std::exception thrown %s",e.what()),EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}

bool Root::Manager::init_database()
{
	// Get dir from config
	std::map<std::string,std::string>::const_iterator i=m_config_args.find("regdb_path");
	if (i == m_config_args.end())
		LOG_ERROR_RETURN(("Missing 'regdb_path' config setting"),false);

	std::string dir = i->second;
	if (!dir.empty())
	{
#if defined(_WIN32)
		std::replace(dir.begin(),dir.end(),'/','\\');
		if (*dir.rbegin() != '\\')
			dir += '\\';
#else
		std::replace(dir.begin(),dir.end(),'\\','/');
		if (*dir.rbegin() != '/')
			dir += '/';
#endif
	}

	// Create a new system database
	OOBASE_NEW(m_registry,Registry::Hive(this,dir + "system.regdb"));
	if (!m_registry)
		LOG_ERROR_RETURN(("Out of memory"),false);

	if (!m_registry->open(SQLITE_OPEN_READWRITE))
		return false;

	// Create a new System database
	OOBASE_NEW(m_registry_sandbox,Registry::Hive(this,dir + "sandbox.regdb"));
	if (!m_registry_sandbox)
		LOG_ERROR_RETURN(("Out of memory"),false);

	return m_registry_sandbox->open(SQLITE_OPEN_READWRITE);
}

bool Root::Manager::load_config_file(const std::string& strFile)
{
	// Load simple config file...
	std::ifstream fs(strFile.c_str());
	if (!fs.is_open())
		LOG_ERROR_RETURN(("Failed to open config file: %s",strFile.c_str()),false);

	while (!fs.eof())
	{
		// Read line
		std::string line;
		std::getline(fs,line);

		if (!line.empty() && line[0] != '#')
		{
			// Read line as key=value
			std::string key,value;
			size_t pos = line.find('=');
			if (pos == std::string::npos)
			{
				key = line;
				value = "true";
			}
			else
			{
				key = line.substr(0,pos);
				value = line.substr(pos+1);
			}

			// Insert into map
			m_config_args[key] = value;
		}
	}

	return true;
}

bool Root::Manager::spawn_sandbox()
{
	bool bUnsafe = (m_cmd_args.find("unsafe") != m_cmd_args.end());

	// Get username from config
	std::string strUName;
	std::map<std::string,std::string>::const_iterator i=m_config_args.find("sandbox_uname");
	if (i != m_config_args.end())
		strUName = i->second.c_str();

	bool bAgain = false;
	OOSvrBase::AsyncLocalSocket::uid_t uid = OOSvrBase::AsyncLocalSocket::uid_t(-1);
	if (strUName.empty())
	{
		if (!bUnsafe)
			LOG_ERROR_RETURN(("Failed to find the 'sandbox_uname' setting in the config"),false);

		std::string strOurUName;
		if (!get_our_uid(uid,strOurUName))
			return false;
		
		// Warn!
		OOSvrBase::Logger::log(OOSvrBase::Logger::Warning,
			"Failed to find the 'sandbox_uname' setting in the config.\n\n"
			"Because the 'unsafe' mode is set the sandbox process will be started under the current user account '%s'.\n\n"
			"This is a security risk and should only be allowed for debugging purposes, and only then if you really know what you are doing.\n",
			strOurUName.c_str());		
	}
	else 
	{
		if (!get_sandbox_uid(strUName,uid,bAgain) && bAgain && bUnsafe)
		{
			std::string strOurUName;
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

	std::string strPipe;
	m_sandbox_channel = spawn_user(uid,m_registry_sandbox,true,strPipe,bAgain);
	if (m_sandbox_channel == 0 && bUnsafe && !strUName.empty() && bAgain)
	{
		std::string strOurUName;
		if (!get_our_uid(uid,strOurUName))
			return false;

		OOSvrBase::Logger::log(OOSvrBase::Logger::Warning,
							   APPNAME " is running under a user account that does not have the priviledges required to create new processes as a different user.\n\n"
							   "Because the 'unsafe' mode is set the sandbox process will be started under the current user account '%s'.\n\n"
							   "This is a security risk and should only be allowed for debugging purposes, and only then if you really know what you are doing.\n",
							   strOurUName.c_str());

		m_sandbox_channel = spawn_user(uid,m_registry_sandbox,true,strPipe,bAgain);
	}

#if defined(_WIN32)
	CloseHandle(uid);
#endif
	
	return (m_sandbox_channel != 0);	
}

bool Root::Manager::wait_to_quit()
{
	for (std::string debug = getenv_i("OMEGA_DEBUG");;)
	{
		switch (wait_for_quit())
		{
#if defined (_WIN32)
			case CTRL_BREAK_EVENT:
				return (debug=="yes");

			default:
				return true;
#elif defined(HAVE_UNISTD_H)
			case SIGHUP:
				return (debug=="yes");
				
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
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		m_mapUserProcesses.erase(channel);
	}
	catch (std::exception& e)
	{
		LOG_ERROR(("std::exception thrown %s",e.what()));
	}
}

bool Root::Manager::get_user_process(OOSvrBase::AsyncLocalSocket::uid_t& uid, UserProcess& user_process)
{
	try
	{
		bool bUnsafe = (m_cmd_args.find("unsafe") != m_cmd_args.end());

		for (bool bFirst = true;bFirst;bFirst = false)
		{
			// See if we have a process already
			OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

			for (std::map<Omega::uint32_t,UserProcess>::const_iterator i=m_mapUserProcesses.begin(); i!=m_mapUserProcesses.end(); ++i)
			{
				if (i->second.ptrSpawn->Compare(uid))
				{
					user_process = i->second;
					return true;
				}
				else if (i->second.ptrSpawn->IsSameUser(uid))
				{
					user_process.ptrRegistry = i->second.ptrRegistry;
				}
			}

			guard.release();

			// Spawn a new user process
			bool bAgain = false;
			if (spawn_user(uid,user_process.ptrRegistry,false,user_process.strPipe,bAgain) != 0)
				return true;

			if (bFirst && bAgain && bUnsafe)
			{
				std::string strOurUName;
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
	catch (std::exception& e)
	{
		LOG_ERROR_RETURN(("std::exception thrown %s",e.what()),false);
	}
}

Omega::uint32_t Root::Manager::spawn_user(OOSvrBase::AsyncLocalSocket::uid_t uid, OOBase::SmartPtr<Registry::Hive> ptrRegistry, bool bSandbox, std::string& strPipe, bool& bAgain)
{
	// Do a platform specific spawn
	Omega::uint32_t channel_id = 0;
	OOBase::SmartPtr<OOServer::MessageConnection> ptrMC;

	UserProcess process;
	process.ptrSpawn = platform_spawn(uid,bSandbox,strPipe,channel_id,ptrMC,bAgain);
	if (!process.ptrSpawn)
		return 0;

	process.ptrRegistry = ptrRegistry;
	process.strPipe = strPipe;

	// Init the registry, if necessary
	bool bOk = true;
	if (!process.ptrRegistry)
	{
		bOk = false;

		std::string strHive;
		if (process.ptrSpawn->GetRegistryHive(m_config_args["regdb_path"],m_config_args["users_path"],strHive))
		{
			// Create a new database
			OOBASE_NEW(process.ptrRegistry,Registry::Hive(this,strHive));
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
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		// Check we haven't created a duplicate while we spawned...
		for (std::map<Omega::uint32_t,UserProcess>::iterator i=m_mapUserProcesses.begin(); i!=m_mapUserProcesses.end(); ++i)
		{
			if (i->second.ptrSpawn->Compare(uid))
			{
				ptrMC->close();
				return i->first;
			}
		}

		m_mapUserProcesses.insert(std::map<Omega::uint32_t,UserProcess>::value_type(channel_id,process));
	}
	catch (std::exception& e)
	{
		ptrMC->close();
		LOG_ERROR_RETURN(("std::exception thrown %s",e.what()),0);
	}

	// Now start the read cycle from ptrMC
	return (ptrMC->read() ? channel_id : 0);
}

Omega::uint32_t Root::Manager::bootstrap_user(OOBase::SmartPtr<OOSvrBase::AsyncLocalSocket>& ptrSocket, OOBase::SmartPtr<OOServer::MessageConnection>& ptrMC, std::string& strPipe)
{
	OOBase::CDRStream stream;
	if (!stream.write(m_sandbox_channel))
		LOG_ERROR_RETURN(("CDRStream::write failed: %s",OOBase::system_error_text(stream.last_error()).c_str()),0);

	int err = ptrSocket->send(stream.buffer());
	if (err != 0)
		LOG_ERROR_RETURN(("Socket::send failed: %s",OOBase::system_error_text(err).c_str()),0);

	stream.reset();

	// We know a CDRStream writes strings as a 4 byte length followed by the character data
	size_t mark = stream.buffer()->mark_rd_ptr();
	err = ptrSocket->recv(stream.buffer(),4);
	if (err != 0)
		LOG_ERROR_RETURN(("Socket::recv failed: %s",OOBase::system_error_text(err).c_str()),0);

	Omega::uint32_t len = 0;
	if (!stream.read(len))
		LOG_ERROR_RETURN(("CDRStream::read failed: %s",OOBase::system_error_text(stream.last_error()).c_str()),0);

	err = ptrSocket->recv(stream.buffer(),len);
	if (err != 0)
		LOG_ERROR_RETURN(("Socket::recv failed: %s",OOBase::system_error_text(err).c_str()),0);

	// Now reset rd_ptr and read the string
	stream.buffer()->mark_rd_ptr(mark);
	if (!stream.read(strPipe))
		LOG_ERROR_RETURN(("CDRStream::read failed: %s",OOBase::system_error_text(stream.last_error()).c_str()),0);

	OOBASE_NEW(ptrMC,OOServer::MessageConnection(this,ptrSocket));
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
		LOG_ERROR_RETURN(("CDRStream::write failed: %s",OOBase::system_error_text(stream.last_error()).c_str()),0);
	}

	return (ptrMC->send(stream.buffer()) ? channel_id : 0);
}

void Root::Manager::process_request(OOBase::CDRStream& request, Omega::uint32_t seq_no, Omega::uint32_t src_channel_id, Omega::uint16_t src_thread_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs)
{
	OOServer::RootOpCode_t op_code;
	request.read(op_code);

	if (request.last_error() != 0)
	{
		LOG_ERROR(("Bad request: %s",OOBase::system_error_text(request.last_error()).c_str()));
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

OOServer::MessageHandler::io_result::type Root::Manager::sendrecv_sandbox(const OOBase::CDRStream& request, OOBase::SmartPtr<OOBase::CDRStream>& response, const OOBase::timeval_t* deadline, Omega::uint16_t attribs)
{
	return send_request(m_sandbox_channel,&request,response,deadline,attribs);
}
