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

#include "Condition.h"

#if defined(_WIN32)

OOBase::Condition::Condition()
{
	Win32::InitializeConditionVariable(&m_var);
}

OOBase::Condition::~Condition()
{
	Win32::DeleteConditionVariable(&m_var);
}

bool OOBase::Condition::wait(Condition::Mutex& mutex, const timeval_t* wait)
{
	return (Win32::SleepConditionVariable(&m_var,&mutex,wait ? wait->msec() : INFINITE) != FALSE);
}

void OOBase::Condition::signal()
{
	Win32::WakeConditionVariable(&m_var);
}

void OOBase::Condition::broadcast()
{
	Win32::WakeAllConditionVariable(&m_var);
}

#else

#error Fix me!

#endif
