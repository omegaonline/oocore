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
#if defined(_MSC_VER)
		typedef __int64 time_64_t;
#elif defined(HAVE_STDINT_H)
		typedef int64_t time_64_t;
#else
#error Fix me!
#endif

		time_64_t tv_sec;    ///< Seconds since 01 Jan 1970 UTC
		int       tv_usec;   ///< Milliseconds since last second

		timeval_t() {}
		timeval_t(time_64_t s, int us = 0) :
			tv_sec(s), tv_usec(us)
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

		unsigned long msec() const
		{
			return static_cast<unsigned long>((tv_sec * 1000) + (tv_usec / 1000));
		}

		static const timeval_t max_time;
		static const timeval_t zero;

		static timeval_t deadline(unsigned long msec);

		void normalise()
		{
			if (tv_usec >= 1000000)
			{
				long nsec = tv_usec / 1000000;
				tv_usec -= 1000000 * nsec;
				tv_sec += nsec;
			}
		}

	private:
		int cmp(const timeval_t& rhs) const
		{
			if (tv_sec < rhs.tv_sec)
				return -1;
			else if (tv_sec > rhs.tv_sec)
				return 1;

			if (tv_usec < rhs.tv_usec)
				return -1;
			else if (tv_usec > rhs.tv_usec)
				return 1;

			return 0;
		}
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
	void sleep(const timeval_t& wait);

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
