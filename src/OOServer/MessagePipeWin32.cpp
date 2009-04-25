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
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"
#include "MessagePipe.h"

#if defined(ACE_HAS_WIN32_NAMED_PIPES)

#include <aclapi.h>

Root::MessagePipe::MessagePipe() :
	m_hRead(ACE_INVALID_HANDLE), m_hWrite(ACE_INVALID_HANDLE)
{
}

int Root::MessagePipe::connect(OOBase::SmartPtr<MessagePipe>& pipe, const ACE_CString& strAddr, ACE_Time_Value* wait)
{
	ACE_TString strPipe = ACE_TEXT_CHAR_TO_TCHAR(strAddr.c_str());

	ACE_NEW_RETURN(pipe,MessagePipe,-1);
	
	ACE_Time_Value val(30);
	if (!wait)
		wait = &val;

	ACE_Countdown_Time countdown(wait);

	ACE_SPIPE_Connector connector;
	ACE_SPIPE_Addr addr;

	ACE_SPIPE_Stream down;
	addr.string_to_addr((strPipe + ACE_TEXT("\\down")).c_str());
	if (connector.connect(down,addr,wait,ACE_Addr::sap_any,0,O_RDWR | FILE_FLAG_OVERLAPPED) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("connector.connect() failed")),-1);

	countdown.update();

	ACE_SPIPE_Stream up;
	addr.string_to_addr((strPipe + ACE_TEXT("\\up")).c_str());
	if (connector.connect(up,addr,wait,ACE_Addr::sap_any,0,O_WRONLY) != 0)
	{
		down.close();
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("connector.connect() failed")),-1);
	}

	pipe->m_hRead = down.get_handle();
	pipe->m_hWrite = up.get_handle();

	up.set_handle(ACE_INVALID_HANDLE);
	down.set_handle(ACE_INVALID_HANDLE);

	return 0;
}

void Root::MessagePipe::close()
{
	ACE_HANDLE hRead = m_hRead;
	m_hRead = ACE_INVALID_HANDLE;
	ACE_HANDLE hWrite = m_hWrite;
	m_hWrite = ACE_INVALID_HANDLE;

	if (hRead != ACE_INVALID_HANDLE)
		ACE_OS::close(hRead);

	if (hWrite != ACE_INVALID_HANDLE)
	{
		ACE_OS::fsync(hWrite);
		ACE_OS::close(hWrite);
	}
}

ACE_HANDLE Root::MessagePipe::get_read_handle() const
{
	return m_hRead;
}

ssize_t Root::MessagePipe::send(const void* buf, size_t len, size_t* sent)
{
	return ACE_OS::write_n(m_hWrite,buf,len,sent);
}

ssize_t Root::MessagePipe::send(const ACE_Message_Block* mb, size_t* sent)
{
	return ACE::write_n(m_hWrite,mb,sent);
}

ssize_t Root::MessagePipe::recv(void* buf, size_t len)
{
	ssize_t nRead = ACE_OS::read_n(m_hRead,buf,len);
	if (nRead == -1 && GetLastError() == ERROR_MORE_DATA)
		nRead = static_cast<ssize_t>(len);

	return nRead;
}

Root::MessagePipeAcceptor::MessagePipeAcceptor()
{
	m_sa.lpSecurityDescriptor = NULL;
	m_sa.nLength = 0;
	m_pACL = NULL;
}

Root::MessagePipeAcceptor::~MessagePipeAcceptor()
{
	LocalFree(m_pACL);
	LocalFree(m_sa.lpSecurityDescriptor);
}

bool Root::MessagePipeAcceptor::CreateSA(HANDLE hToken, void*& pSD, PACL& pACL)
{
	const int NUM_ACES  = 2;
	EXPLICIT_ACCESSW ea[NUM_ACES];
	ZeroMemory(&ea, NUM_ACES * sizeof(EXPLICIT_ACCESS));

	// Get the current processes user SID
	HANDLE hProcessToken;
	if (!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hProcessToken))
		return false;

	DWORD dwLen = 0;
	GetTokenInformation(hProcessToken,TokenUser,NULL,0,&dwLen);
	if (dwLen == 0)
	{
		CloseHandle(hProcessToken);
		return false;
	}

	TOKEN_USER* pSIDProcess = static_cast<TOKEN_USER*>(ACE_OS::malloc(dwLen));
	if (!pSIDProcess)
	{
		CloseHandle(hProcessToken);
		return false;
	}

	if (!GetTokenInformation(hProcessToken,TokenUser,pSIDProcess,dwLen,&dwLen))
	{
		CloseHandle(hProcessToken);
		ACE_OS::free(pSIDProcess);
		return false;
	}
	CloseHandle(hProcessToken);

	// Set full control for the calling process SID
	ea[0].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
	ea[0].grfAccessMode = SET_ACCESS;
	ea[0].grfInheritance = NO_INHERITANCE;
	ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea[0].Trustee.ptstrName = (LPWSTR)pSIDProcess->User.Sid;
	
	PSID pSIDUsers = NULL;
	TOKEN_USER* pSIDToken = NULL;
	if (!hToken)
	{
		// Create a SID for the BUILTIN\Users group.
		SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
		if (!AllocateAndInitializeSid(&SIDAuthNT, 2,
			SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_USERS,
			0, 0, 0, 0, 0, 0,
			&pSIDUsers))
		{
			ACE_OS::free(pSIDProcess);
			return false;
		}

		// Set read access
		ea[1].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
		ea[1].grfAccessMode = SET_ACCESS;
		ea[1].grfInheritance = NO_INHERITANCE;
		ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea[1].Trustee.ptstrName = (LPWSTR) pSIDUsers;
	}
	else
	{
		// Get the SID for the token's user
		dwLen = 0;
		GetTokenInformation(hToken,TokenUser,NULL,0,&dwLen);
		if (dwLen == 0)
		{
			ACE_OS::free(pSIDProcess);
			return false;
		}

		pSIDToken = static_cast<TOKEN_USER*>(ACE_OS::malloc(dwLen));
		if (!pSIDToken)
		{
			ACE_OS::free(pSIDProcess);
			return false;
		}

		if (!GetTokenInformation(hToken,TokenUser,pSIDToken,dwLen,&dwLen))
		{
			ACE_OS::free(pSIDProcess);
			ACE_OS::free(pSIDToken);
			return false;
		}

		// Set read access for Specific user.
		ea[1].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
		ea[1].grfAccessMode = SET_ACCESS;
		ea[1].grfInheritance = NO_INHERITANCE;
		ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[1].Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea[1].Trustee.ptstrName = (LPWSTR)pSIDToken->User.Sid;
	}

	if (ERROR_SUCCESS != SetEntriesInAclW(NUM_ACES,ea,NULL,&pACL))
	{
		ACE_OS::free(pSIDProcess);
		if (pSIDUsers)
			FreeSid(pSIDUsers);
		if (pSIDToken)
			ACE_OS::free(pSIDToken);
		return false;
	}

	ACE_OS::free(pSIDProcess);
	if (pSIDUsers)
		FreeSid(pSIDUsers);
	if (pSIDToken)
		ACE_OS::free(pSIDToken);

	// Create a new security descriptor
	pSD = LocalAlloc(LPTR,SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (pSD == NULL)
	{
		LocalFree(pACL);
		pACL = NULL;
		return false;
	}

	// Initialize a security descriptor.
	if (!InitializeSecurityDescriptor(static_cast<PSECURITY_DESCRIPTOR>(pSD),SECURITY_DESCRIPTOR_REVISION))
	{
		LocalFree(pSD);
		LocalFree(pACL);
		pACL = NULL;
		return false;
	}

	// Add the ACL to the SD
	if (!SetSecurityDescriptorDacl(static_cast<PSECURITY_DESCRIPTOR>(pSD),TRUE,pACL,FALSE))
	{
		LocalFree(pSD);
		LocalFree(pACL);
		pACL = NULL;
		return false;
	}

	return true;
}

int Root::MessagePipeAcceptor::open(const ACE_CString& strAddr, HANDLE hToken)
{
	ACE_TString strPipe = ACE_TEXT_CHAR_TO_TCHAR(strAddr.c_str());

	if (m_sa.nLength == 0)
	{
		m_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		m_sa.bInheritHandle = FALSE;

		if (!CreateSA(hToken,m_sa.lpSecurityDescriptor,m_pACL))
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: Failed to create security descriptor: %#x\n"),GetLastError()),-1);
	}

	ACE_SPIPE_Addr addr;
	addr.string_to_addr((strPipe + ACE_TEXT("\\up")).c_str());
	if (m_acceptor_up.open(addr,1,ACE_DEFAULT_FILE_PERMS,&m_sa,PIPE_READMODE_BYTE) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("acceptor.open() failed")),-1);

	addr.string_to_addr((strPipe + ACE_TEXT("\\down")).c_str());
	if (m_acceptor_down.open(addr,1,ACE_DEFAULT_FILE_PERMS,&m_sa,PIPE_READMODE_BYTE) != 0)
	{
		m_acceptor_up.close();
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("acceptor.open() failed")),-1);
	}

	return 0;
}

int Root::MessagePipeAcceptor::accept(OOBase::SmartPtr<MessagePipe>& pipe, ACE_Time_Value* timeout)
{
	ACE_NEW_RETURN(pipe,MessagePipe,-1);
	
	ACE_Time_Value val(30);
	if (!timeout)
		timeout = &val;

	ACE_Countdown_Time countdown(timeout);
	ACE_SPIPE_Stream down;
	if (m_acceptor_down.accept(down,0,timeout) != 0 && GetLastError() != ERROR_MORE_DATA)	
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("acceptor.accept() failed")),-1);

	countdown.update();

	ACE_SPIPE_Stream up;
	if (m_acceptor_up.accept(up,0,timeout) != 0 && GetLastError() != ERROR_MORE_DATA)
	{
		down.close();
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%N:%l: %p\n"),ACE_TEXT("acceptor.accept() failed")),-1);
	}

	pipe->m_hRead = up.get_handle();
	pipe->m_hWrite = down.get_handle();

	up.set_handle(ACE_INVALID_HANDLE);
	down.set_handle(ACE_INVALID_HANDLE);

	return 0;
}

ACE_HANDLE Root::MessagePipeAcceptor::get_handle()
{
	return m_acceptor_up.get_handle();
}

void Root::MessagePipeAcceptor::close()
{
	m_acceptor_up.close();
	m_acceptor_down.close();
}

#endif // defined(ACE_HAS_WIN32_NAMED_PIPES)
