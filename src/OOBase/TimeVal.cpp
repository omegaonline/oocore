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

const OOBase::timeval_t OOBase::timeval_t::max_time((time_t)LLONG_MAX, LONG_MAX);
const OOBase::timeval_t OOBase::timeval_t::zero(0, 0);

OOBase::timeval_t OOBase::gettimeofday()
{
	OOBase::timeval_t ret;

#if defined(_WIN32)

	static const ULONGLONG epoch = 116444736000000000LL;

	FILETIME file_time;
	GetSystemTimeAsFileTime(&file_time);
	
	ULARGE_INTEGER ularge;
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;

	// Move to epoch
	ULONGLONG q = (ularge.QuadPart - epoch);

	ret.tv_sec = static_cast<time_t>(q / 10000000LL);
	ret.tv_usec = static_cast<long>((q / 10L) % 1000000L);

#elif defined(HAVE_SYS_TIME_H) && (HAVE_SYS_TIME_H == 1)
	
	timeval tv;
	if (::gettimeofday(&tv,0) != 0)
		CallCriticalFailure();

	ret.tv_sec = tv.tv_sec;
	ret.tv_usec = tv.tv_usec;

#else
#error gettimeofday() !!
#endif

	return ret;
}

void OOBase::sleep(const timeval_t& wait)
{
#if defined(_WIN32)
	::Sleep(wait.msec());
#else
#error Fix me!
#endif
}

OOBase::timeval_t& OOBase::timeval_t::operator += (const timeval_t& rhs)
{
	if (tv_usec + rhs.tv_usec > 1000000) 
	{
		long nsec = (tv_usec + rhs.tv_usec) / 1000000;
		tv_usec -= 1000000 * nsec;
		tv_sec += nsec;
	}

	tv_sec += rhs.tv_sec;
	tv_usec += rhs.tv_usec;

	return *this;
}

OOBase::timeval_t& OOBase::timeval_t::operator -= (const timeval_t& rhs)
{
	/* Perform the carry for the later subtraction by updating r. */
	timeval_t r = rhs;
	if (tv_usec < r.tv_usec) 
	{
		long nsec = (r.tv_usec - tv_usec) / 1000000 + 1;
		r.tv_usec -= 1000000 * nsec;
		r.tv_sec += nsec;
	}
	
	if (tv_usec - r.tv_usec > 1000000) 
	{
		long nsec = (tv_usec - r.tv_usec) / 1000000;
		r.tv_usec += 1000000 * nsec;
		r.tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	 tv_usec is certainly positive. */
	tv_sec -= r.tv_sec;
	tv_usec -= r.tv_usec;

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
		*m_wait = timeval_t::zero;
	else
	{
		m_start = now;
		*m_wait -= diff;
	}
}
