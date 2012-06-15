///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
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

#include <stdlib.h>

#if defined(HAVE_UNISTD_H)
#include <netdb.h>
#endif

namespace
{
	bool get_service_dependencies(OOBase::SmartPtr<Db::Hive> ptrRegistry, const Omega::int64_t key, const OOBase::String& strName, OOBase::Queue<OOBase::String,OOBase::LocalAllocator>& queueNames, OOBase::Queue<Omega::int64_t,OOBase::LocalAllocator>& queueKeys)
	{
		OOBase::LocalString strSubKey,strLink,strFullKeyName;
		int err2 = strSubKey.assign(strName.c_str(),strName.length());
		if (err2)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err2)),false);

		Omega::int64_t subkey = key;
		Db::hive_errors err = ptrRegistry->create_key(key,subkey,strSubKey,0,0,strLink,strFullKeyName);
		if (err)
			LOG_ERROR_RETURN(("Failed to open the '/System/Services/%s' key in the registry",strName.c_str()),false);

		// Add the name to the name queue
		// This prevents circular dependencies
		err2 = queueNames.push(strName);
		if (!err2)
			err2 = queueKeys.push(subkey);
		if (err2)
			LOG_ERROR_RETURN(("Failed to push to queue: %s",OOBase::system_error_text(err2)),false);

		// Check dependencies
		err2 = strSubKey.assign("Dependencies");
		if (err2)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err2)),false);

		Omega::int64_t depskey = subkey;
		err = ptrRegistry->create_key(subkey,depskey,strSubKey,0,0,strLink,strFullKeyName);
		if (err && err != Db::HIVE_NOTFOUND)
			LOG_ERROR_RETURN(("Failed to open the '/System/Services/%s/Dependencies' key in the registry",strName.c_str()),false);
		else if (!err)
		{
			Db::Hive::registry_set_t names;
			err = ptrRegistry->enum_values(depskey,0,names);
			if (err)
				LOG_ERROR_RETURN(("Failed to enumerate the '/System/Services/%s/Dependencies' values in the user registry",strName.c_str()),false);

			for (OOBase::String strDep;names.pop(&strDep);)
			{
				if (!queueNames.find(strDep) && !get_service_dependencies(ptrRegistry,key,strDep,queueNames,queueKeys))
					return false;
			}
		}

		return true;
	}

	bool enum_services(OOBase::SmartPtr<Db::Hive> ptrRegistry, OOBase::Queue<OOBase::String,OOBase::LocalAllocator>& queueNames, OOBase::Queue<Omega::int64_t,OOBase::LocalAllocator>& queueKeys)
	{
		Omega::int64_t key = 0;
		OOBase::LocalString strSubKey,strLink,strFullKeyName;
		int err2 = strSubKey.assign("/System/Services");
		if (err2)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err2)),false);

		Db::hive_errors err = ptrRegistry->create_key(0,key,strSubKey,0,0,strLink,strFullKeyName);
		if (err)
		{
			if (err != Db::HIVE_NOTFOUND)
				LOG_ERROR_RETURN(("Failed to open the '/System/Services' key in the registry"),false);

			return true;
		}

		// Enum each service, building a queue of dependencies...
		Db::Hive::registry_set_t keys;
		err = ptrRegistry->enum_subkeys(key,0,keys);
		if (err)
			LOG_ERROR_RETURN(("Failed to enumerate the '/System/Services' values in the registry"),false);

		for (OOBase::String strName;keys.pop(&strName);)
		{
			if (!queueNames.find(strName) && !get_service_dependencies(ptrRegistry,key,strName,queueNames,queueKeys))
				return false;
		}

		return true;
	}

	bool split_string(const OOBase::LocalString& strSocket, size_t& pos, OOBase::LocalString& strSub)
	{
		int err = 0;
		size_t end = strSocket.find('/',pos);
		if (end == OOBase::LocalString::npos)
		{
			err = strSub.assign(strSocket.c_str()+pos);
			pos = end;
		}
		else
		{
			err = strSub.assign(strSocket.c_str()+pos,end-pos);
			pos = end + 1;
		}

		if (err)
			LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),false);

		return true;
	}

	bool create_and_forward_socket(pid_t pid, const OOBase::String& strName, const OOBase::String& strSocketName, const OOBase::LocalString& strValue, OOBase::RefPtr<OOBase::Socket> ptrSocket)
	{
		/* The format is:
		 *
		 * family/type/protocol[/address/port[/IPv6_scope_id]]
		 *
		 * Where:
		 *   family is one of: ipv4, ipv6, or numeric AF_ value
		 *   type is one of: stream, dgram, raw, or numeric SOCK_ value
		 *   protocol is one of: udp, tcp, or numeric PROTO_ value
		 *
		 *   address is optional, and if present is formatted for getaddrinfo(family)
		 *     using numeric formats only (AI_NUMERICHOST), and bind() is called.
		 *     If address is '*' then INADDR_ANY is used.
		 *
		 *   port is optional (mandatory if address is given), and if present is
		 *     formatted as a decimal number (AI_NUMERICSERV)
		 */

		addrinfo ai = {0};
		ai.ai_flags = AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV | AI_ADDRCONFIG;

		// Parse address family
		OOBase::LocalString strSub;
		size_t pos = 0;
		if (!split_string(strValue,pos,strSub))
			return false;

		if (strSub == "ipv4")
			ai.ai_family = AF_INET;
		else if (strSub == "ipv6")
			ai.ai_family = AF_INET6;
		else
		{
			char* end_ptr = NULL;
			ai.ai_family = strtoul(strSub.c_str(),&end_ptr,10);
			if (!ai.ai_family || *end_ptr != '\0')
				LOG_ERROR_RETURN(("Failed to parse family value in '/System/Services/%s/Connections' '%s' connection string: %s",strName.c_str(),strSocketName.c_str(),strSub.c_str()),false);
		}

		// Parse stream type
		if (!split_string(strValue,pos,strSub))
			return false;

		if (strSub == "stream")
			ai.ai_socktype = SOCK_STREAM;
		else if (strSub == "dgram")
			ai.ai_socktype = SOCK_DGRAM;
		else if (strSub == "raw")
			ai.ai_socktype = SOCK_RAW;
		else
		{
			char* end_ptr = NULL;
			ai.ai_socktype = strtoul(strSub.c_str(),&end_ptr,10);
			if (!ai.ai_socktype || *end_ptr != '\0')
				LOG_ERROR_RETURN(("Failed to parse type value in '/System/Services/%s/Connections' '%s' connection string: %s",strName.c_str(),strSocketName.c_str(),strSub.c_str()),false);
		}

		// Parse protocol
		if (!split_string(strValue,pos,strSub))
			return false;

		if (strSub == "udp")
			ai.ai_protocol = IPPROTO_UDP;
		else if (strSub == "tcp")
			ai.ai_protocol = IPPROTO_TCP;
		else
		{
			char* end_ptr = NULL;
			ai.ai_protocol = strtoul(strSub.c_str(),&end_ptr,10);
			if (!ai.ai_protocol || *end_ptr != '\0')
				LOG_ERROR_RETURN(("Failed to parse protocol value '/System/Services/%s/Connections' '%s' connection string: %s",strName.c_str(),strSocketName.c_str(),strSub.c_str()),false);
		}

		// Address
		if (!split_string(strValue,pos,strSub))
			return false;

		// Create the socket
		int err = 0;
		OOBase::socket_t new_sock = OOBase::Net::open_socket(ai.ai_family,ai.ai_socktype,ai.ai_protocol,err);
		if (err)
			LOG_ERROR_RETURN(("Failed to create socket: %s",OOBase::system_error_text(err)),false);

		// Bind if required
		if (!strSub.empty())
		{
			OOBase::LocalString strPort;
			if (!split_string(strValue,pos,strPort))
			{
				OOBase::Net::close_socket(new_sock);
				return false;
			}

			if (strSub == "*" && (ai.ai_family == AF_INET || ai.ai_family == AF_INET6))
			{
				if (ai.ai_family == AF_INET)
				{
					sockaddr_in addr = {0};
					addr.sin_family = AF_INET;
					addr.sin_addr.s_addr = INADDR_ANY;

					char* end_ptr = NULL;
					ushort port = strtoull(strPort.c_str(),&end_ptr,10);
					if (*end_ptr != '\0')
					{
						OOBase::Net::close_socket(new_sock);
						LOG_ERROR_RETURN(("Failed to parse port value '/System/Services/%s/Connections' '%s' connection string: %s",strName.c_str(),strSocketName.c_str(),strPort.c_str()),false);
					}

					addr.sin_port = htons(port);

					err = OOBase::Net::bind(new_sock,(struct sockaddr*)&addr,sizeof(addr));
				}
				else if (ai.ai_family == AF_INET6)
				{
					sockaddr_in6 addr = {0};
					addr.sin6_family = AF_INET6;

					in6_addr any = IN6ADDR_ANY_INIT;
					addr.sin6_addr = any;

					char* end_ptr = NULL;
					ushort port = strtoull(strPort.c_str(),&end_ptr,10);
					if (*end_ptr != '\0')
					{
						OOBase::Net::close_socket(new_sock);
						LOG_ERROR_RETURN(("Failed to parse port value '/System/Services/%s/Connections' '%s' connection string: %s",strName.c_str(),strSocketName.c_str(),strPort.c_str()),false);
					}

					addr.sin6_port = htons(port);

					err = OOBase::Net::bind(new_sock,(struct sockaddr*)&addr,sizeof(addr));
				}
			}
			else
			{
				addrinfo* addr = NULL;
				err = getaddrinfo(strSub.c_str(),strPort.c_str(),&ai,&addr);
				if (err)
				{
					OOBase::Net::close_socket(new_sock);
					LOG_ERROR_RETURN(("Failed to parse address value '/System/Services/%s/Connections' '%s' connection string, %s/%s: %s",strName.c_str(),strSocketName.c_str(),strSub.c_str(),strPort.c_str(),gai_strerror(err)),false);
				}

				if (addr->ai_family == AF_INET6)
				{
					sockaddr_in6* addr6 = reinterpret_cast<sockaddr_in6*>(addr->ai_addr);
					if (IN6_IS_ADDR_LINKLOCAL(&addr6->sin6_addr))
					{
						// If we are a link-local IPv6 address, we need a scope id
						if (!split_string(strValue,pos,strSub))
						{
							OOBase::Net::close_socket(new_sock);
							return false;
						}

						// Add support for named adaptors...
						void* TODO;

						char* end_ptr = NULL;
						unsigned long scope = strtoull(strSub.c_str(),&end_ptr,10);
						if (*end_ptr != '\0')
						{
							OOBase::Net::close_socket(new_sock);
							LOG_ERROR_RETURN(("Failed to parse scope-id value '/System/Services/%s/Connections' '%s' connection string: %s",strName.c_str(),strSocketName.c_str(),strSub.c_str()),false);
						}

						addr6->sin6_scope_id = htonl(scope);
					}
				}

				OOBase::Logger::log(OOBase::Logger::Information,"Binding socket %s for service '%s'",strValue.c_str(),strName.c_str());

				err = OOBase::Net::bind(new_sock,addr->ai_addr,addr->ai_addrlen);

				freeaddrinfo(addr);
			}

			if (err)
			{
				OOBase::Net::close_socket(new_sock);
				LOG_ERROR_RETURN(("Failed to bind socket value to '/System/Services/%s/Connections' '%s' connection string: %s",strName.c_str(),strSocketName.c_str(),OOBase::system_error_text(err)),false);
			}
		}

		// Send to service
		err = ptrSocket->send(Omega::uint32_t(strSocketName.length()));
		if (!err)
			ptrSocket->send(strSocketName.c_str(),Omega::uint32_t(strSocketName.length()),err);
		if (!err)
			err = ptrSocket->send_socket(new_sock,pid);

		OOBase::Net::close_socket(new_sock);
		if (err)
			LOG_ERROR_RETURN(("Failed to send socket to %s: %s",strName.c_str(),OOBase::system_error_text(err)),false);

		return true;
	}

	void enum_sockets(OOBase::SmartPtr<Db::Hive> ptrRegistry, const OOBase::String& strName, OOBase::RefPtr<OOBase::Socket> ptrSocket, const Omega::int64_t& key)
	{
		pid_t pid = 0;
#if defined(_WIN32)
		int err3 = ptrSocket->recv(pid);
		if (err3)
		{
			LOG_ERROR(("Failed to read process id from service socket: %s",OOBase::system_error_text(err3)));
			return;
		}
#endif

		Omega::int64_t sub_key = 0;
		OOBase::LocalString strSubKey,strLink,strFullKeyName;
		int err2 = strSubKey.assign("Connections");
		if (err2)
			LOG_ERROR(("Failed to assign string: %s",OOBase::system_error_text(err2)));
		else
		{
			Db::hive_errors err = ptrRegistry->create_key(key,sub_key,strSubKey,0,0,strLink,strFullKeyName);
			if (err)
			{
				if (err != Db::HIVE_NOTFOUND)
					LOG_ERROR(("Failed to open the '/System/Services/%s/Connections' key in the registry",strName.c_str()));
			}
			else
			{
				// Enum each connection...
				Db::Hive::registry_set_t values;
				err = ptrRegistry->enum_values(sub_key,0,values);
				if (err)
					LOG_ERROR(("Failed to enumerate the '/System/Services/%s/Connections' values in the registry",strName.c_str()));
				else
				{
					OOBase::String strSocketName;
					for (size_t idx = 0;values.pop(&strSocketName);)
					{
						// Make sure we have some kind of name!
						if (strSocketName.empty())
							strSocketName.printf("%lu",++idx);

						if (strSocketName[0] == '.')
							continue;

						OOBase::LocalString strValue;
						err = ptrRegistry->get_value(sub_key,strSocketName.c_str(),0,strValue);
						if (err)
							LOG_ERROR(("Failed to get '%s' from '/System/Services/%s/Connections' in the registry",strSocketName.c_str(),strName.c_str()));
						else
							create_and_forward_socket(pid,strName,strSocketName,strValue,ptrSocket);
					}
				}
			}
		}

		// Write terminating NULL
		ptrSocket->send(Omega::uint32_t(0));
		if (err2)
			LOG_ERROR(("Failed to send connection data to service: %s",OOBase::system_error_text(err2)));
	}

}

bool Root::Manager::start_services()
{
	// Get the list of services, ordered by dependency
	OOBase::Queue<OOBase::String,OOBase::LocalAllocator> queueNames;
	OOBase::Queue<Omega::int64_t,OOBase::LocalAllocator> queueKeys;
	if (!enum_services(m_registry,queueNames,queueKeys))
		return false;

	// Find the sandbox process
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	UserProcess sandbox;
	if (!m_sandbox_channel || !m_mapUserProcesses.find(m_sandbox_channel,sandbox))
		LOG_ERROR_RETURN(("Failed to find sandbox process"),false);

	guard.release();

	OOBase::String strName;
	Omega::int64_t key;
	while (queueNames.pop(&strName) && queueKeys.pop(&key))
	{
		OOBase::Logger::log(OOBase::Logger::Information,"Starting service: %s",strName.c_str());

		// Get the timeout value
		unsigned long wait_secs = 0;
		OOBase::LocalString strTimeout;
		if (m_registry->get_value(key,"Timeout",0,strTimeout) == Db::HIVE_OK)
			wait_secs = strtoul(strTimeout.c_str(),NULL,10);
		
		if (wait_secs == 0)
			wait_secs = 15;

		if (Root::is_debug())
			wait_secs = 1000;

		// Create a unique local socket name
		OOBase::RefPtr<OOBase::Socket> ptrSocket = sandbox.m_ptrProcess->LaunchService(this,strName,key,wait_secs);
		if (ptrSocket)
			enum_sockets(m_registry,strName,ptrSocket,key);
	}

	OOBase::Logger::log(OOBase::Logger::Information,"All services started");

	return true;
}

bool Root::Manager::stop_services()
{
	OOBase::Logger::log(OOBase::Logger::Information,"Stopping services");

	// Send the stop message to the sandbox oosvruser process
	OOBase::CDRStream request;
	if (!request.write(static_cast<OOServer::RootOpCode_t>(OOServer::StopServices)))
		LOG_ERROR_RETURN(("Failed to write request data: %s",OOBase::system_error_text(request.last_error())),false);
	
	// Make a blocking call
	OOBase::CDRStream response;
	OOServer::MessageHandler::io_result::type res = sendrecv_sandbox(request,&response,OOBase::Timeout(),OOServer::Message_t::synchronous);
	if (res != OOServer::MessageHandler::io_result::success)
		LOG_ERROR_RETURN(("Failed to send service stop request to sandbox"),false);

	Omega::int32_t err = 0;
	if (!response.read(err))
		LOG_ERROR_RETURN(("Failed to read response data: %s",OOBase::system_error_text(response.last_error())),false);

	if (err)
		LOG_ERROR_RETURN(("Service stop failed: %s",OOBase::system_error_text(err)),false);

	return true;
}

