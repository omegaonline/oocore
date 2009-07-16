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

namespace OOBase
{
	struct timeval_t
	{
		time_t tv_sec; ///< Seconds since 01 Jan 1970 UTC
		long tv_usec;  ///< Milliseconds since last second

		timeval_t() {}
		timeval_t(time_t s, long us = 0) : 
			tv_sec(s), tv_usec(us)
		{}

		timeval_t& operator += (const timeval_t& rhs);
		timeval_t& operator -= (const timeval_t& rhs);

		bool operator == (const timeval_t& rhs) const
		{
			return (tv_sec == rhs.tv_sec) && (tv_usec == rhs.tv_usec);
		}

		bool operator != (const timeval_t& rhs) const
		{
			return !(*this == rhs);
		}

		bool operator < (const timeval_t& rhs) const
		{
			return (tv_sec < rhs.tv_sec) || (tv_sec == rhs.tv_sec && tv_usec < rhs.tv_usec);
		}

		bool operator <= (const timeval_t& rhs) const
		{
			return (*this < rhs || *this == rhs);
		}

		bool operator > (const timeval_t& rhs) const
		{
			return !(*this <= rhs);
		}

		bool operator >= (const timeval_t& rhs) const
		{
			return !(*this < rhs);
		}

		unsigned long msec() const
		{
			return static_cast<unsigned long>((tv_sec * time_t(1000)) + (tv_usec / 1000));
		}

		static const timeval_t max_time;
		static const timeval_t zero;

		static timeval_t deadline(unsigned long msec);
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
