///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2013 Rick Taylor
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

#include "OOServer_User.h"
#include "UserRootConn.h"
#include "UserManager.h"

User::RootConnection::RootConnection(Manager* pManager, OOBase::RefPtr<OOBase::AsyncSocket>& sock) :
		m_pManager(pManager),
		m_socket(sock)
{
}

User::RootConnection::~RootConnection()
{
	// Tell the manager that we have closed
	m_pManager->quit();
}

bool User::RootConnection::start()
{
#if defined(HAVE_UNISTD_H)
	OOBase::RefPtr<OOBase::Buffer> ctl_buffer = OOBase::Buffer::create(2*CMSG_SPACE(sizeof(int)),sizeof(size_t));
	if (!ctl_buffer)
		LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	addref();

	int err = OOBase::CDRIO::recv_msg_with_header_sync<size_t>(128,m_socket,this,&RootConnection::on_start_posix,ctl_buffer);
#elif defined(_WIN32)

	addref();

	int err = OOBase::CDRIO::recv_with_header_sync<size_t>(128,m_socket,this,&RootConnection::on_start_win32);
#endif
	if (err)
	{
		release();
		LOG_ERROR_RETURN(("Failed to receive from root: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

bool User::RootConnection::recv_next()
{
#if defined(HAVE_UNISTD_H)
	OOBase::RefPtr<OOBase::Buffer> ctl_buffer = OOBase::Buffer::create(CMSG_SPACE(sizeof(int)),sizeof(size_t));
	if (!ctl_buffer)
		LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	addref();

	int err = OOBase::CDRIO::recv_msg_with_header_sync<size_t>(128,m_socket,this,&RootConnection::on_message_posix,ctl_buffer);
#elif defined(_WIN32)

	addref();

	int err = OOBase::CDRIO::recv_with_header_sync<size_t>(128,m_socket,this,&RootConnection::on_message_win32);
#endif
	if (err)
	{
		release();
		LOG_ERROR_RETURN(("Failed to receive from root: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

#if defined(HAVE_UNISTD_H)

void User::RootConnection::on_start_posix(OOBase::CDRStream& stream, OOBase::Buffer* ctl_buffer, int err)
{
	if (err)
		LOG_ERROR(("Failed to recv from root pipe: %s",OOBase::system_error_text(err)));
	else
	{
		OOBase::POSIX::SmartFD passed_fds[2];

		// Read struct cmsg
		struct msghdr msgh = {0};
		msgh.msg_control = const_cast<char*>(ctl_buffer->rd_ptr());
		msgh.msg_controllen = ctl_buffer->length();

		struct cmsghdr* msg = CMSG_FIRSTHDR(&msgh);
		if (!msg)
		{
			LOG_ERROR(("Failed to receive handle from root pipe"));
			err = EINVAL;
		}
		else
		{
			if (msg->cmsg_level == SOL_SOCKET && msg->cmsg_type == SCM_RIGHTS)
			{
				if (msg->cmsg_len == CMSG_LEN(sizeof(int)))
					passed_fds[0] = *reinterpret_cast<int*>(CMSG_DATA(msg));
				else if (msg->cmsg_len == CMSG_LEN(2*sizeof(int)))
				{
					passed_fds[0] = reinterpret_cast<int*>(CMSG_DATA(msg))[0];
					passed_fds[1] = reinterpret_cast<int*>(CMSG_DATA(msg))[1];
				}
				else
				{
					LOG_ERROR(("Root pipe control data looks wrong"));
					err = EINVAL;
				}
			}
			else
			{
				LOG_ERROR(("Root pipe control data has weird stuff in it"));
				err = EINVAL;
			}

			if (CMSG_NXTHDR(&msgh,msg) != NULL)
			{
				LOG_ERROR(("Root pipe control data has extra stuff in it"));
				err = EINVAL;
			}
		}

		if (!err)
		{
			// Read and cache any root parameters
			void* p1;
			stream.read(p1);

			if (stream.last_error())
				LOG_ERROR(("Failed to read request from root: %s",OOBase::system_error_text(stream.last_error())));
			else
			{
				err = stream.reset();
				if (err)
					LOG_ERROR(("Failed to reset stream: %s",OOBase::system_error_text(stream.last_error())));
				else
				{
					size_t mark = stream.buffer()->mark_wr_ptr();

					stream.write(size_t(0));
					stream.write(p1);

					// Attach to the pipe
					int ret_err = 0;
					OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket = m_pManager->m_proactor->attach(passed_fds[0],ret_err);
					if (ret_err)
						LOG_ERROR(("Failed to attach to registry pipe: %s",OOBase::system_error_text(ret_err)));
					else
					{
						passed_fds[0].detach();

						// Tell the manager to create a new connection
						ret_err = m_pManager->connect_registry(ptrSocket);
						if (!ret_err && passed_fds[1].is_valid())
						{
							// Tell the manager we have an upstream OM
						}
					}

					stream.write(ret_err);

					stream.replace(stream.buffer()->length(),mark);
					if (stream.last_error())
						LOG_ERROR(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())));
					else
					{
						err = m_socket->send(this,NULL,stream.buffer());
						if (err)
							LOG_ERROR(("Failed to write response to root: %s",OOBase::system_error_text(stream.last_error())));
						else
							recv_next();
					}
				}
			}
		}
	}

	release();
}

void User::RootConnection::on_message_posix(OOBase::CDRStream& stream, OOBase::Buffer* ctl_buffer, int err)
{
	if (err)
		LOG_ERROR(("Failed to receive from root pipe: %s",OOBase::system_error_text(err)));
	else
	{
		OOBase::POSIX::SmartFD passed_fd;

		// Read struct cmsg
		struct msghdr msgh = {0};
		msgh.msg_control = const_cast<char*>(ctl_buffer->rd_ptr());
		msgh.msg_controllen = ctl_buffer->length();

		struct cmsghdr* msg = CMSG_FIRSTHDR(&msgh);
		if (msg)
		{
			if (msg->cmsg_level == SOL_SOCKET && msg->cmsg_type == SCM_RIGHTS)
			{
				if (msg->cmsg_len != CMSG_LEN(sizeof(int)))
				{
					LOG_ERROR(("Root pipe control data looks wrong"));
					err = EINVAL;
				}
				else
					passed_fd = *reinterpret_cast<int*>(CMSG_DATA(msg));
			}
			else
			{
				LOG_ERROR(("Root pipe control data has weird stuff in it"));
				err = EINVAL;
			}

			if (CMSG_NXTHDR(&msgh,msg) != NULL)
			{
				LOG_ERROR(("Root pipe control data has extra stuff in it"));
				err = EINVAL;
			}
		}

		if (!err)
			on_message(stream,passed_fd);
	}

	release();
}

void User::RootConnection::new_connection(OOBase::CDRStream& stream, OOBase::POSIX::SmartFD& passed_fd)
{
	// Read and cache any root parameters
	void* p1;
	stream.read(p1);

	if (stream.last_error())
		LOG_ERROR(("Failed to read request from root: %s",OOBase::system_error_text(stream.last_error())));
	else
	{
		int err = stream.reset();
		if (err)
			LOG_ERROR(("Failed to reset stream: %s",OOBase::system_error_text(stream.last_error())));
		else
		{
			size_t mark = stream.buffer()->mark_wr_ptr();

			stream.write(size_t(0));
			stream.write(p1);

			// Attach to the pipe
			int ret_err = 0;

			void* TODO; // Omega_ConnectChannel(passed_fd) ?!?!
			if (!ret_err)
				passed_fd.detach();

			stream.write(ret_err);

			stream.replace(stream.buffer()->length(),mark);
			if (stream.last_error())
				LOG_ERROR(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())));
			else
			{
				err = m_socket->send(this,NULL,stream.buffer());
				if (err)
					LOG_ERROR(("Failed to write response to root: %s",OOBase::system_error_text(stream.last_error())));
			}
		}
	}
}

#elif defined(_WIN32)

void User::RootConnection::on_start_win32(OOBase::CDRStream& stream, int err)
{
	if (err)
		LOG_ERROR(("Failed to receive from root pipe: %s",OOBase::system_error_text(err)));
	else
	{
		// Read and cache any root parameters
		void* p1;
		stream.read(p1);

		OOBase::StackAllocator<256> allocator;
		OOBase::LocalString strPipe(allocator);
		stream.read_string(strPipe);

		if (stream.last_error())
			LOG_ERROR(("Failed to read request from root: %s",OOBase::system_error_text(stream.last_error())));
		else
		{
			err = stream.reset();
			if (err)
				LOG_ERROR(("Failed to reset stream: %s",OOBase::system_error_text(stream.last_error())));
			else
			{
				size_t mark = stream.buffer()->mark_wr_ptr();

				stream.write(size_t(0));

				int ret_err = 0;

				OOBase::Timeout timeout(20,0);
				OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket = m_pManager->m_proactor->connect(strPipe.c_str(),ret_err,timeout);
				if (ret_err != 0)
					LOG_ERROR(("Failed to connect to registry pipe: %s",OOBase::system_error_text(ret_err)));
				else
					ret_err = m_pManager->connect_registry(ptrSocket);

				stream.write(ret_err);

				stream.replace(stream.buffer()->length(),mark);
				if (stream.last_error())
					LOG_ERROR(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())));
				else
				{
					err = m_socket->send(this,NULL,stream.buffer());
					if (err)
						LOG_ERROR(("Failed to write response to root: %s",OOBase::system_error_text(stream.last_error())));
					else
						recv_next();
				}
			}
		}
	}

	release();
}

void User::RootConnection::on_message_win32(OOBase::CDRStream& stream, int err)
{
	if (err)
		LOG_ERROR(("Failed to receive from root pipe: %s",OOBase::system_error_text(err)));
	else
		on_message(stream);

	release();
}

void User::RootConnection::new_connection(OOBase::CDRStream& stream)
{
	// Read and cache any root parameters
	void* p1;
	stream.read(p1);

	if (stream.last_error())
		LOG_ERROR(("Failed to read request from root: %s",OOBase::system_error_text(stream.last_error())));
	else
	{
		KICK OFF A CONNECT ETC...

		int err = stream.reset();
		if (err)
			LOG_ERROR(("Failed to reset stream: %s",OOBase::system_error_text(stream.last_error())));
		else
		{
			size_t mark = stream.buffer()->mark_wr_ptr();

			stream.write(size_t(0));
			stream.write(p1);

			OOBase::StackAllocator<256> allocator;
			OOBase::LocalString pipe(allocator);

			int ret_err = connect_user(pipe);
			stream.write(ret_err);
			if (!ret_err)
				stream.write_string(pipe);

			stream.replace(stream.buffer()->length(),mark);
			if (stream.last_error())
				LOG_ERROR(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())));
			else
			{
				err = m_socket->send(this,NULL,stream.buffer());
				if (err)
					LOG_ERROR(("Failed to write response to root: %s",OOBase::system_error_text(stream.last_error())));
			}
		}
	}
}

#endif

#if defined(HAVE_UNISTD_H)
void User::RootConnection::on_message(OOBase::CDRStream& stream, OOBase::POSIX::SmartFD& passed_fd)
#elif defined(_WIN32)
void User::RootConnection::on_message(OOBase::CDRStream& stream)
#endif
{
	OOServer::Root2User_OpCode_t op_code;
	if (!stream.read(op_code))
		LOG_ERROR(("Failed to read request opcode from root: %s",OOBase::system_error_text(stream.last_error())));
	else
	{
		switch (static_cast<OOServer::Root2User_OpCode>(op_code))
		{
		case OOServer::Root2User_NewConnection:
			if (recv_next())
			{
#if defined(HAVE_UNISTD_H)
				new_connection(stream,passed_fd);
#else
				new_connection(stream);
#endif
			}
			break;

		default:
			break;
		}
	}
}
