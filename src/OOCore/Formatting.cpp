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

#if defined(HAVE_CONFIG_H)
#include <autoconf.h>
#endif

#include <limits.h>

using namespace Omega;
using namespace OTL;

namespace
{
	class FormattingException :
			public ExceptionImpl<Formatting::IFormattingException>
	{
	public:
		BEGIN_INTERFACE_MAP(FormattingException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<Formatting::IFormattingException>)
		END_INTERFACE_MAP()
	};

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

		if (str.Length() >= 2)
		{
			if (iswdigit(str[1]))
			{
				const wchar_t* endp = 0;
				precision = OOCore::wcsto32(str.c_str()+1,endp,10);
				if (*endp != L'\0')
					return false;
			}
		}

		return true;
	}

	void do_intl_mon(std::string& str, bool negative)
	{
#if defined(_WIN32)
		LCID lcid = GetThreadLocale();

		char decimal_point[5] = {0};
		if (!GetLocaleInfoA(lcid,LOCALE_SMONDECIMALSEP,decimal_point,sizeof(decimal_point)))
		{
			DWORD dwErr = GetLastError();
			OMEGA_THROW(dwErr);
		}

		char grouping_buf[128] = {0};
		if (!GetLocaleInfoA(lcid,LOCALE_SMONGROUPING,grouping_buf,sizeof(grouping_buf)))
		{
			DWORD dwErr = GetLastError();
			OMEGA_THROW(dwErr);
		}

		// Build a crt style grouping
		char grouping_trans[128] = {0};
		const char* grouping = grouping_trans;
		size_t g = 0;
		for (const char* c = grouping_buf; *c != '\0' && g<sizeof(grouping_trans)-1; ++g)
		{
			grouping_trans[g] = static_cast<char>(atoi(c));
			if (!grouping_trans[g])
				break;

			c = strchr(c,';');
			if (!c)
				break;
			++c;
		}

		grouping_trans[g+1] = CHAR_MAX;

		char thousands_sep[5] = {0};
		if (!GetLocaleInfoA(lcid,LOCALE_SMONTHOUSANDSEP,thousands_sep,sizeof(thousands_sep)))
		{
			DWORD dwErr = GetLastError();
			OMEGA_THROW(dwErr);
		}
#else
		const char* decimal_point = ".";
		const char* grouping = "\x03\x00";
		const char* thousands_sep = ",";

		const lconv* lc = localeconv();
		if (lc)
		{
			decimal_point = lc->mon_decimal_point;
			grouping = lc->mon_grouping;
			thousands_sep = lc->mon_thousands_sep;
		}
#endif

		size_t pos = str.find('.');
		if (pos != std::string::npos)
			str.replace(pos,1,decimal_point);
		else
			pos = str.length();

		for (int grp = CHAR_MAX;;)
		{
			char n_grp = *grouping;
			if (n_grp != 0)
			{
				grp = n_grp;
				++grouping;
			}

			if (grp == CHAR_MAX)
				break;

			if (pos <= static_cast<size_t>(grp))
				break;

			pos -= grp;
			str.insert(pos,thousands_sep);
		}

		bool cs_precedes = false;
		bool sep_by_space = false;
		int posn = 0;
		std::string sign;
		std::string currency;

#if defined(_WIN32)
		if (negative)
		{
			DWORD val = 0;
			if (!GetLocaleInfoA(lcid,LOCALE_INEGSEPBYSPACE | LOCALE_RETURN_NUMBER,(LPSTR)&val,sizeof(val)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}
			sep_by_space = (val == 1);

			if (!GetLocaleInfoA(lcid,LOCALE_INEGSYMPRECEDES | LOCALE_RETURN_NUMBER,(LPSTR)&val,sizeof(val)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}
			cs_precedes = (val == 1);

			if (!GetLocaleInfoA(lcid,LOCALE_INEGSIGNPOSN | LOCALE_RETURN_NUMBER,(LPSTR)&val,sizeof(val)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}
			posn = static_cast<int>(val);

			char sign_symbol[6];
			if (!GetLocaleInfoA(lcid,LOCALE_SNEGATIVESIGN,sign_symbol,sizeof(sign_symbol)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}

			sign = sign_symbol;
		}
		else
		{
			DWORD val = 0;
			if (!GetLocaleInfoA(lcid,LOCALE_IPOSSEPBYSPACE | LOCALE_RETURN_NUMBER,(LPSTR)&val,sizeof(val)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}
			sep_by_space = (val == 1);

			if (!GetLocaleInfoA(lcid,LOCALE_IPOSSYMPRECEDES | LOCALE_RETURN_NUMBER,(LPSTR)&val,sizeof(val)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}
			cs_precedes = (val == 1);

			if (!GetLocaleInfoA(lcid,LOCALE_IPOSSIGNPOSN | LOCALE_RETURN_NUMBER,(LPSTR)&val,sizeof(val)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}
			posn = static_cast<int>(val);

			char sign_symbol[6];
			if (!GetLocaleInfoA(lcid,LOCALE_SPOSITIVESIGN,sign_symbol,sizeof(sign_symbol)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}

			sign = sign_symbol;
		}

		char currency_buf[8];
		if (!GetLocaleInfoA(lcid,LOCALE_SCURRENCY,currency_buf,sizeof(currency)))
		{
			DWORD dwErr = GetLastError();
			OMEGA_THROW(dwErr);
		}

		currency = currency_buf;

#else
		if (negative)
		{
			cs_precedes = (lc->n_cs_precedes == 1);
			sep_by_space = (lc->n_sep_by_space == 1);
			posn = lc->n_sign_posn;
			sign = lc->negative_sign;
		}
		else
		{
			cs_precedes = (lc->p_cs_precedes == 1);
			sep_by_space = (lc->p_sep_by_space == 1);
			posn = lc->p_sign_posn;
			sign = lc->positive_sign;
		}
		currency = lc->currency_symbol;
#endif

		switch (posn)
		{
		case 0:
			if (cs_precedes)
				str = (sep_by_space ? currency + ' ' : currency) + str;
			else
				str += (sep_by_space ? ' ' + currency : currency);
			str = '(' + str + ')';
			break;

		case 1:
			if (cs_precedes)
				str = (sep_by_space ? currency + ' ' : currency) + str;
			else
				str += (sep_by_space ? ' ' + currency : currency);
			str = sign + str;
			break;

		case 2:
			if (cs_precedes)
				str = (sep_by_space ? currency + ' ' : currency) + str;
			else
				str += (sep_by_space ? ' ' + currency : currency);
			str += sign;
			break;

		case 3:
			currency = sign + currency;
			if (cs_precedes)
				str = (sep_by_space ? currency + ' ' : currency) + str;
			else
				str += (sep_by_space ? ' ' + currency : currency);
			break;

		case 4:
		default:
			currency += sign;
			if (cs_precedes)
				str = (sep_by_space ? currency + ' ' : currency) + str;
			else
				str += (sep_by_space ? ' ' + currency : currency);
			break;
		}
	}

	size_t do_intl(std::string& str, bool bThou)
	{
#if defined(_WIN32)
		LCID lcid = GetThreadLocale();

		char decimal_point[5] = {0};
		if (!GetLocaleInfoA(lcid,LOCALE_SDECIMAL,decimal_point,sizeof(decimal_point)))
		{
			DWORD dwErr = GetLastError();
			OMEGA_THROW(dwErr);
		}

		char thousands_sep[5] = {0};
		char trans_grouping[128] = {0};
		const char* grouping = trans_grouping;

		if (bThou)
		{
			char grouping_buf[128] = {0};
			if (!GetLocaleInfoA(lcid,LOCALE_SGROUPING,grouping_buf,sizeof(grouping_buf)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}

			// Build a crt style grouping

			size_t g = 0;
			for (const char* c = grouping_buf; *c != '\0' && g<sizeof(trans_grouping)-1; ++g)
			{
				trans_grouping[g] = static_cast<char>(atoi(c));
				if (!trans_grouping[g])
					break;

				c = strchr(c,';');
				if (!c)
					break;
				++c;
			}

			trans_grouping[g+1] = CHAR_MAX;

			if (!GetLocaleInfoA(lcid,LOCALE_STHOUSAND,thousands_sep,sizeof(thousands_sep)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}
		}
#else
		const char* decimal_point = ".";
		const char* grouping = "\x03\x00";
		const char* thousands_sep = ",";

		const lconv* lc = localeconv();
		if (lc)
		{
			decimal_point = lc->mon_decimal_point;
			grouping = lc->mon_grouping;
			thousands_sep = lc->mon_thousands_sep;
		}
#endif

		size_t dp = str.find('.');
		if (dp != std::string::npos)
			str.replace(dp,1,decimal_point);

		if (bThou)
		{
			size_t pos = dp;
			if (pos == std::string::npos)
				pos = str.length();

			for (int grp = CHAR_MAX;;)
			{
				char n_grp = *grouping;
				if (n_grp != 0)
				{
					grp = n_grp;
					++grouping;
				}

				if (grp == CHAR_MAX)
					break;

				if (pos <= static_cast<size_t>(grp))
					break;

				pos -= grp;
				str.insert(pos,thousands_sep);
			}
		}

		return dp;
	}

	std::string fmt_fixed_i(const double& val, int precision)
	{
		std::ostringstream ss;
		ss.imbue(std::locale::classic());

		ss.setf(std::ios_base::fixed,std::ios_base::floatfield);

		ss.precision(precision);

		ss << val;

		return ss.str();
	}

	template <typename T>
	std::string fmt_fixed_i(T val, int precision)
	{
		std::ostringstream ss;
		ss.imbue(std::locale::classic());

		ss << val;

		std::string ret = ss.str();

		if (precision > 0)
		{
			ret += '.';
			ret.append(precision,'0');
		}

		return ret;
	}

	template <typename T>
	string_t fmt_currency(T val, int precision)
	{
		if (precision < 0)
		{
#if defined(_WIN32)
			LCID lcid = GetThreadLocale();

			DWORD v = 0;
			if (!GetLocaleInfoA(lcid,LOCALE_IDIGITS | LOCALE_RETURN_NUMBER,(LPSTR)&v,sizeof(v)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}

			precision = static_cast<int>(v);
#else
			precision = 2;
			const lconv* lc = localeconv();
			if (lc && lc->frac_digits != CHAR_MAX)
				precision = lc->frac_digits;
#endif
		}

		std::string ret = fmt_fixed_i(val,precision);
		if (!ret.empty() && (ret[0]=='-' || ret[0]=='+'))
			ret = ret.substr(1);

		do_intl_mon(ret,val < 0.0);

		return string_t(ret.c_str(),false);
	}

	template <typename T>
	string_t fmt_number(T val, int precision)
	{
		if (precision < 0)
		{
#if defined(_WIN32)
			LCID lcid = GetThreadLocale();

			DWORD v = 0;
			if (!GetLocaleInfoA(lcid,LOCALE_IDIGITS | LOCALE_RETURN_NUMBER,(LPSTR)&v,sizeof(v)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}

			precision = static_cast<int>(v);
#else
			precision = 2;
			const lconv* lc = localeconv();
			if (lc && lc->frac_digits != CHAR_MAX)
				precision = lc->frac_digits;
#endif
		}

		std::string ret = fmt_fixed_i(val,precision);

		do_intl(ret,true);

		return string_t(ret.c_str(),false);
	}

	template <typename T>
	std::string fmt_decimal_i(T val, int width)
	{
		std::ostringstream ss;
		ss.imbue(std::locale::classic());

		if (width >= 0)
		{
			ss.fill('0');
			ss.width(width);
		}

		ss << val;

		return ss.str();
	}

	std::string fmt_decimal_i(const double& val, int width)
	{
		std::ostringstream ss;
		ss.imbue(std::locale::classic());

		ss.setf(std::ios_base::fixed,std::ios_base::floatfield);
		ss.precision(0);

		if (width >= 0)
		{
			ss.fill('0');
			ss.width(width);
		}

		ss << val;

		return ss.str();
	}

	template <typename T>
	string_t fmt_decimal(T val, int width)
	{
		return string_t(fmt_decimal_i(val,width).c_str(),false);
	}

	template <typename T>
	string_t fmt_hex(T val, bool capital, int precision)
	{
		std::ostringstream ss;
		ss.imbue(std::locale::classic());

		ss.setf(std::ios_base::hex,std::ios_base::basefield);
		if (capital)
			ss.setf(std::ios_base::uppercase);

		if (precision >= 0)
		{
			ss.fill('0');
			ss.width(precision);
		}

		ss << val;

		return string_t(ss.str().c_str(),false);
	}

	template <typename T>
	string_t fmt_fixed(T val, int precision)
	{
		if (precision < 0)
		{
#if defined(_WIN32)
			LCID lcid = GetThreadLocale();

			DWORD v = 0;
			if (!GetLocaleInfoA(lcid,LOCALE_IDIGITS | LOCALE_RETURN_NUMBER,(LPSTR)&v,sizeof(v)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}

			precision = static_cast<int>(v);
#else
			precision = 2;
			const lconv* lc = localeconv();
			if (lc && lc->frac_digits != CHAR_MAX)
				precision = lc->frac_digits;
#endif
		}

		std::string ret = fmt_fixed_i(val,precision);

		do_intl(ret,false);

		return string_t(ret.c_str(),false);
	}

	std::string exp_strip(const std::string& str, int precision, bool show_plus)
	{
		assert(precision >= 0);

		std::string ret = str;
		size_t pos = ret.find_first_of("Ee");
		if (pos == std::string::npos || pos == ret.length()-1)
			return str;

		// Remove + if !exp_plus
		++pos;
		if (ret[pos] == '+')
		{
			if (!show_plus)
				ret.erase(pos,1);
			else
				++pos;
		}
		else if (str[pos] == '-')
			++pos;

		// Remove leading zeros if > precision
		while (pos < ret.length()-precision && ret[pos] == '0')
			ret.erase(pos,1);

		// Insert any extra zero's required
		if (ret.length() < pos+precision)
			ret.insert(pos,pos+precision-ret.length(),'0');

		return ret;
	}

	std::string fmt_scientific_i(const double& val, bool capital, int precision)
	{
		std::ostringstream ss;
		ss.imbue(std::locale::classic());

		ss.setf(std::ios_base::scientific,std::ios_base::floatfield);

		if (precision >= 0)
			ss.precision(precision);

		if (capital)
			ss.setf(std::ios_base::uppercase);

		ss << val;

		return ss.str();
	}

	template <typename T>
	std::string fmt_scientific_i(T val, bool capital, int precision)
	{
		return fmt_scientific_i(static_cast<double>(val),capital,precision);
	}

	template <typename T>
	string_t fmt_scientific(T val, bool capital, int precision)
	{
		std::string ret = fmt_scientific_i(val,capital,precision);

		do_intl(ret,false);
		ret = exp_strip(ret,3,true);

		return string_t(ret.c_str(),false);
	}

	string_t fmt_general(const double& val, bool capital, int precision)
	{
		std::ostringstream ss;
		ss.imbue(std::locale::classic());

		if (capital)
			ss.setf(std::ios_base::uppercase);

		ss.precision(precision);

		ss << val;

		return string_t(exp_strip(ss.str(),0,false).c_str(),false);
	}

	template <typename T>
	string_t fmt_general(T val, bool /*capital*/, int /*precision*/)
	{
		std::ostringstream ss;
		ss.imbue(std::locale::classic());

		ss << val;

		return string_t(ss.str().c_str(),false);
	}

	template <typename T>
	string_t fmt_round_trip(T val, int /*precision*/)
	{
		return fmt_decimal(val,-1);
	}

	string_t fmt_round_trip(const double& val, int precision)
	{
		string_t s;
		double p = 0.0;
		for (int i=0; i<10 && val != p; ++i)
		{
			s = fmt_general(val,false,precision+i);

			const wchar_t* end = 0;
			p = OOCore::wcstod(s.c_str(),end);
		}

		return s;
	}

	size_t find_skip_quote(const string_t& str, size_t start, const string_t& strFind)
	{
		for (size_t pos = start;;)
		{
			size_t found = str.FindOneOf(L"'\"" + strFind,pos);
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

	size_t parse_custom(const string_t& str, std::vector<string_t>& parts)
	{
		size_t pos = find_skip_quote(str,0,L";");
		if (pos == string_t::npos)
			parts.push_back(str);
		else
		{
			parts.push_back(string_t(str.c_str(),pos++));
			size_t pos2 = find_skip_quote(str,pos,L";");
			if (pos2 == string_t::npos)
				parts.push_back(string_t(str.c_str()+pos,string_t::npos));
			else
			{
				parts.push_back(string_t(str.c_str()+pos,pos2-pos));
				parts.push_back(string_t(str.c_str()+pos2+1,string_t::npos));
			}
		}
		return parts.size();
	}

	template <typename T>
	string_t fmt_custom(T val, const string_t& strFormat, int def_precision);

	template <typename T>
	string_t fmt_recurse(T val, const string_t& strFormat, int def_precision, bool bRecurse);

	template <typename T>
	string_t fmt_rnd_away(T val)
	{
		return fmt_decimal(val,-1);
	}

	string_t fmt_rnd_away(const double& val)
	{
		double res = ceil(fabs(val));
		return fmt_decimal(res < 0.0 ? -res : res,-1);
	}

	template <typename T>
	T quick_abs(T val, bool& negative)
	{
		negative = (val < 0);
		return (negative ? -val : val);
	}

	uint64_t quick_abs(uint64_t val, bool& negative)
	{
		negative = false;
		return val;
	}

	template <typename T>
	string_t fmt_custom_i(T val, const string_t& strFormat)
	{
		if (strFormat == L"##" || strFormat == L"00")
			return fmt_rnd_away(val);

		bool negative;
		T abs_val = quick_abs(val,negative);

#if defined(_WIN32)
		LCID lcid = GetThreadLocale();

		char decimal_point[5] = {0};
		if (!GetLocaleInfoA(lcid,LOCALE_SDECIMAL,decimal_point,5))
		{
			DWORD dwErr = GetLastError();
			OMEGA_THROW(dwErr);
		}

		std::string decimal_char(decimal_point);
		string_t decimal(decimal_point,false);

		char thousands_sep[5] = {0};
		if (!GetLocaleInfoA(lcid,LOCALE_STHOUSAND,thousands_sep,5))
		{
			DWORD dwErr = GetLastError();
			OMEGA_THROW(dwErr);
		}

		std::string thousands_char(thousands_sep);
		string_t thousands(thousands_sep,false);
#else
		std::string decimal_char(".");
		std::string thousands_char(",");

		const lconv* lc = localeconv();
		if (lc)
		{
			decimal_char = lc->decimal_point;
			thousands_char = lc->thousands_sep;
		}

		string_t decimal(decimal_char.c_str(),false);
		string_t thousands(thousands_char.c_str(),false);
#endif

		string_t strFind(L"0#Ee");
		strFind += decimal[0];
		strFind += thousands[0];

		// Work out precision and mode...
		size_t sci = string_t::npos;
		size_t dec_place = string_t::npos;
		bool group = false;
		int precision = 0;
		int width = 0;
		int exp_digits = 0;

		for (size_t pos = 0; pos < strFormat.Length();)
		{
			size_t found = find_skip_quote(strFormat,pos,strFind);
			if (found == string_t::npos)
				break;

			if (strFormat[found] == L'E' || strFormat[found] == L'e')
			{
				if (found < strFormat.Length()-1)
				{
					if (strFormat[found+1] == L'-' || strFormat[found+1] == L'+')
					{
						if (found < strFormat.Length()-2 && strFormat[found+2] == L'0')
						{
							sci = found;
							found += 2;
						}
					}
					else if (strFormat[found+1] == L'0')
					{
						sci = found;
						++found;
					}

					if (sci != string_t::npos)
					{
						for (; strFormat[found] == L'0' && found<strFormat.Length(); ++found)
							++exp_digits;

						break;
					}
				}
			}
			else if (strFormat[found] == L'#' || strFormat[found] == L'0')
			{
				if (dec_place != string_t::npos)
					++precision;
				else
					++width;
			}
			else if (strFormat.Compare(decimal,found,decimal.Length()) == 0)
			{
				if (dec_place == string_t::npos)
				{
					dec_place = found;
					found += decimal.Length()-1;
				}
			}
			else if (strFormat.Compare(thousands,found,thousands.Length()) == 0)
			{
				if (!group && dec_place == string_t::npos &&
						found > 0 && found < strFormat.Length()-1 &&
						(strFormat[found-1]==L'#' || strFormat[found-1]==L'0') &&
						(strFormat[found+1]==L'#' || strFormat[found+1]==L'0'))
				{
					group = true;
					found += thousands.Length()-1;
				}
			}

			pos = found + 1;
		}

		// Create the base number string
		std::string number;
		if (sci != string_t::npos)
		{
			number = fmt_scientific_i(abs_val,strFormat[sci] == L'E',precision);
			number = exp_strip(number,exp_digits,strFormat[sci+1] == L'+');
		}
		else if (dec_place != string_t::npos)
			number = fmt_fixed_i(abs_val,precision);
		else
			number = fmt_decimal_i(abs_val,width);

		if (sci != string_t::npos || dec_place != string_t::npos)
		{
			// Add extra leading zero's
			size_t dp = number.find('.');
			if (dp == std::string::npos)
			{
				dp = number.length();
				number += '.';
			}

			if (static_cast<size_t>(width) > dp)
				number.insert(0,width-dp,'0');
		}

		// Translate to intl format
		size_t dp = do_intl(number,group);

		// Transpose number into format
		string_t res;
		bool seen_decimal = false;
		bool sig_zero = false;
		bool done_neg = false;
		size_t numpos = 0;
		group = false;
		for (size_t pos = 0; pos < strFormat.Length(); ++pos)
		{
			wchar_t wc = strFormat[pos];
			switch (wc)
			{
			case L'\'':
				pos = strFormat.Find(L'\'',pos+1);
				break;

			case L'"':
				pos = strFormat.Find(L'"',pos+1);
				break;

			case L'#':
				{
					size_t len = 1;
					if (group && number.compare(numpos+1,thousands_char.length(),thousands_char) == 0)
						len += thousands_char.length();

					if (sig_zero || number[numpos] != '0')
					{
						if (!done_neg && negative)
						{
							res += L'-';
							done_neg = true;
						}

						if (numpos + len < dp && --width == 0)
							len += (dp == std::string::npos ? number.length() : dp) - (numpos + len);

						res += string_t(number.substr(numpos,len).c_str(),false);
						if (number[numpos] != '0')
							sig_zero = !seen_decimal;
					}
					numpos += len;
				}
				break;

			case L'0':
				{
					size_t len = 1;
					if (group && number.compare(numpos+1,thousands_char.length(),thousands_char) == 0)
						len += thousands_char.length();

					if (!done_neg && negative)
					{
						res += L'-';
						done_neg = true;
					}

					if (numpos + len < dp && --width == 0)
						len += (dp == std::string::npos ? number.length() : dp) - (numpos + len);

					res += string_t(number.substr(numpos,len).c_str(),false);
					numpos += len;
				}
				break;

			case L'E':
			case L'e':
				{
					size_t len = 1;
					if (pos == sci)
					{
						if (strFormat[pos+1] == L'-' || strFormat[pos+1] == L'+')
							++pos;

						if (number[numpos+1] == '+' || number[numpos+1] == '-')
							++len;

						len += exp_digits;
						pos += exp_digits;
					}

					res += string_t(number.substr(numpos,len).c_str(),false);
				}
				break;

			default:
				if (strFormat.Compare(decimal,pos,decimal.Length()) == 0)
				{
					if (numpos < dp)
						res += string_t(number.substr(numpos,dp-numpos).c_str(),false);

					seen_decimal = true;
					sig_zero = true;
					res += decimal;
					pos += decimal.Length()-1;

					numpos = dp + decimal_char.length();
				}
				else if (strFormat.Compare(thousands,pos,thousands.Length()) == 0)
				{
					if (!group && !seen_decimal &&
							pos > 0 && pos < strFormat.Length()-1 &&
							(strFormat[pos-1]==L'#' || strFormat[pos-1]==L'0') &&
							(strFormat[pos+1]==L'#' || strFormat[pos+1]==L'0'))
					{
						group = true;
						pos += thousands.Length()-1;
					}
					else
						res += wc;
				}
				else
					res += wc;
				break;
			}
		}

		return res;
	}

	template <typename T>
	string_t fmt_recurse(T val, const string_t& strFormat, int def_precision, bool bRecurse)
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
				return fmt_decimal(val,precision);

			case scientific:
				return fmt_scientific(val,capital,precision);

			case fixed_point:
				return fmt_fixed(val,precision);

			case number:
				return fmt_number(val,precision);

			case hexadecimal:
				return fmt_hex(val,capital,precision);

			case round_trip:
				return fmt_round_trip(val,def_precision);

			case general:
			default:
				if (precision <= 0)
					precision = def_precision;

				return fmt_general(val,capital,precision);
			}
		}
		else if (bRecurse)
		{
			return fmt_custom(val,strFormat,def_precision);
		}
		else
		{
			return fmt_custom_i(val,strFormat);
		}
	}

	template <typename T>
	string_t fmt_custom(T val, const string_t& strFormat, int def_precision)
	{
		std::vector<string_t> parts;
		switch (parse_custom(strFormat,parts))
		{
		case 3:
			if (val == 0)
				return fmt_recurse(val,parts[2],def_precision,false);

		case 2:
			if (val < 0)
			{
				bool neg;
				return fmt_recurse(quick_abs(val,neg),parts[1],def_precision,false);
			}
			else
				return fmt_recurse(val,parts[0],def_precision,false);

		default:
			return fmt_custom_i(val,strFormat);
		}
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::Formatting::IFormattingException*,OOCore_IFormattingException_Create,1,((in),const Omega::string_t&,msg))
{
	ObjectImpl<FormattingException>* pNew = ObjectImpl<FormattingException>::CreateInstance();
	pNew->m_strDesc = msg;
	return static_cast<Formatting::IFormattingException*>(pNew);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(string_t,OOCore_to_string_int_t,3,((in),int64_t,val,(in),const string_t&,strFormat,(in),size_t,byte_width))
{
	int def_precision = 0;
	switch (byte_width)
	{
	case sizeof(byte_t):
		def_precision = 3;
		break;

	case sizeof(uint16_t):
		def_precision = 5;
		break;

	case sizeof(uint32_t):
		def_precision = 10;
		break;

	default:
	case sizeof(uint64_t):
		def_precision = 19;
		break;
	}

	return fmt_recurse(val,strFormat,def_precision,true);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(string_t,OOCore_to_string_uint_t,3,((in),uint64_t,val,(in),const string_t&,strFormat,(in),size_t,byte_width))
{
	int def_precision = 0;
	switch (byte_width)
	{
	case sizeof(byte_t):
		def_precision = 3;
		break;

	case sizeof(uint16_t):
		def_precision = 5;
		break;

	case sizeof(uint32_t):
		def_precision = 10;
		break;

	default:
	case sizeof(uint64_t):
		def_precision = 19;
		break;
	}

	return fmt_recurse(val,strFormat,def_precision,true);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(string_t,OOCore_to_string_float_t,3,((in),float8_t,val,(in),const string_t&,strFormat,(in),size_t,byte_width))
{
	int def_precision = 0;
	switch (byte_width)
	{
	case sizeof(float4_t):
		def_precision = 7;
		break;

	case sizeof(float8_t):
	default:
		def_precision = 15;
		break;
	}

	return fmt_recurse(val,strFormat,def_precision,true);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(string_t,OOCore_to_string_bool_t,2,((in),bool_t,val,(in),const string_t&,strFormat))
{
	// These need internationalisation...
	if (strFormat.IsEmpty())
		return (val ? string_t(L"true") : string_t(L"false"));

	std::vector<string_t> parts;
	if (parse_custom(strFormat,parts) != 2)
		throw Formatting::IFormattingException::Create(L"Invalid Omega::bool_t format string: {0}" % strFormat);

	return val ? parts[0] : parts[1];
}

int32_t OOCore::wcsto32(const wchar_t* sz, wchar_t const*& endptr, unsigned int base)
{
	static_assert(sizeof(::wcstol(0,0,0)) == sizeof(int32_t),"Non-standard wcstol");

	return ::wcstol(sz,const_cast<wchar_t**>(&endptr),base);
}

uint32_t OOCore::wcstou32(const wchar_t* sz, wchar_t const*& endptr, unsigned int base)
{
	static_assert(sizeof(::wcstoul(0,0,0)) == sizeof(uint32_t),"Non-standard wcstoul");

	return ::wcstoul(sz,const_cast<wchar_t**>(&endptr),base);
}

int64_t OOCore::wcsto64(const wchar_t* sz, wchar_t const*& endptr, unsigned int base)
{
#if defined(HAVE_WCSTOLL)
	static_assert(sizeof(::wcstoll(0,0,0)) == sizeof(int64_t),"Non-standard wcstoll");

	return wcstoll(sz,const_cast<wchar_t**>(&endptr),base);
#elif defined(HAVE__WCSTOI64)
	static_assert(sizeof(::_wcstoi64(0,0,0)) == sizeof(int64_t),"Non-standard _wcstoi64");

	return _wcstoi64(sz,const_cast<wchar_t**>(&endptr),base);
#else
#error Fix me!
#endif
}

uint64_t OOCore::wcstou64(const wchar_t* sz, wchar_t const*& endptr, unsigned int base)
{
#if defined(HAVE_WCSTOULL)
	static_assert(sizeof(::wcstoull(0,0,0)) == sizeof(int64_t),"Non-standard wcstoull");

	return wcstoull(sz,const_cast<wchar_t**>(&endptr),base);
#elif defined(HAVE__WCSTOUI64)
	static_assert(sizeof(::_wcstoui64(0,0,0)) == sizeof(int64_t),"Non-standard _wcstoui64");

	return _wcstoui64(sz,const_cast<wchar_t**>(&endptr),base);
#else
#error Fix me!
#endif
}

float8_t OOCore::wcstod(const wchar_t* sz, wchar_t const*& endptr)
{
	static_assert(sizeof(::wcstod(0,0)) == sizeof(float8_t),"Non-standard wcstod");

	return ::wcstod(sz,const_cast<wchar_t**>(&endptr));
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::int64_t,OOCore_wcsto64,3,((in),const Omega::string_t&,str,(out),size_t&,end_pos,(in),unsigned int,base))
{
	const wchar_t* start = str.c_str();
	const wchar_t* end = start;
	int64_t v = OOCore::wcsto64(start,end,base);

	end_pos = static_cast<size_t>(end - start);
	if (end_pos >= str.Length())
		end_pos = string_t::npos;

	return v;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::uint64_t,OOCore_wcstou64,3,((in),const Omega::string_t&,str,(out),size_t&,end_pos,(in),unsigned int,base))
{
	const wchar_t* start = str.c_str();
	const wchar_t* end = start;
	uint64_t v = OOCore::wcstou64(start,end,base);

	end_pos = static_cast<size_t>(end - start);
	if (end_pos >= str.Length())
		end_pos = string_t::npos;

	return v;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::float8_t,OOCore_wcstod,2,((in),const Omega::string_t&,str,(out),size_t&,end_pos))
{
	const wchar_t* start = str.c_str();
	const wchar_t* end = start;
	float8_t v = OOCore::wcstod(start,end);

	end_pos = static_cast<size_t>(end - start);
	if (end_pos >= str.Length())
		end_pos = string_t::npos;

	return v;
}
