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

#if defined(_WIN32)

	/*std::wstring fmt_currency_i(uint64_t val, int)
	{
		wchar_t mon_grouping[12] = {0};
		if (!GetLocaleInfoW(LOCALE_USER_DEFAULT,LOCALE_SMONGROUPING,mon_grouping,12))
			OMEGA_THROW(GetLastError());

		wchar_t mon_sep[5] = {0};
		if (!GetLocaleInfoW(LOCALE_USER_DEFAULT,LOCALE_SMONTHOUSANDSEP,mon_sep,5))
			OMEGA_THROW(GetLastError());

		wchar_t* grouping = mon_grouping;
		int grp = -1;
		int d = 0;

		std::wstring str;
		do
		{
			if (!d && *grouping)
			{
				unsigned int g = OOCore::parse_uint(grouping);
				if (g)
				{
					grp = g;
					grouping = wcschr(grouping,L';');
					if (grouping)
						++grouping;
				}
			}

			str = static_cast<wchar_t>((val % 10) + L'0') + str;
			val /= 10;

			if (grp!=-1 && ++d==grp && val!=0)
			{
				str = mon_sep + str;
				d = 0;
			}

		} while (val > 0);
		return str;
	}

	std::wstring fmt_currency_i(int64_t val, int)
	{
		return fmt_currency_i(static_cast<uint64_t>(val < 0 ? -val : val),-1);
	}

	std::wstring fmt_currency_i(const double& val, int precision)
	{
		double int_part = 0.0;
		double frac_part = fabs(modf(val,&int_part));

		std::wstring ret = fmt_currency_i(static_cast<int64_t>(int_part),-1);
		
		if (precision < 0)
		{
			DWORD prec = 0;
			if (!GetLocaleInfoW(LOCALE_USER_DEFAULT,LOCALE_RETURN_NUMBER | LOCALE_ICURRDIGITS,(LPWSTR)&prec,sizeof(DWORD)))
				OMEGA_THROW(GetLastError());

			precision = prec;
		}

		wchar_t mon_dec[5] = {0};
		if (!GetLocaleInfoW(LOCALE_USER_DEFAULT,LOCALE_SMONDECIMALSEP,mon_dec,5))
			OMEGA_THROW(GetLastError());

		if (precision > 0)
			ret += mon_dec;
		
		std::wstring dps;
		dps.reserve(std::numeric_limits<double>::digits10);
		do
		{
			frac_part = modf(frac_part * 10.0,&int_part);
			dps += static_cast<wchar_t>(int_part + L'0');
		} while (frac_part != 0.0 && dps.size() < static_cast<size_t>(precision)-1);

		if (frac_part != 0.0)
		{
			// Round...
			if (frac_part >= 0.5)
				dps += static_cast<wchar_t>(ceil(frac_part*10.0)) + L'0';
			else
				dps += static_cast<wchar_t>(floor(frac_part*10.0)) + L'0';
		}

		if (precision >= 0 && static_cast<size_t>(precision) > dps.size())
			dps.append(precision-dps.size(),'0');

		return ret + dps;
	}
	
	template <typename T>
	string_t fmt_currency(T val, int precision)
	{
		std::wstring str = fmt_currency_i(val,precision);

		wchar_t symbol[8] = {0};
		if (!GetLocaleInfoW(LOCALE_USER_DEFAULT,LOCALE_SCURRENCY,symbol,8))
			OMEGA_THROW(GetLastError());

		if (val >= 0)
		{
			DWORD fmt = 0;
			if (!GetLocaleInfoW(LOCALE_USER_DEFAULT,LOCALE_RETURN_NUMBER | LOCALE_ICURRENCY,(LPWSTR)&fmt,sizeof(DWORD)))
				OMEGA_THROW(GetLastError());

			switch (fmt)
			{
			case 0:
			default:
				str = symbol + str;
				break;

			case 1:
				str += symbol;
				break;

			case 2:
				str = std::wstring(symbol) + L' ' + str;
				break;

			case 3:
				str += L' ';
				str += symbol;
				break;
			}
		}
		else
		{
			DWORD fmt = 0;
			if (!GetLocaleInfoW(LOCALE_USER_DEFAULT,LOCALE_RETURN_NUMBER | LOCALE_INEGCURR,(LPWSTR)&fmt,sizeof(DWORD)))
				OMEGA_THROW(GetLastError());

			wchar_t neg[6] = {0};
			if (!GetLocaleInfoW(LOCALE_USER_DEFAULT,LOCALE_SNEGATIVESIGN,neg,6))
				OMEGA_THROW(GetLastError());

			switch (fmt)
			{
			case 0:
				str = std::wstring(L"(") + symbol + str + L')';
				break;

			default:
			case 1:
				str = std::wstring(neg) + symbol + str;
				break;

			case 2:
				str = std::wstring(symbol) + neg + str;
				break;

			case 3:
				str = symbol + str + neg;
				break;

			case 4:
				str = L'(' + str + symbol + L')';
				break;

			case 5:
				str = neg + str + symbol;
				break;

			case 6:
				str += neg;
				str += symbol;
				break;

			case 7:
				str += symbol;
				str += neg;
				break;

			case 8:
				str = neg + str + L' ' + symbol;
				break;

			case 9:
				str = std::wstring(neg) + symbol + L' ' + str;
				break;

			case 10:
				str += L' ';
				str += symbol;
				str += neg;
				break;

			case 11:
				str = std::wstring(symbol) + L' ' + str + neg;
				break;

			case 12:
				str = std::wstring(symbol) + L' ' + neg + str;
				break;

			case 13:
				str += neg;
				str += L' ';
				str += symbol;
				break;

			case 14:
				str = std::wstring(L"(") + symbol + L' ' + str + L')';
				break;

			case 15:
				str = L'(' + str + L' ' + symbol + L')';
				break;
			}
		}
		return string_t(str.c_str());
	}

	std::wstring fmt_number_i(uint64_t val, int)
	{
		wchar_t mon_grouping[25] = {0};
		if (!GetLocaleInfoW(LOCALE_USER_DEFAULT,LOCALE_SGROUPING,mon_grouping,25))
			OMEGA_THROW(GetLastError());

		wchar_t mon_sep[5] = {0};
		if (!GetLocaleInfoW(LOCALE_USER_DEFAULT,LOCALE_STHOUSAND,mon_sep,5))
			OMEGA_THROW(GetLastError());

		wchar_t* grouping = mon_grouping;
		int grp = -1;
		int d = 0;

		std::wstring str;
		do
		{
			if (!d && *grouping)
			{
				unsigned int g = OOCore::parse_uint(grouping);
				if (g)
				{
					grp = g;
					grouping = wcschr(grouping,L';');
					if (grouping)
						++grouping;
				}
			}

			str = static_cast<wchar_t>((val % 10) + L'0') + str;
			val /= 10;

			if (grp!=-1 && ++d==grp && val!=0)
			{
				str = mon_sep + str;
				d = 0;
			}

		} while (val > 0);
		return str;
	}

	std::wstring fmt_number_i(int64_t val, int)
	{
		return fmt_number_i(static_cast<uint64_t>(val < 0 ? -val : val),-1);
	}

	std::wstring fmt_number_i(const double& val, int precision)
	{
		double int_part = 0.0;
		double frac_part = fabs(modf(val,&int_part));

		std::wstring ret = fmt_number_i(static_cast<int64_t>(int_part),-1);
		
		if (precision < 0)
		{
			DWORD curr_digits = 0;
			if (!GetLocaleInfoW(LOCALE_USER_DEFAULT,LOCALE_RETURN_NUMBER | LOCALE_IDIGITS,(LPWSTR)&curr_digits,sizeof(DWORD)))
				OMEGA_THROW(GetLastError());

			precision = curr_digits;
		}

		wchar_t mon_dec[5] = {0};
		if (!GetLocaleInfoW(LOCALE_USER_DEFAULT,LOCALE_SDECIMAL,mon_dec,5))
			OMEGA_THROW(GetLastError());

		if (precision > 0)
			ret += mon_dec;
		
		std::wstring dps;
		dps.reserve(std::numeric_limits<double>::digits10);
		do
		{
			frac_part = modf(frac_part * 10.0,&int_part);
			dps += static_cast<wchar_t>(int_part + L'0');
		} while (frac_part != 0.0 && dps.size() < static_cast<size_t>(precision)-1);

		if (frac_part != 0.0)
		{
			// Round...
			if (frac_part >= 0.5)
				dps += static_cast<wchar_t>(ceil(frac_part*10.0)) + L'0';
			else
				dps += static_cast<wchar_t>(floor(frac_part*10.0)) + L'0';
		}

		if (precision >= 0 && static_cast<size_t>(precision) > dps.size())
			dps.append(precision-dps.size(),'0');

		return ret + dps;
	}
	
	template <typename T>
	string_t fmt_number(T val, int precision)
	{
		std::wstring str = fmt_number_i(val,precision);

		if (val < 0)
		{
			wchar_t neg[6] = {0};
			if (!GetLocaleInfoW(LOCALE_USER_DEFAULT,LOCALE_SNEGATIVESIGN,neg,6))
				OMEGA_THROW(GetLastError());

			DWORD fmt = 0;
			if (!GetLocaleInfoW(LOCALE_USER_DEFAULT,LOCALE_RETURN_NUMBER | LOCALE_INEGNUMBER,(LPWSTR)&fmt,sizeof(DWORD)))
				OMEGA_THROW(GetLastError());

			switch (fmt)
			{
			case 0:
				str = L'(' + str + L')';
				break;

			default:
			case 1:
				str = neg + str;
				break;

			case 2:
				str = std::wstring(neg) + L' ' + str;
				break;

			case 3:
				str += neg;
				break;

			case 4:
				str += L' ';
				str += neg;
				break;
			}
		}
		return string_t(str.c_str());
	}*/

	template <typename T>
	string_t fmt_currency(T val, int precision)
	{
		string_t str = fmt_fixed(val,precision,false);
				
		int buf_size = GetCurrencyFormatW(LOCALE_USER_DEFAULT,0,str.c_str(),NULL,NULL,0);
		if (!buf_size)
			OMEGA_THROW(GetLastError());

		OOBase::SmartPtr<wchar_t,OOBase::ArrayDestructor<wchar_t> > buf;
		OMEGA_NEW(buf,wchar_t[buf_size]);
		if (!GetCurrencyFormatW(LOCALE_USER_DEFAULT,0,str.c_str(),NULL,buf.value(),buf_size))
			OMEGA_THROW(GetLastError());

		return string_t(buf.value());
	}

	template <typename T>
	string_t fmt_number(T val, int precision)
	{
		string_t str = fmt_fixed(val,precision,false);
				
		int buf_size = GetNumberFormatW(LOCALE_USER_DEFAULT,0,str.c_str(),NULL,NULL,0);
		if (!buf_size)
			OMEGA_THROW(GetLastError());

		OOBase::SmartPtr<wchar_t,OOBase::ArrayDestructor<wchar_t> > buf;
		OMEGA_NEW(buf,wchar_t[buf_size]);
		if (!GetNumberFormatW(LOCALE_USER_DEFAULT,0,str.c_str(),NULL,buf.value(),buf_size))
			OMEGA_THROW(GetLastError());

		return string_t(buf.value());
	}

#else

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
	
		if (precision < 0)
		{
			if (lc->frac_digits != CHAR_MAX)
				precision = lc->frac_digits;
		}

		if (precision != 0)
		{
			if (!lc->mon_decimal_point || lc->mon_decimal_point[0] == '\0')
				ret += lc->decimal_point;
			else
				ret += lc->mon_decimal_point;
		}
		
		std::string dps;
		dps.reserve(std::numeric_limits<double>::digits10);
		do
		{
			frac_part = modf(frac_part * 10.0,&int_part);
			dps += static_cast<char>(int_part + '0');
		} while (frac_part != 0.0 && dps.size() < static_cast<size_t>(precision)-1);

		if (frac_part != 0.0)
		{
			// Round...
			if (frac_part >= 0.5)
				dps += static_cast<char>(ceil(frac_part*10.0)) + '0';
			else
				dps += static_cast<char>(floor(frac_part*10.0)) + '0';
		}

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
		
		if (precision < 0)
		{
			if (lc->frac_digits != CHAR_MAX)
				precision = lc->frac_digits;
		}

		if (precision != 0)
			ret += lc->decimal_point;
					
		std::string dps;
		dps.reserve(std::numeric_limits<double>::digits10);
		do
		{
			frac_part = modf(frac_part * 10.0,&int_part);
			dps += static_cast<char>(int_part + '0');
		} while (frac_part != 0.0 && dps.size() < static_cast<size_t>(precision)-1);

		if (frac_part != 0.0)
		{
			// Round...
			if (frac_part >= 0.5)
				dps += static_cast<char>(ceil(frac_part*10.0)) + '0';
			else
				dps += static_cast<char>(floor(frac_part*10.0)) + '0';
		}

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

#endif // !_WIN32

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
		
		if (precision != 0)
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
				if (frac_part >= 0.5)
					dps += static_cast<wchar_t>(ceil(frac_part*10.0)) + L'0';
				else
					dps += static_cast<wchar_t>(floor(frac_part*10.0)) + L'0';
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

	size_t find_skip_quote(const string_t& str, size_t start, wchar_t c)
	{
		string_t strFind = L"'\"";
		strFind += c;
		size_t pos = start;
		
		for (;;)
		{
			size_t found = str.FindOneOf(strFind,pos);
			if (found == string_t::npos)
				return found;

			if (str[found] == L'\'')
			{
				found = str.Find(L'\'',found+1);
				if (found == string_t::npos)
					return found;

				pos = found+1;
			}
			else if (str[found] == L'"')
			{
				found = str.Find(L'"',found+1);
				if (found == string_t::npos)
					return found;

				pos = found+1;
			}
			else
				return found;
		}
	}

	int parse_custom(const string_t& str, std::vector<string_t>& parts)
	{
		size_t pos = find_skip_quote(str,0,L';');
		if (pos == string_t::npos)
			parts.push_back(str);
		else
		{
			parts.push_back(string_t(str.c_str(),pos++));
			size_t pos2 = find_skip_quote(str,pos,L';');
			if (pos2 == string_t::npos)
				parts.push_back(string_t(str.c_str()+pos));
			else
			{
				parts.push_back(string_t(str.c_str()+pos,pos2-pos));
				parts.push_back(string_t(str.c_str()+pos2+1));
			}
		}
		return parts.size();
	}

	template <typename T>
	string_t fmt_custom(T val, const string_t& strFormat, size_t byte_width);

	template <typename T>
	string_t fmt_custom_i(T val, const string_t& /*strFormat*/, size_t byte_width)
	{
		void* TODO;
		return fmt_recurse(val,L"G",byte_width,false);
	}

	string_t fmt_recurse(int64_t val, const string_t& strFormat, size_t byte_width, bool bRecurse)
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
		else if (bRecurse)
		{
			return fmt_custom(val,strFormat,byte_width);
		}
		else
		{
			return fmt_custom_i(val,strFormat,byte_width);
		}
	}

	string_t fmt_recurse(uint64_t val, const string_t& strFormat, size_t byte_width, bool bRecurse)
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
		else if (bRecurse)
		{
			return fmt_custom(val,strFormat,byte_width);
		}
		else
		{
			return fmt_custom_i(val,strFormat,byte_width);
		}
	}

	string_t fmt_recurse(float8_t val, const string_t& strFormat, size_t byte_width, bool bRecurse)
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
		else if (bRecurse)
		{
			return fmt_custom(val,strFormat,byte_width);
		}
		else
		{
			return fmt_custom_i(val,strFormat,byte_width);
		}
	}

	template <typename T>
	string_t fmt_custom(T val, const string_t& strFormat, size_t byte_width)
	{
		std::vector<string_t> parts;
		switch (parse_custom(strFormat,parts))
		{
		case 3:
			if (val == 0)
				return fmt_recurse(val,parts[2],byte_width,false);

		case 2:
			if (val < 0)
				return fmt_recurse(val,parts[1],byte_width,false);
			else
				return fmt_recurse(val,parts[0],byte_width,false);

		default:
			return fmt_custom_i(val,strFormat,byte_width);
		}
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(string_t,OOCore_to_string_int_t,3,((in),int64_t,val,(in),const string_t&,strFormat,(in),size_t,byte_width))
{
	return fmt_recurse(val,strFormat,byte_width,true);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(string_t,OOCore_to_string_uint_t,3,((in),uint64_t,val,(in),const string_t&,strFormat,(in),size_t,byte_width))
{
	return fmt_recurse(val,strFormat,byte_width,true);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(string_t,OOCore_to_string_float_t,3,((in),float8_t,val,(in),const string_t&,strFormat,(in),size_t,byte_width))
{
	string_t ret;
	if (check_double(ret,val))
		return ret;

	return fmt_recurse(val,strFormat,byte_width,true);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(string_t,OOCore_to_string_bool_t,2,((in),bool_t,val,(in),const string_t&,strFormat))
{
	if (strFormat.IsEmpty())
		return val ? L"true" : L"false";

	std::vector<string_t> parts;
	parse_custom(strFormat,parts);

	if (parts.size() != 2)
		OMEGA_THROW(L"Invalid bool_t format string {0}" % strFormat);

	return val ? parts[0] : parts[1];
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
