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

#include "TimeVal.h"

#if defined(HAVE_SYS_TIME_H) && (HAVE_SYS_TIME_H == 1)
#include <sys/time.h>
#endif

#include <limits.h>

const OOBase::timeval_t OOBase::timeval_t::MaxTime(LLONG_MAX, 999999);
const OOBase::timeval_t OOBase::timeval_t::Zero(0, 0);

OOBase::timeval_t OOBase::gettimeofday()
{

#if defined(_WIN32)

	static const ULONGLONG epoch = 116444736000000000LL;

	FILETIME file_time;
	GetSystemTimeAsFileTime(&file_time);

	ULARGE_INTEGER ularge;
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;

	// Move to epoch
	ULONGLONG q = (ularge.QuadPart - epoch);

	return OOBase::timeval_t(q / 10000000LL,static_cast<int>((q / 10L) % 1000000L));

#elif defined(HAVE_SYS_TIME_H) && (HAVE_SYS_TIME_H == 1)

	timeval tv;
	::gettimeofday(&tv,0);

	return OOBase::timeval_t(tv.tv_sec,tv.tv_usec);

#else
#error gettimeofday() !!
#endif
}

OOBase::timeval_t& OOBase::timeval_t::operator += (const timeval_t& rhs)
{
	if (m_tv_usec + rhs.m_tv_usec >= 1000000)
	{
		int nsec = (m_tv_usec + rhs.m_tv_usec) / 1000000;
		m_tv_usec -= 1000000 * nsec;
		m_tv_sec += nsec;
	}

	m_tv_sec += rhs.m_tv_sec;
	m_tv_usec += rhs.m_tv_usec;

	return *this;
}

OOBase::timeval_t& OOBase::timeval_t::operator -= (const timeval_t& rhs)
{
	/* Perform the carry for the later subtraction by updating r. */
	timeval_t r = rhs;
	if (m_tv_usec < r.m_tv_usec)
	{
		int nsec = (r.m_tv_usec - m_tv_usec) / 1000000 + 1;
		r.m_tv_usec -= 1000000 * nsec;
		r.m_tv_sec += nsec;
	}

	if (m_tv_usec - r.m_tv_usec >= 1000000)
	{
		int nsec = (m_tv_usec - r.m_tv_usec) / 1000000;
		r.m_tv_usec += 1000000 * nsec;
		r.m_tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	 m_tv_usec is certainly positive. */
	m_tv_sec -= r.m_tv_sec;
	m_tv_usec -= r.m_tv_usec;

	return *this;
}

OOBase::timeval_t OOBase::timeval_t::deadline(unsigned long msec)
{
	return gettimeofday() + timeval_t(msec / 1000,(msec % 1000) * 1000);
}

OOBase::Countdown::Countdown(timeval_t* wait) :
		m_start(gettimeofday()),
		m_wait(wait)
{
	assert(wait);
}

void OOBase::Countdown::update()
{
	timeval_t now = gettimeofday();
	timeval_t diff = (now - m_start);
	if (diff >= *m_wait)
		*m_wait = timeval_t::Zero;
	else
	{
		m_start = now;
		*m_wait -= diff;
	}
}
