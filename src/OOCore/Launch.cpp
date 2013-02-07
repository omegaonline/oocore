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

#if defined(HAVE_DBUS_H)
#undef interface
#include <dbus/dbus.h>
#define interface struct
#endif

#if !defined(ECONNREFUSED)
#define ECONNREFUSED ENOENT
#endif

#if defined(_WIN32)
	#define ROOT_NAME "OmegaOnline"
#elif defined(__linux__)
	#define ROOT_NAME "\0/org/omegaonline"
#elif defined(P_tmpdir)
	#define ROOT_NAME P_tmpdir "/omegaonline"
#else
	#define ROOT_NAME "/tmp/omegaonline"
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

	void connect_root(OOBase::CDRStream& response)
	{
#if defined(NDEBUG)
		OOBase::Timeout timeout(15,0);
#else
		OOBase::Timeout timeout;
#endif

		int err = 0;
		OOBase::RefPtr<OOBase::Socket> root_socket;
		while (!timeout.has_expired())
		{
			root_socket = OOBase::Socket::connect(ROOT_NAME,err,timeout);
			if (!err || (err != ENOENT && err != ECONNREFUSED))
				break;

			// We ignore the error, and try again until we timeout
		}

		if (err)
		{
			ObjectPtr<IException> ptrE = ISystemException::Create(err);
			throw IInternalException::Create(OOCore::get_text("Failed to connect to network daemon"),"Omega::Initialize",0,NULL,ptrE);
		}

		uint32_t version = (OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16) | OOCORE_PATCH_VERSION;

		OOBase::StackAllocator<128> allocator;
		OOBase::LocalString strSid(allocator);
		get_session_id(strSid);

		if (!response.write(version) || !response.write_string(strSid))
			OMEGA_THROW(response.last_error());

		err = OOBase::CDRIO::send_and_recv_with_header_blocking<Omega::uint16_t>(response,root_socket);
		if (err)
			OMEGA_THROW(err);
	}
}

void OOCore::UserSession::start(void* data, size_t length)
{
	int err = 0;
	OOBase::CDRStream stream;
	if (data)
	{
		err = stream.buffer()->space(length);
		if (err)
			OMEGA_THROW(err);

		memcpy(stream.buffer()->wr_ptr(),data,length);
		stream.buffer()->wr_ptr(length);
	}
	else
		connect_root(stream);


	// Send version information
	uint32_t version = (OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16) | OOCORE_PATCH_VERSION;
	if ((err = m_stream->send(version)) != 0)
		OMEGA_THROW(err);

	// Read our channel id
	if ((err = m_stream->recv(m_channel_id)) != 0)
		OMEGA_THROW(err);

	// Create the zero compartment
	OOBase::SmartPtr<Compartment> ptrZeroCompt = new (OOCore::throwing) Compartment(this);
	ptrZeroCompt->set_id(0);

	// Register our local channel factory
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);
	m_rot_cookies.push(OTL::GetModule()->RegisterAutoObjectFactory<OOCore::ChannelMarshalFactory>());

	// Create a new object manager for the user channel on the zero compartment
	if ((err = m_mapCompartments.force_insert(0,ptrZeroCompt)) != 0)
		OMEGA_THROW(err);

	guard.release();

	// Spawn off the io worker thread
	m_worker_thread.run(io_worker_fn,this);

	// Create a proxy to the server interface
	IObject* pIPS = NULL;
	ObjectPtr<Remoting::IObjectManager> ptrOM = ptrZeroCompt->get_channel_om(m_channel_id & 0xFF000000);
	ptrOM->GetRemoteInstance(OID_InterProcessService,Activation::Library | Activation::DontLaunch,OMEGA_GUIDOF(IInterProcessService),pIPS);
	ObjectPtr<IInterProcessService> ptrIPS = static_cast<IInterProcessService*>(pIPS);

	// Register the IPS...
	OTL::GetModule()->RegisterIPS(ptrIPS,false);

	// And register the Registry
	ObjectPtr<Registry::IKey> ptrReg;
	ptrReg.GetObject(Registry::OID_Registry_Instance);
	if (ptrReg)
	{
		// Re-register the proxy locally.. it saves a lot of time!
		ObjectPtr<Activation::IRunningObjectTable> ptrROT;
		ptrROT.GetObject(Activation::OID_RunningObjectTable_Instance);

		guard.acquire();

		m_rot_cookies.push(ptrROT->RegisterObject(Registry::OID_Registry_Instance,ptrReg,Activation::ProcessScope));
		m_rot_cookies.push(ptrROT->RegisterObject(string_t::constant("Omega.Registry"),ptrReg,Activation::ProcessScope));
	}
}
