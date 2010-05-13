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

Root::Manager::Manager(const std::map<std::string,std::string>& args) :
		m_cmd_args(args),
		m_sandbox_channel(0)
{
	// Root channel is fixed
	set_channel(0x80000000,0x80000000,0x7F000000,0);
}

Root::Manager::~Manager()
{
}

int Root::Manager::run()
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
		std::string strPipe;
		m_sandbox_channel = spawn_user(OOBase::LocalSocket::uid_t(-1),0,strPipe);
		if (m_sandbox_channel)
		{
			// Start listening for clients
			if (m_client_acceptor.start(this))
			{
				bOk = true;
				bQuit = wait_for_quit();

				// Stop accepting new clients
				m_client_acceptor.stop();
			}

			// Close all channels
			close_channels();
		}

		// Stop the MessageHandler
		stop_request_threads();

		if (!bOk)
			return EXIT_FAILURE;
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
	OOBASE_NEW(m_registry,Registry::Hive(this,dir + "system.regdb",Registry::Hive::write_check | Registry::Hive::read_check));
	if (!m_registry)
		LOG_ERROR_RETURN(("Out of memory"),false);

	if (!m_registry->open(SQLITE_OPEN_READWRITE))
		return false;

	// Create a new System database
	OOBASE_NEW(m_registry_sandbox,Registry::Hive(this,dir + "sandbox.regdb",Registry::Hive::write_check));
	if (!m_registry_sandbox)
		LOG_ERROR_RETURN(("Out of memory"),false);

	return m_registry_sandbox->open(SQLITE_OPEN_READWRITE);
}

bool Root::Manager::load_config_file(const std::string& strFile)
{
	// Load simple config file...
	try
	{
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
	catch (std::exception& e)
	{
		LOG_ERROR_RETURN(("std::exception thrown %s",e.what()),false);
	}
}

bool Root::Manager::can_route(Omega::uint32_t src_channel, Omega::uint32_t dest_channel)
{
	// Don't route to null channels
	if (!src_channel || !dest_channel)
		return false;

	// Only route to or from the sandbox
	return (src_channel == m_sandbox_channel || dest_channel == m_sandbox_channel);
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

std::string Root::Manager::get_user_pipe(OOBase::LocalSocket::uid_t uid)
{
	try
	{
		OOBase::SmartPtr<Registry::Hive> ptrRegistry;

		try
		{
			// See if we have a process already
			OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

			for (std::map<Omega::uint32_t,UserProcess>::iterator i=m_mapUserProcesses.begin(); i!=m_mapUserProcesses.end(); ++i)
			{
				if (i->second.ptrSpawn->Compare(uid))
				{
					return i->second.strPipe;
				}
				else if (i->second.ptrSpawn->IsSameUser(uid))
				{
					ptrRegistry = i->second.ptrRegistry;
				}
			}
		}
		catch (std::exception& e)
		{
			LOG_ERROR_RETURN(("std::exception thrown %s",e.what()),"");
		}

		// Spawn a new user process
		std::string strPipe;
		if (!spawn_user(uid,ptrRegistry,strPipe))
			return "";

		return strPipe;
	}
	catch (std::exception& e)
	{
		LOG_ERROR_RETURN(("std::exception thrown %s",e.what()),"");
	}
}

Omega::uint32_t Root::Manager::spawn_user(OOBase::LocalSocket::uid_t uid, OOBase::SmartPtr<Registry::Hive> ptrRegistry, std::string& strPipe)
{
	// Do a platform specific spawn
	Omega::uint32_t channel_id = 0;
	OOBase::SmartPtr<MessageConnection> ptrMC;

	UserProcess process;
	process.ptrSpawn = platform_spawn(uid,strPipe,channel_id,ptrMC);
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
			OOBASE_NEW(process.ptrRegistry,Registry::Hive(this,strHive,0));
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
				strPipe = i->second.strPipe;
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
	if (!ptrMC->read())
	{
		ptrMC->close();
		return 0;
	}

	return channel_id;
}

Omega::uint32_t Root::Manager::bootstrap_user(OOBase::Socket* pSocket, OOBase::SmartPtr<MessageConnection>& ptrMC, std::string& strPipe)
{
	int err = pSocket->send(m_sandbox_channel);
	if (err != 0)
		LOG_ERROR_RETURN(("Socket::send failed: %s",OOSvrBase::Logger::format_error(err).c_str()),0);

	size_t uLen = 0;
	err = pSocket->recv(uLen);
	if (err != 0)
		LOG_ERROR_RETURN(("Socket::recv failed: %s",OOSvrBase::Logger::format_error(err).c_str()),0);

	OOBase::SmartPtr<char,OOBase::ArrayDestructor<char> > buf = 0;
	OOBASE_NEW(buf,char[uLen]);
	if (!buf)
		LOG_ERROR_RETURN(("Out of memory"),0);

	pSocket->recv(buf,uLen,&err);
	if (err != 0)
		LOG_ERROR_RETURN(("Socket::recv failed: %s",OOSvrBase::Logger::format_error(err).c_str()),0);

	strPipe = buf;

	OOBASE_NEW(ptrMC,MessageConnection(this));
	if (!ptrMC)
		LOG_ERROR_RETURN(("Out of memory"),0);

	Omega::uint32_t channel_id = register_channel(ptrMC,0);
	if (!channel_id)
		return 0;

	err = pSocket->send(channel_id);
	if (err != 0)
		LOG_ERROR_RETURN(("Socket::send failed: %s",OOSvrBase::Logger::format_error(err).c_str()),0);

	return channel_id;
}

void Root::Manager::process_request(OOBase::CDRStream& request, Omega::uint32_t seq_no, Omega::uint32_t src_channel_id, Omega::uint16_t src_thread_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs)
{
	RootOpCode_t op_code;
	request.read(op_code);

	if (request.last_error() != 0)
	{
		LOG_ERROR(("Bad request: %s",OOSvrBase::Logger::format_error(request.last_error()).c_str()));
		return;
	}

	OOBase::CDRStream response;
	switch (op_code)
	{
	case KeyExists:
		registry_key_exists(src_channel_id,request,response);
		break;

	case CreateKey:
		registry_create_key(src_channel_id,request,response);
		break;

	case DeleteKey:
		registry_delete_key(src_channel_id,request,response);
		break;

	case EnumSubKeys:
		registry_enum_subkeys(src_channel_id,request,response);
		break;

	case ValueType:
		registry_value_type(src_channel_id,request,response);
		break;

	case GetStringValue:
		registry_get_string_value(src_channel_id,request,response);
		break;

	case GetIntegerValue:
		registry_get_int_value(src_channel_id,request,response);
		break;

	case GetBinaryValue:
		registry_get_binary_value(src_channel_id,request,response);
		break;

	case SetStringValue:
		registry_set_string_value(src_channel_id,request,response);
		break;

	case SetIntegerValue:
		registry_set_int_value(src_channel_id,request,response);
		break;

	case SetBinaryValue:
		registry_set_binary_value(src_channel_id,request,response);
		break;

	case GetDescription:
		registry_get_description(src_channel_id,request,response);
		break;

	case GetValueDescription:
		registry_get_value_description(src_channel_id,request,response);
		break;

	case SetDescription:
		registry_set_description(src_channel_id,request,response);
		break;

	case SetValueDescription:
		registry_set_value_description(src_channel_id,request,response);
		break;

	case EnumValues:
		registry_enum_values(src_channel_id,request,response);
		break;

	case DeleteValue:
		registry_delete_value(src_channel_id,request,response);
		break;

	case OpenMirrorKey:
		registry_open_mirror_key(src_channel_id,request,response);
		break;

	default:
		response.write(EINVAL);
		LOG_ERROR(("Bad request op_code: %d",op_code));
		break;
	}

	if ((response.last_error() == 0) && !(attribs & 1))
		send_response(seq_no,src_channel_id,src_thread_id,response,deadline,attribs);
}
