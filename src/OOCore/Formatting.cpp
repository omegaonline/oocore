///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOCore_precomp.h"

using namespace Omega;

namespace 
{
	enum num_fmt
	{
		currency,
		decimal,
		scientific,
		fixed_point,
		general,
		number,
		round_trip,
		hexadecimal
	};

	bool is_quiet_nan(const double& val)
	{
		static const double nan = std::numeric_limits<double>::quiet_NaN();
		return *reinterpret_cast<const uint64_t*>(&val) == *reinterpret_cast<const uint64_t*>(&nan);
	}

	bool is_nan(const double& val)
	{
		static const double nan = std::numeric_limits<double>::signaling_NaN();
		return *reinterpret_cast<const uint64_t*>(&val) == *reinterpret_cast<const uint64_t*>(&nan);
	}

	bool is_p_infinity(const double& val)
	{
		return val == std::numeric_limits<double>::infinity();
	}

	bool is_n_infinity(const double& val)
	{
		return val == -std::numeric_limits<double>::infinity();
	}

	bool check_double(string_t& strText, const double& val)
	{
		if (is_quiet_nan(val))
		{
			strText = L"1.#IND";
			return true;
		}

		if (is_nan(val))
		{
			strText = L"1.#NAN";
			return true;
		}

		if (is_p_infinity(val))
		{
			strText = L"1.#INF";
			return true;
		}

		if (is_n_infinity(val))
		{
			strText = L"-1.#INF";
			return true;
		}

		return false;
	}

	bool parse_numfmt(const string_t& str, num_fmt& fmt, bool& capital, int& precision)
	{
		if (str.Length() > 3)
			return false;

		fmt = general;
		capital = true;
		precision = -1;

		if (str.Length() >= 1)
		{
			switch (str[0])
			{
			case L'c':
				capital = false;
			case L'C':
				fmt = currency;
				break;

			case L'd':
				capital = false;
			case L'D':
				fmt = decimal;
				break;

			case L'e':
				capital = false;
			case L'E':
				fmt = scientific;
				break;

			case L'f':
				capital = false;
			case L'F':
				fmt = fixed_point;
				break;

			case L'g':
				capital = false;
			case L'G':
				fmt = general;
				break;

			case L'n':
				capital = false;
			case L'N':
				fmt = number;
				break;

			case L'r':
				capital = false;
			case L'R':
				fmt = round_trip;
				break;

			case L'x':
				capital = false;
			case L'X':
				fmt = hexadecimal;
				break;

			default:
				return false;
			}
		}

		try
		{
			if (str.Length() >= 2)
				precision = OOCore::parse_uint(str[1]);

			if (str.Length() == 3)
			{
				precision *= 10;
				precision += OOCore::parse_uint(str[2]);
			}
		}
		catch (int)
		{
			return false;
		}

		return true;
	}

	////// START OF THE DEFINE STUFF

	std::string fmt_currency(lconv* lc, uint64_t val, int)
	{
		char* grouping = lc->mon_grouping;
		int grp = CHAR_MAX;
		int d = 0;

		std::string str;
		do
		{
			if (!d && *grouping)
				grp = *grouping++;

			str = static_cast<char>((val % 10) + '0') + str;
			val /= 10;

			if (grp!=CHAR_MAX && ++d==grp && val!=0)
			{
				str = lc->mon_thousands_sep + str;
				d = 0;
			}

		} while (val > 0);
		return str;
	}

	std::string fmt_currency(lconv* lc, int64_t val, int)
	{
		return fmt_currency(lc,static_cast<uint64_t>(val < 0 ? -val : val),-1);
	}

	std::string fmt_currency(lconv* lc, const double& val, int precision)
	{
		double int_part = 0.0;
		double frac_part = fabs(modf(val,&int_part));

		std::string ret = fmt_currency(lc,static_cast<int64_t>(int_part),-1);
		if (int_part == -0.0)
			ret = '-' + ret;

		if (!lc->mon_decimal_point || lc->mon_decimal_point[0] == '\0')
			ret += lc->decimal_point;
		else
			ret += lc->mon_decimal_point;

		if (precision < 0)
		{
			if (lc->frac_digits != CHAR_MAX)
				precision = lc->frac_digits;
			else
				precision = 2;
		}
		
		std::string dps;
		dps.reserve(std::numeric_limits<double>::digits10);
		do
		{
			frac_part = modf(frac_part * 10,&int_part);
			dps += static_cast<char>(int_part + '0');
		} while (frac_part != 0.0 && dps.size() < static_cast<size_t>(precision));

		if (precision >= 0 && static_cast<size_t>(precision) > dps.size())
			dps.append(precision-dps.size(),'0');

		return ret + dps;
	}

	template <typename T>
	string_t fmt_currency(T val, int precision)
	{
		lconv* lc = localeconv();
		if (!lc)
			OMEGA_THROW(L"Failed to retrieve locale information.");

		std::string str = fmt_currency(lc,val,precision);
		if (val >= 0)
		{
			switch (lc->p_sign_posn)
			{
			case 0:
			case 1:
			case 2:
				if (lc->p_cs_precedes)
				{
					if (lc->p_sep_by_space)
						str = lc->currency_symbol + (" " + str);
					else
						str = lc->currency_symbol + str;
				}
				else
				{
					if (lc->p_sep_by_space)
						str = str + ' ' + lc->currency_symbol;
					else
						str += lc->currency_symbol;
				}
				if (lc->p_sign_posn == 0)
					str = '(' + str + ')';
				else if (lc->p_sign_posn == 1)
					str = lc->positive_sign + str;
				else
					str += lc->positive_sign;
				break;

			case 3:
				if (lc->p_cs_precedes)
				{
					if (lc->p_sep_by_space)
						str = std::string(lc->positive_sign) + lc->currency_symbol + " " + str;
					else
						str = std::string(lc->positive_sign) + lc->currency_symbol + str;
				}
				else
				{
					if (lc->p_sep_by_space)
						str = str + " " + lc->positive_sign + lc->currency_symbol;
					else
						str = str + lc->positive_sign + lc->currency_symbol;
				}
				break;

			case 4:
				if (lc->p_cs_precedes)
				{
					if (lc->p_sep_by_space)
						str = lc->currency_symbol + std::string(lc->positive_sign) + " " + str;
					else
						str = lc->currency_symbol + std::string(lc->positive_sign) + str;
				}
				else
				{
					if (lc->p_sep_by_space)
						str = str + ' ' + lc->currency_symbol + lc->positive_sign;
					else
						str += lc->currency_symbol + std::string(lc->positive_sign);
				}
				break;

			default:
				str = lc->currency_symbol + str;
				break;
			}
		}
		else
		{
			switch (lc->n_sign_posn)
			{
			case 0:
			case 1:
			case 2:
				if (lc->n_cs_precedes)
				{
					if (lc->n_sep_by_space)
						str = lc->currency_symbol + (" " + str);
					else
						str = lc->currency_symbol + str;
				}
				else
				{
					if (lc->n_sep_by_space)
						str = str + ' ' + lc->currency_symbol;
					else
						str += lc->currency_symbol;
				}
				if (lc->n_sign_posn == 0)
					str = '(' + str + ')';
				else if (lc->n_sign_posn == 1)
					str = lc->negative_sign + str;
				else
					str += lc->negative_sign;
				break;

			case 3:
				if (lc->n_cs_precedes)
				{
					if (lc->n_sep_by_space)
						str = std::string(lc->negative_sign) + lc->currency_symbol + " " + str;
					else
						str = std::string(lc->negative_sign) + lc->currency_symbol + str;
				}
				else
				{
					if (lc->n_sep_by_space)
						str = str + " " + lc->negative_sign + lc->currency_symbol;
					else
						str = str + lc->negative_sign + lc->currency_symbol;
				}
				break;

			case 4:
				if (lc->n_cs_precedes)
				{
					if (lc->n_sep_by_space)
						str = lc->currency_symbol + std::string(lc->negative_sign) + " " + str;
					else
						str = lc->currency_symbol + std::string(lc->negative_sign) + str;
				}
				else
				{
					if (lc->n_sep_by_space)
						str = str + ' ' + lc->currency_symbol + lc->negative_sign;
					else
						str += lc->currency_symbol + std::string(lc->negative_sign);
				}
				break;

			default:
				if (lc->negative_sign && lc->negative_sign[0] != '\0')
					str = std::string(lc->negative_sign) + lc->currency_symbol + str;
				else
					str = std::string("-") + lc->currency_symbol + str;
				break;
			}
		}

		return string_t(str.c_str(),false);
	}

	std::string fmt_number(lconv* lc, uint64_t val, int)
	{
		char* grouping = lc->grouping;
		int grp = CHAR_MAX;
		int d = 0;

		std::string str;
		do
		{
			if (!d && *grouping)
				grp = *grouping++;

			str = static_cast<char>((val % 10) + '0') + str;
			val /= 10;

			if (grp!=CHAR_MAX && ++d==grp && val!=0)
			{
				str = lc->thousands_sep + str;
				d = 0;
			}

		} while (val > 0);
		return str;
	}

	std::string fmt_number(lconv* lc, int64_t val, int)
	{
		if (val < 0)
			return '-' + fmt_number(lc,static_cast<uint64_t>(-val),-1);
		else
			return fmt_number(lc,static_cast<uint64_t>(val),-1);
	}

	std::string fmt_number(lconv* lc, const double& val, int precision)
	{
		double int_part = 0.0;
		double frac_part = fabs(modf(val,&int_part));

		std::string ret = fmt_number(lc,static_cast<int64_t>(int_part),-1) + lc->decimal_point;
		if (int_part == -0.0)
			ret = '-' + ret;

		if (precision < 0)
		{
			if (lc->frac_digits != CHAR_MAX)
				precision = lc->frac_digits;
			else
				precision = 2;
		}
		
		std::string dps;
		dps.reserve(std::numeric_limits<double>::digits10);
		do
		{
			frac_part = modf(frac_part * 10,&int_part);
			dps += static_cast<char>(int_part + '0');
		} while (frac_part != 0.0 && dps.size() < static_cast<size_t>(precision));

		if (precision >= 0 && static_cast<size_t>(precision) > dps.size())
			dps.append(precision-dps.size(),'0');

		return ret + dps;
	}

	template <typename T>
	string_t fmt_number(T val, int precision)
	{
		lconv* lc = localeconv();
		if (!lc)
			OMEGA_THROW(L"Failed to retrieve locale information.");

		return string_t(fmt_number(lc,val,precision).c_str(),false);
	}

	////// END OF THE DEFINE STUFF

	string_t fmt_decimal(uint64_t val, int precision, bool zeros)
	{
		const size_t max_len = 99 + std::numeric_limits<uint64_t>::digits10;
		wchar_t szBuf[max_len+1] = {0};
		wchar_t* p = szBuf+max_len;

		do
		{
			*--p = (static_cast<wchar_t>(val % 10) + L'0');
			val /= 10;
		} while (val > 0);

		while (zeros && precision > (szBuf+max_len-p))
			*--p = L'0';
		
		return string_t(p);
	}

	string_t fmt_decimal(int64_t val, int precision, bool zeros)
	{
		if (val < 0)
			return L"-" + fmt_decimal(static_cast<uint64_t>(-val),precision,zeros);
		else
			return fmt_decimal(static_cast<uint64_t>(val),precision,zeros);
	}

	string_t fmt_hex(uint64_t val, bool capital, int precision)
	{
		const size_t max_len = 99 + std::numeric_limits<uint64_t>::digits10;
		wchar_t szBuf[max_len+1] = {0};
		wchar_t* p = szBuf+max_len;

		do
		{
			int v = static_cast<int>(val % 16);
			if (v >= 0 && v <= 9)
				*--p = static_cast<wchar_t>(v + L'0');
			else
				*--p = static_cast<wchar_t>(v-10 + (capital ? L'A' : L'a'));
			
			val /= 16;
		} while (val > 0);

		while (precision > (szBuf+max_len-p))
			*--p = L'0';
		
		return string_t(p);
	}

	string_t fmt_fixed(int64_t val, int precision, bool zeros)
	{
		return fmt_decimal(val,precision,zeros);
	}

	string_t fmt_fixed(uint64_t val, int precision, bool zeros)
	{
		return fmt_decimal(val,precision,zeros);
	}

	string_t fmt_fixed(const double& val, int precision, bool zeros)
	{
		double int_part = 0.0;
		double frac_part = fabs(modf(val,&int_part));

		std::wstring dps;
		string_t ret = fmt_decimal(static_cast<int64_t>(int_part),-1,false);
		if (int_part == -0.0)
			ret = L"-" + ret;

		if (precision > 0)
		{
			ret += L".";
				
			dps.reserve(std::numeric_limits<double>::digits10);
			do
			{
				frac_part = modf(frac_part * 10.0,&int_part);
				dps += static_cast<wchar_t>(int_part) + L'0';
			} while (frac_part != 0.0 && dps.size() < static_cast<size_t>(precision)-1);

			if (frac_part != 0.0)
			{
				// Round...
				dps += static_cast<wchar_t>(fabs(frac_part*10.0)) + L'0';
			}

			if (zeros && precision >= 0 && static_cast<size_t>(precision) > dps.size())
				dps.append(precision-dps.size(),L'0');
		}
		
		return ret + dps.c_str();		
	}

	string_t fmt_scientific(const double& val, bool capital, int precision, bool zeros)
	{
		if (precision < 0)
			precision = 6;

		double e = 0.0;
		if (val != 0.0)
			e = floor(log10(fabs(val)));

		string_t ret = fmt_fixed(val / pow(10.0,e),precision,zeros);
		ret += (capital ? L"E" : L"e");
		if (val >= 0.0)
			ret += L"+";
		ret += fmt_decimal(static_cast<int64_t>(e),3,true);
		return ret;
	}

	template <typename T>
	string_t fmt_scientific(T val, bool capital, int precision, bool zeros)
	{
		return fmt_scientific(static_cast<double>(val),capital,precision,zeros);
	}

	template <typename T>
	string_t fmt_general(T val, bool capital, int precision)
	{
		double e = 0;
		if (val != 0)
			e = floor(log10(fabs(static_cast<double>(val))));
		if (e > -5 && e < precision)
			return fmt_fixed(val,precision,false);
		else
			return fmt_scientific(val,capital,precision,false);
	}

	string_t fmt_round_trip(double val)
	{
		void* TODO;
		return fmt_fixed(val,-1,false);
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(string_t,OOCore_to_string_int_t,3,((in),int64_t,val,(in),const string_t&,strFormat,(in),size_t,byte_width))
{
	num_fmt fmt;
	bool capital;
	int precision;
	if (parse_numfmt(strFormat,fmt,capital,precision))
	{
		switch (fmt)
		{
		case currency:
			return fmt_currency(val,precision);

		case decimal:
		case round_trip:
			return fmt_decimal(val,precision,true);

		case scientific:
			return fmt_scientific(val,capital,precision,true);

		case fixed_point:
			return fmt_fixed(val,precision,true);

		case number:
			return fmt_number(val,precision);

		case hexadecimal:
			return fmt_hex(static_cast<uint64_t>(val),capital,precision);

		case general:
		default:
			if (precision <= 0)
			{
				switch (byte_width)
				{
				case sizeof(byte_t):
					precision = 3;
					break;

				case sizeof(uint16_t):
					precision = 5;
					break;

				case sizeof(uint32_t):
					precision = 10;
					break;

				case sizeof(uint64_t):
					precision = 19;
					break;

				default:
					break;
				}
			}
			return fmt_general(val,capital,precision);
		}
	}
	else
	{
		void* TODO;
		return fmt_fixed(val,6,false);
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(string_t,OOCore_to_string_uint_t,3,((in),uint64_t,val,(in),const string_t&,strFormat,(in),size_t,byte_width))
{
	num_fmt fmt;
	bool capital;
	int precision;
	if (parse_numfmt(strFormat,fmt,capital,precision))
	{
		switch (fmt)
		{
		case currency:
			return fmt_currency(val,precision);

		case decimal:
		case round_trip:
			return fmt_decimal(val,precision,true);

		case scientific:
			return fmt_scientific(val,capital,precision,true);

		case fixed_point:
			return fmt_fixed(val,precision,true);

		case number:
			return fmt_number(val,precision);

		case hexadecimal:
			return fmt_hex(val,capital,precision);

		case general:
		default:
			if (precision <= 0)
			{
				switch (byte_width)
				{
				case sizeof(byte_t):
					precision = 3;
					break;

				case sizeof(uint16_t):
					precision = 5;
					break;

				case sizeof(uint32_t):
					precision = 10;
					break;

				case sizeof(uint64_t):
					precision = 19;
					break;

				default:
					break;
				}
			}
			return fmt_general(val,capital,precision);
		}
	}
	else
	{
		void* TODO;
		return fmt_fixed(val,6,false);
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(string_t,OOCore_to_string_float_t,3,((in),float8_t,val,(in),const string_t&,strFormat,(in),size_t,byte_width))
{
	string_t ret;
	if (check_double(ret,val))
		return ret;

	num_fmt fmt;
	bool capital;
	int precision;
	if (parse_numfmt(strFormat,fmt,capital,precision))
	{
		switch (fmt)
		{
		case currency:
			return fmt_currency(val,precision);

		case scientific:
			return fmt_scientific(val,capital,precision,true);

		case decimal:
		case fixed_point:
			return fmt_fixed(val,precision,true);

		case number:
			return fmt_number(val,precision);

		case round_trip:
			return fmt_round_trip(val);

		case hexadecimal:
			return fmt_hex(static_cast<uint64_t>(val),capital,precision);

		case general:
		default:
			if (precision <= 0)
			{
				switch (byte_width)
				{
				case sizeof(float4_t):
					precision = 7;
					break;

				case sizeof(float8_t):
					precision = 15;
					break;

				default:
					break;
				}
			}
			return fmt_general(val,capital,precision);
		}
	}
	else
	{
		void* TODO;
		return fmt_fixed(val,6,false);
	}	
}

OMEGA_DEFINE_EXPORTED_FUNCTION(string_t,OOCore_to_string_bool_t,2,((in),bool_t,val,(in),const string_t&,strFormat))
{
	void* TODO;
	return val ? L"true" : L"false";
}

unsigned int OOCore::parse_uint_hex(wchar_t c)
{
	if (c >= L'0' && c <= L'9')
		return (c-L'0');
	else if (c >= L'A' && c <= L'F')
		return (c-L'A'+10);
	else if (c >= L'a' && c <= L'f')
		return (c-L'a'+10);
	else
		throw int(0);
}

unsigned int OOCore::parse_uint(wchar_t c)
{
	if (c >= L'0' && c <= L'9')
		return (c-L'0');
	else
		throw int(0);
}

unsigned int OOCore::parse_uint(const wchar_t* sz)
{
	unsigned int v = 0;
	const wchar_t* p = sz;
	if (*p == L'+')
		++p;

	try
	{
		while (*p >=L'0' && *p<=L'9')
		{
			unsigned int i = parse_uint(*p++);

			if (v > UINT_MAX/10)
				break;
			v *= 10;

			if (v > UINT_MAX-i)
				break;
			v += i;		
		}
	}
	catch (int)
	{}

	return v;
}

int OOCore::parse_int(const wchar_t* sz)
{
	bool bNeg = false;
	int v = 0;
	const wchar_t* p = sz;
	if (*p == L'+')
		++p;
	else if (*p == L'-')
	{
		++p;
		bNeg = true;
	}

	try
	{
		while (*p >=L'0' && *p<=L'9')
		{
			unsigned int i = parse_uint(*p++);

			if (v > INT_MAX/10)
				break;
			v *= 10;

			if ((unsigned int)v > INT_MAX-i)
				break;
			v += i;		
		}
	}
	catch (int)
	{}

	return (bNeg ? -v : v);
}
