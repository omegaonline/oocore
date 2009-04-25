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

#include "Win32.h"

namespace OOBase
{
	// This is the critical failure hook
	void CriticalFailure(const char* msg);
}

std::string OOBase::strerror(int err)
{
	char szBuf[1024] = {0};
	sprintf_s(szBuf,sizeof(szBuf),"(%d) ",err);
	std::string str = szBuf;

	if (strerror_s(szBuf,sizeof(szBuf),err) == 0)
		str += szBuf;
	else
		str += "No message";
	
	return str;
}

void OOBase::CallCriticalFailureE(const char* pszFile, unsigned int nLine, int err)
{
	CallCriticalFailureX(pszFile,nLine,strerror(err).c_str());
}

#if defined(_WIN32)

void OOBase::CallCriticalFailureX(const char* pszFile, unsigned int nLine, int err)
{
	CallCriticalFailureX(pszFile,nLine,OOBase::Win32::FormatMessage(err).c_str());
}

void OOBase::CallCriticalFailureMem(const char* pszFile, unsigned int nLine)
{
	CallCriticalFailureX(pszFile,nLine,ERROR_OUTOFMEMORY);
}

#else

void OOBase::CallCriticalFailureX(const char* pszFile, unsigned int nLine, int err)
{
	CallCriticalFailureE(pszFile,nLine,err);
}

void OOBase::CallCriticalFailureMem(const char* pszFile, unsigned int nLine)
{
	CallCriticalFailureE(pszFile,nLine,ENOMEM);
}

#endif

void OOBase::CallCriticalFailureX(const char* pszFile, unsigned int nLine, const char* msg)
{
	char szBuf[64] = {0};
	sprintf_s(szBuf,sizeof(szBuf),"(%lu): ",nLine);

	std::string str = pszFile;
	str += szBuf;
	str += msg;

	CriticalFailure(str.c_str());
}
