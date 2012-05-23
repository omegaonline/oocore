///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOCore_precomp.h"

#include "UserSession.h"
#include "Activation.h"

using namespace Omega;
using namespace OTL;

#if !defined(ECONNREFUSED) && defined(_WIN32)
#define ECONNREFUSED WSAECONNREFUSED
#endif

#if defined(HAVE_DBUS_H)
#undef interface
#include <dbus/dbus.h>
#define interface struct
#endif

namespace
{
	void get_session_id(OOBase::LocalString& strId)
	{
		// We don't use session_id with Win32
#if defined(_WIN32)
		(void)strId;
#else

#if defined(HAVE_UNISTD_H)
		// Just default to using the sid with POSIX
		strId.printf("%d",getsid(0));
#endif

#if defined(HAVE_DBUS_H)
		{
			// Try for a DBUS session id
			DBusError error;
			dbus_error_init(&error);

			DBusConnection* conn = dbus_bus_get(DBUS_BUS_SYSTEM,NULL);
			if (dbus_error_is_set(&error))
				dbus_error_free(&error);
			else if (conn)
			{
				DBusMessage* call = dbus_message_new_method_call(
						"org.freedesktop.ConsoleKit",
						"/org/freedesktop/ConsoleKit/Manager",
						"org.freedesktop.ConsoleKit.Manager",
						"GetCurrentSession");

				if (call)
				{
					DBusMessage* reply = dbus_connection_send_with_reply_and_block(conn,call,-1,&error);
					if (dbus_error_is_set(&error))
						dbus_error_free(&error);
					else if (reply)
					{
						const char* session_path = NULL;
						if (!dbus_message_get_args(reply,&error,DBUS_TYPE_OBJECT_PATH,&session_path,DBUS_TYPE_INVALID))
							dbus_error_free(&error);
						else
							strId.assign(session_path);

						dbus_message_unref(reply);
					}

					dbus_message_unref(call);
				}

				dbus_connection_unref(conn);
			}
		}
#endif // HAVE_DBUS_H

#endif // !WIN32
	}

	void discover_server_port(OOBase::LocalString& strPipe)
	{
		int err = strPipe.getenv("OMEGA_SESSION_ADDRESS");
		if (err != 0)
			OMEGA_THROW(err);

		if (!strPipe.empty())
			return;

 	#if defined(_WIN32)
 		const char* name = "OmegaOnline";
	#else
 		const char* abstract_name = "\0/tmp/omegaonline";
		const char* name = abstract_name + 1;
	#endif

		OOBase::SmartPtr<OOBase::Socket> root_socket;

#if defined (__linux__)
		root_socket = OOBase::Socket::connect_local(abstract_name,err);
		if (err)
#endif
		root_socket = OOBase::Socket::connect_local(name,err);
		if (err)
		{
			ObjectPtr<IException> ptrE = ISystemException::Create(err);
			throw IInternalException::Create("Failed to connect to network daemon","Omega::Initialize",size_t(-1),NULL,ptrE);
		}

		uint32_t version = (OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16) | OOCORE_PATCH_VERSION;

		OOBase::LocalString strSid;
		get_session_id(strSid);

		OOBase::CDRStream stream;
		if (!stream.write(version) || !stream.write(strSid.c_str()))
			OMEGA_THROW(stream.last_error());

		err = root_socket->send(stream.buffer());
		if (err)
			OMEGA_THROW(err);

		stream.reset();

		// Now read strPipe
		if (!stream.recv_string(root_socket,strPipe))
			OMEGA_THROW(stream.last_error());
	}
}

void OOCore::UserSession::start()
{
	OOBase::LocalString strPipe;
	discover_server_port(strPipe);

	int err = 0;

#if defined(__linux__)
	char abstract[108] = {0};
	memcpy(abstract+1,strPipe.c_str(),sizeof(abstract)-2);
#endif

	// Connect up to the user process...
	OOBase::Timeout timeout(15,0);
	do
	{

#if defined(__linux__)
		// Try for an abstract socket first...
		m_stream = OOBase::Socket::connect_local(abstract,err,timeout);
		if (!err)
			break;
#endif

		m_stream = OOBase::Socket::connect_local(strPipe.c_str(),err,timeout);
		if (!err || (err != ENOENT && err != ECONNREFUSED))
			break;

		// We ignore the error, and try again until we timeout
	}
	while (!timeout.has_expired());

	if (err)
		OMEGA_THROW(err);

	// Send version information
	uint32_t version = (OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16) | OOCORE_PATCH_VERSION;
	if ((err = m_stream->send(version)) != 0)
		OMEGA_THROW(err);

	// Read our channel id
	if ((err = m_stream->recv(m_channel_id)) != 0)
		OMEGA_THROW(err);

	// Spawn off the io worker thread
	m_worker_thread.run(io_worker_fn,this);

	// Register built-ins
	RegisterObjects();

	// Create the zero compartment
	OOBase::SmartPtr<Compartment> ptrZeroCompt = new (OOCore::throwing) Compartment(this);
	
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		if ((err = m_mapCompartments.force_insert(0,ptrZeroCompt)) != 0)
			OMEGA_THROW(err);

		ptrZeroCompt->set_id(0);
	}

	// Create a new object manager for the user channel on the zero compartment
	ObjectPtr<Remoting::IObjectManager> ptrOM = ptrZeroCompt->get_channel_om(m_channel_id & 0xFF000000);

	// Create a proxy to the server interface
	IObject* pIPS = NULL;
	ptrOM->GetRemoteInstance(OID_InterProcessService,Activation::Library | Activation::DontLaunch,OMEGA_GUIDOF(IInterProcessService),pIPS);
	ObjectPtr<IInterProcessService> ptrIPS = static_cast<IInterProcessService*>(pIPS);

	// Register locally...
	m_nIPSCookie = OOCore_RegisterIPS(ptrIPS);
	
	// Now set our pipe name as an env var
	if (!strPipe.empty())
	{
#if defined(_WIN32)
		SetEnvironmentVariableA("OMEGA_SESSION_ADDRESS",strPipe.c_str());
#else
		setenv("OMEGA_SESSION_ADDRESS",strPipe.c_str(),1);
#endif
	}
}
