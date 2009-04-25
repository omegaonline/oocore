///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOBase, the Omega Online Base library.
//
// OOBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "Socket.h"

#if !defined(HAVE_WINDOWS_H)

namespace 
{
	class LocalSocket : public OOBase::Socket
	{
	public:
		LocalSocket(int sock);

		virtual ~LocalSocket()
		{
			close();
		}

		virtual int send(const void* buf, size_t len, const OOBase::timeval_t* timeout = 0);
		virtual size_t recv(void* buf, size_t len, int* perr, const OOBase::timeval_t* timeout = 0);
		virtual void close();
		virtual uid_t get_uid();

	private:
		int m_socket;
	};
}

OOBase::Socket* OOBase::Socket::connect_local(const std::string& path, int* perr)
{
}

int OOCore::UserSession::MessagePipe::connect(MessagePipe& pipe, const std::string& strAddr, OOBase::timeval_t* wait)
{
	ACE_UNIX_Addr addr(ACE_TEXT_CHAR_TO_TCHAR(strAddr.c_str()));

	if (ACE_SOCK_Connector().connect(pipe.m_stream,addr,wait) != 0)
		return -1;

	return 0;
}

void OOCore::UserSession::MessagePipe::close()
{
    m_stream.close_writer();
	m_stream.close_reader();
	m_stream.close_reader();
	m_stream.close();
}

ssize_t OOCore::UserSession::MessagePipe::send(const ACE_Message_Block* mb, OOBase::timeval_t* timeout, size_t* sent)
{
	return m_stream.send_n(mb,timeout,sent);
}

ssize_t OOCore::UserSession::MessagePipe::recv(void* buf, size_t len)
{
	return m_stream.recv(buf,len);
}

OOBase::Socket::uid_t LocalSocket::get_uid()
{
#if defined(HAVE_GETPEEREID)
	/* OpenBSD style:  */
	uid_t uid;
	gid_t gid;
	if (getpeereid(pSocket->get_read_handle(), &uid, &gid) != 0)
	{
		/* We didn't get a valid credentials struct. */
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("could not get peer credentials")),-1);
	}

#elif defined(SO_PEERCRED)
	/* Linux style: use getsockopt(SO_PEERCRED) */
	struct ucred peercred;
	//socklen_t so_len = sizeof(peercred);
	size_t so_len = sizeof(peercred);

	if (getsockopt(pSocket->get_read_handle(), SOL_SOCKET, SO_PEERCRED, &peercred, &so_len) != 0 || so_len != sizeof(peercred))
	{
		/* We didn't get a valid credentials struct. */
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("could not get peer credentials")),-1);
	}

	uid_t uid = peercred.uid;

#elif defined(HAVE_GETPEERUCRED)
	/* Solaris > 10 */
	ucred_t* ucred = NULL; /* must be initialized to NULL */
	if (getpeerucred(pSocket->get_read_handle(), &ucred) == -1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("could not get peer credentials")),-1);

	uid_t uid;
	if ((uid = ucred_geteuid(ucred)) == -1)
	{
		ucred_free(ucred);
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("could not get effective UID from peer credentials")),-1);
	}

#elif defined(HAVE_STRUCT_CMSGCRED) || defined(HAVE_STRUCT_FCRED) || (defined(HAVE_STRUCT_SOCKCRED) && defined(LOCAL_CREDS))

	/*
	* Receive credentials on next message receipt, BSD/OS,
	* NetBSD. We need to set this before the client sends the
	* next packet.
	*/
	int on = 1;
	if (setsockopt(pSocket->get_read_handle(), 0, LOCAL_CREDS, &on, sizeof(on)) < 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("could not enable credential reception")),-1);

	/* Credentials structure */
#if defined(HAVE_STRUCT_CMSGCRED)
	typedef struct cmsgcred Cred;
	#define cruid cmcred_uid
#elif defined(HAVE_STRUCT_FCRED)
	typedef struct fcred Cred;
	#define cruid fc_uid
#elif defined(HAVE_STRUCT_SOCKCRED)
	typedef struct sockcred Cred;
	#define cruid sc_uid
#endif
	/* Compute size without padding */
	char cmsgmem[ALIGN(sizeof(struct cmsghdr)) + ALIGN(sizeof(Cred))];   /* for NetBSD */

	/* Point to start of first structure */
	struct cmsghdr* cmsg = (struct cmsghdr*)cmsgmem;
	struct iovec iov;

	msghdr msg;
	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = (char *) cmsg;
	msg.msg_controllen = sizeof(cmsgmem);
	memset(cmsg, 0, sizeof(cmsgmem));

	/*
	 * The one character which is received here is not meaningful; its
	 * purposes is only to make sure that recvmsg() blocks long enough for the
	 * other side to send its credentials.
	 */
	char buf;
	iov.iov_base = &buf;
	iov.iov_len = 1;

	if (recvmsg(pSocket->get_read_handle(), &msg, 0) < 0 || cmsg->cmsg_len < sizeof(cmsgmem) || cmsg->cmsg_type != SCM_CREDS)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("could not get peer credentials")),-1);

	Cred* cred = (Cred*)CMSG_DATA(cmsg);
	uid_t uid = cred->cruid;
#else
	// We can't handle this situation
	#error Fix me!
#endif
}

#endif // defined(ACE_HAS_WIN32_NAMED_PIPES)
