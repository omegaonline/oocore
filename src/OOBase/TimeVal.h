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

#ifndef OOBASE_TIME_H_INCLUDED_
#define OOBASE_TIME_H_INCLUDED_

#include "config-base.h"

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

namespace OOBase
{
	struct timeval_t
	{
		/** \typedef time_64_t
		 *  A 64-bit signed integer.
		 */
#if defined(HAVE_STDINT_H)
		typedef int64_t time_64_t;
#elif defined(_MSC_VER)
		typedef __int64 time_64_t;
#else
#error Fix me!
#endif

		timeval_t() {}
		timeval_t(time_64_t s, int us = 0) :
				m_tv_sec(s), m_tv_usec(us)
		{
			normalise();
		}

		timeval_t& operator += (const timeval_t& rhs);
		timeval_t& operator -= (const timeval_t& rhs);

		bool operator == (const timeval_t& rhs) const
		{
			return (cmp(rhs) == 0);
		}

		bool operator != (const timeval_t& rhs) const
		{
			return (cmp(rhs) != 0);
		}

		bool operator < (const timeval_t& rhs) const
		{
			return (cmp(rhs) < 0);
		}

		bool operator <= (const timeval_t& rhs) const
		{
			return (cmp(rhs) <= 0);
		}

		bool operator > (const timeval_t& rhs) const
		{
			return (cmp(rhs) > 0);
		}

		bool operator >= (const timeval_t& rhs) const
		{
			return (cmp(rhs) >= 0);
		}

		time_64_t tv_sec() const
		{
			return m_tv_sec;
		}

		int tv_usec() const
		{
			return m_tv_usec;
		}

		unsigned long msec() const
		{
			return static_cast<unsigned long>((m_tv_sec * 1000) + (m_tv_usec / 1000));
		}

		static const timeval_t MaxTime;
		static const timeval_t Zero;

		static timeval_t deadline(unsigned long msec);

	private:
		int cmp(const timeval_t& rhs) const
		{
			if (m_tv_sec < rhs.m_tv_sec)
				return -1;
			else if (m_tv_sec > rhs.m_tv_sec)
				return 1;

			if (m_tv_usec < rhs.m_tv_usec)
				return -1;
			else if (m_tv_usec > rhs.m_tv_usec)
				return 1;

			return 0;
		}

		void normalise()
		{
			if (m_tv_usec >= 1000000)
			{
				int nsec = m_tv_usec / 1000000;
				m_tv_usec -= 1000000 * nsec;
				m_tv_sec += nsec;
			}
		}

		time_64_t m_tv_sec;    ///< Seconds since 01 Jan 1970 UTC
		int       m_tv_usec;   ///< Milliseconds since last second
	};

	inline timeval_t operator + (const timeval_t& t1, const timeval_t& t2)
	{
		timeval_t res = t1;
		return res += t2;
	}

	inline timeval_t operator - (const timeval_t& t1, const timeval_t& t2)
	{
		timeval_t res = t1;
		return res -= t2;
	}

	timeval_t gettimeofday();
	
	class Countdown
	{
	public:
		Countdown(timeval_t* wait);

		void update();

	private:
		Countdown(const Countdown&);
		Countdown& operator = (const Countdown&);

		timeval_t  m_start;
		timeval_t* m_wait;
	};
}

#endif // OOBASE_TIME_H_INCLUDED_
