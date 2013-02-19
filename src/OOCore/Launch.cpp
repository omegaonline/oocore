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
#if defined(_WIN32)
		// We don't use session_id with Win32,
		strId.clear();
#else

#if defined(HAVE_UNISTD_H)
		// Just default to using the sid with POSIX
		strId.printf("SID:%d",getsid(0));
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
							strId.concat("DBUS:",session_path);

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

	void connect_root(OOBase::CDRStream& response, OOBase::AllocatorInstance& allocator)
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

		OOBase::LocalString strSid(allocator);
		get_session_id(strSid);

		size_t mark = response.buffer()->mark_wr_ptr();

		response.write(Omega::uint16_t(0));
		response.write(version);
		response.write_string(strSid);

#if defined(_WIN32)
		response.write(GetCurrentProcessId());
#elif defined(HAVE_UNISTD_H)
		response.write(getpid());
#endif
		response.replace(static_cast<Omega::uint16_t>(response.buffer()->length()),mark);
		if (response.last_error())
			OMEGA_THROW(response.last_error());

#if !defined(HAVE_UNISTD_H)
		err = OOBase::CDRIO::send_and_recv_with_header_blocking<Omega::uint16_t>(response,root_socket);
#else

#if defined(SO_PASSCRED)
		int val = 1;
		if (::setsockopt(root_socket->get_handle(), SOL_SOCKET, SO_PASSCRED, &val, sizeof(val)) != 0)
			OMEGA_THROW(errno);
#endif
		OOBase::RefPtr<OOBase::Buffer> ctl_buffer = OOBase::Buffer::create(CMSG_SPACE(sizeof(int)),sizeof(size_t));
		if (!ctl_buffer)
			OMEGA_THROW(ERROR_OUTOFMEMORY);

		err = OOBase::CDRIO::send_and_recv_msg_with_header_blocking<Omega::uint16_t>(response,ctl_buffer,root_socket);
		if (err)
			OMEGA_THROW(err);

		OOBase::POSIX::SmartFD passed_fd;

		// Read struct cmsg
		struct msghdr msgh = {0};
		msgh.msg_control = const_cast<char*>(ctl_buffer->rd_ptr());
		msgh.msg_controllen = ctl_buffer->length();

		for (struct cmsghdr* msg = CMSG_FIRSTHDR(&msgh);msg;msg = CMSG_NXTHDR(&msgh,msg))
		{
			if (msg->cmsg_level == SOL_SOCKET && msg->cmsg_type == SCM_RIGHTS)
			{
				int* fds = reinterpret_cast<int*>(CMSG_DATA(msg));
				size_t fd_count = (msg->cmsg_len - CMSG_LEN(0))/sizeof(int);
				size_t fd_start = 0;
				if (fd_count > 0)
				{
					err = OOBase::POSIX::set_close_on_exec(fds[0],true);
					if (!err)
					{
						passed_fd = fds[0];
						fd_start = 1;
					}
				}

				for (size_t i=fd_start;i<fd_count;++i)
					OOBase::POSIX::close(fds[i]);
			}
		}

		// Append fd to response
		if (!response.write(static_cast<int>(passed_fd)))
			OMEGA_THROW(response.last_error());

		// Don't close the fd
		passed_fd.detach();
#endif
	}
}

void OOCore::UserSession::start(void* data, size_t length)
{
	OOBase::StackAllocator<128> allocator;

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
		connect_root(stream,allocator);

	// Now read back all the bits we need... Don't throw yet, there may be an embedded fd
	OOCore::pid_t stream_id = 0;
	stream.read(stream_id);

	OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket;

#if defined(_WIN32)
	OOBase::LocalString strPipe(allocator);
	stream.read_string(strPipe);

#elif defined(HAVE_UNISTD_H)
	// And read the embedded fd
	int fd_ = -1;
	stream.read(fd_);
	OOBase::POSIX::SmartFD fd(fd_);
#endif

	if (stream.last_error())
		OMEGA_THROW(stream.last_error());

	// Start the proactor and threads here!
	void* TODO;

	// Spawn off the io worker thread
	m_worker_thread.run(io_worker_fn,this);

	// Now do the connect/attach


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
