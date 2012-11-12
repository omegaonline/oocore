///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009, 2011 Rick Taylor
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

#include <limits.h>
#include <ctype.h>
#include <locale.h>

#if defined(HAVE_INTTYPES_H)
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#endif

#if !defined(PRId64)
#if defined(__MINGW32__)
#define PRId64 "I64d"
#define PRIu64 "I64u"
#define PRIX64 "I64X"
#define PRIx64 "I64x"
#else
#define PRId64 "lld"
#define PRIu64 "llu"
#define PRIX64 "llX"
#define PRIx64 "llx"
#endif
#endif

#if defined(_WIN32)
#define EXTRA_LCID const lconv* lc,LCID lcid
#define PASS_LCID lc,lcid
#else
#define EXTRA_LCID const lconv* lc
#define PASS_LCID lc
#endif

using namespace Omega;
using namespace OTL;

namespace
{
	enum num_fmt
	{
		unspecified,
		currency,
		decimal,
		scientific,
		fixed_point,
		general,
		number,
		round_trip,
		hexadecimal
	};

	bool priv_isdigit(char c)
	{
		return (c >= '0' && c <= '9');
	}

	bool parse_numfmt(const string_t& str, num_fmt& fmt, bool& capital, int& precision)
	{
		if (str.Length() > 3)
			return false;

		fmt = unspecified;
		capital = true;
		precision = -1;

		if (str.Length() >= 1)
		{
			switch (str[0])
			{
			case 'c':
				capital = false;
				// Intentional fall-through
			case 'C':
				fmt = currency;
				break;

			case 'd':
				capital = false;
				// Intentional fall-through
			case 'D':
				fmt = decimal;
				break;

			case 'e':
				capital = false;
				// Intentional fall-through
			case 'E':
				fmt = scientific;
				break;

			case 'f':
				capital = false;
				// Intentional fall-through
			case 'F':
				fmt = fixed_point;
				break;

			case 'g':
				capital = false;
				// Intentional fall-through
			case 'G':
				fmt = general;
				break;

			case 'n':
				capital = false;
				// Intentional fall-through
			case 'N':
				fmt = number;
				break;

			case 'r':
				capital = false;
				// Intentional fall-through
			case 'R':
				fmt = round_trip;
				break;

			case 'x':
				capital = false;
				// Intentional fall-through
			case 'X':
				fmt = hexadecimal;
				break;

			default:
				return false;
			}
		}

		if (str.Length() >= 2)
		{
			if (priv_isdigit(str[1]))
			{
				const char* endp = 0;
				precision = OOCore::strtol(str.c_str()+1,endp,10);
				if (*endp != '\0')
					return false;
			}
		}

		return true;
	}

	size_t replace(OOBase::LocalString& str, const char* orig, const char* repl)
	{
		size_t pos = str.find(orig);
		if (pos != str.npos && strcmp(orig,repl) != 0)
		{
			OOBase::LocalString str2;
			int err = str2.assign(str.c_str(),pos);
			if (err == 0)
			{
				err = str2.append(repl);
				if (err == 0)
				{
					err = str2.append(str.c_str()+pos+strlen(orig));
					if (err == 0)
						err = str.assign(str2);
				}
			}
			if (err != 0)
				OMEGA_THROW(err);
		}

		return pos;
	}

	void insert(OOBase::LocalString& str, size_t pos, const char* sz)
	{
		OOBase::LocalString str2;
		int err = str2.assign(str.c_str(),pos);
		if (err == 0)
		{
			err = str2.append(sz);
			if (err == 0)
			{
				err = str2.append(str.c_str()+pos);
				if (err == 0)
					err = str.assign(str2);
			}
		}
		if (err != 0)
			OMEGA_THROW(err);
	}

	void insert(OOBase::LocalString& str, size_t pos, size_t count, char c)
	{
		OOBase::LocalString str2;
		int err = str2.assign(str.c_str(),pos);

		while (count-- > 0 && err == 0)
			err = str2.append(&c,1);

		if (err == 0)
		{
			err = str2.append(str.c_str()+pos);
			if (err == 0)
				err = str.assign(str2);
		}

		if (err != 0)
			OMEGA_THROW(err);
	}

	void erase(OOBase::LocalString& str, size_t start, size_t len)
	{
		OOBase::LocalString str2;
		int err = str2.assign(str.c_str(),start);
		if (err == 0)
		{
			err = str2.append(str.c_str()+start+len);
			if (err == 0)
				err = str.assign(str2);
		}

		if (err != 0)
			OMEGA_THROW(err);
	}

	string_t do_intl_mon(OOBase::LocalString& str, bool negative, EXTRA_LCID)
	{
		const char* src_decimal_point = ".";
		if (lc)
			src_decimal_point = lc->decimal_point;

#if defined(_WIN32)
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
		const char* decimal_point = src_decimal_point;
		const char* grouping = "\x03\x00";
		const char* thousands_sep = ",";

		if (lc)
		{
			if (*lc->mon_decimal_point == '\0')
				decimal_point = lc->mon_decimal_point;

			grouping = lc->mon_grouping;
			thousands_sep = lc->mon_thousands_sep;
		}
#endif

		size_t pos = replace(str,src_decimal_point,decimal_point);
		if (pos == str.npos)
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
			insert(str,pos,thousands_sep);
		}

		bool cs_precedes = false;
		bool sep_by_space = false;
		int posn = 0;

#if defined(_WIN32)
		char sign[6];

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
			posn = static_cast<long>(val);

			if (!GetLocaleInfoA(lcid,LOCALE_SNEGATIVESIGN,sign,sizeof(sign)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}
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
			posn = static_cast<long>(val);

			if (!GetLocaleInfoA(lcid,LOCALE_SPOSITIVESIGN,sign,sizeof(sign)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}
		}

		char currency[8];
		if (!GetLocaleInfoA(lcid,LOCALE_SCURRENCY,currency,sizeof(currency)))
		{
			DWORD dwErr = GetLastError();
			OMEGA_THROW(dwErr);
		}

#else
		const char* sign;
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
		const char* currency = lc->currency_symbol;
#endif

		string_t ret;
		switch (posn)
		{
		case 0:
			ret = currency;
			if (cs_precedes)
			{
				if (sep_by_space)
					ret += ' ';
				ret += str.c_str();
			}
			else
			{
				if (sep_by_space)
					ret = ' ' + ret;
				ret = str.c_str() + ret;
			}
			ret = '(' + ret + ')';
			break;

		case 1:
			ret = currency;
			if (cs_precedes)
			{
				if (sep_by_space)
					ret += ' ';
				ret += str.c_str();
			}
			else
			{
				if (sep_by_space)
					ret = ' ' + ret;
				ret = str.c_str() + ret;
			}
			ret = sign + ret;
			break;

		case 2:
			ret = currency;
			if (cs_precedes)
			{
				if (sep_by_space)
					ret += ' ';
				ret += str.c_str();
			}
			else
			{
				if (sep_by_space)
					ret = ' ' + ret;
				ret = str.c_str() + ret;
			}
			ret += sign;
			break;

		case 3:
			ret = string_t(sign) + currency;
			if (cs_precedes)
			{
				if (sep_by_space)
					ret += ' ';
				ret += str.c_str();
			}
			else
			{
				if (sep_by_space)
					ret = ' ' + ret;
				ret = str.c_str() + ret;
			}
			break;

		case 4:
		default:
			ret = string_t(currency) + sign;
			if (cs_precedes)
			{
				if (sep_by_space)
					ret += ' ';
				ret += str.c_str();
			}
			else
			{
				if (sep_by_space)
					ret = ' ' + ret;
				ret = str.c_str() + ret;
			}
			break;
		}

		return ret;
	}

	size_t do_intl(OOBase::LocalString& str, bool bThou, EXTRA_LCID)
	{
#if defined(_WIN32)
		const char* src_decimal_point = ".";
		if (lc)
			src_decimal_point = lc->decimal_point;

		char decimal_point[5] = {0};
		if (!GetLocaleInfoA(lcid,LOCALE_SDECIMAL,decimal_point,sizeof(decimal_point)))
		{
			DWORD dwErr = GetLastError();
			OMEGA_THROW(dwErr);
		}

		size_t dp = replace(str,src_decimal_point,decimal_point);

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

		if (lc)
		{
			decimal_point = lc->decimal_point;
			grouping = lc->grouping;
			thousands_sep = lc->thousands_sep;
		}

		size_t dp = str.find(decimal_point);

#endif
		if (bThou)
		{
			size_t pos = dp;
			if (pos == str.npos)
				pos = str.length();

			for (int grp = CHAR_MAX;;)
			{
				char n_grp = *grouping;
				if (n_grp != '\0')
				{
					grp = n_grp;
					++grouping;
				}

				if (grp == CHAR_MAX)
					break;

				if (pos <= static_cast<size_t>(grp))
					break;

				pos -= grp;
				insert(str,pos,thousands_sep);
			}
		}

		return dp;
	}

	void undo_intl(OOBase::LocalString& str, EXTRA_LCID)
	{
		if (lc)
			replace(str,lc->decimal_point,".");

#if defined(_WIN32)
		char decimal_point[5] = {0};
		if (!GetLocaleInfoA(lcid,LOCALE_SDECIMAL,decimal_point,sizeof(decimal_point)))
		{
			DWORD dwErr = GetLastError();
			OMEGA_THROW(dwErr);
		}

		replace(str,decimal_point,".");
#endif
	}

	void fmt_fixed_i(OOBase::LocalString& str, const int64_t& val, int precision)
	{
		int err = str.printf("%" PRId64,val);
		if (err != 0)
			OMEGA_THROW(err);

		if (precision > 0)
		{
			const char* decimal_point = ".";
			const lconv* lc = localeconv();
			if (lc)
				decimal_point = lc->decimal_point;

			err = str.append(decimal_point);
			if (err != 0)
				OMEGA_THROW(err);

			while (precision-- > 0)
			{
				err = str.append("0");
				if (err != 0)
					OMEGA_THROW(err);
			}
		}
	}

	void fmt_fixed_i(OOBase::LocalString& str, System::Internal::optimal_param<uint64_t>::type val, int precision)
	{
		int err = str.printf("%" PRIu64,val);
		if (err != 0)
			OMEGA_THROW(err);

		if (precision > 0)
		{
			const char* decimal_point = ".";
			const lconv* lc = localeconv();
			if (lc)
				decimal_point = lc->decimal_point;

			err = str.append(decimal_point);
			if (err != 0)
				OMEGA_THROW(err);

			while (precision-- > 0)
			{
				err = str.append("0");
				if (err != 0)
					OMEGA_THROW(err);
			}
		}
	}

	void fmt_fixed_i(OOBase::LocalString& str, System::Internal::optimal_param<double>::type val, int precision)
	{
		int err;
		if (precision >= 0)
			err = str.printf("%.*f",precision,val);
		else
			err = str.printf("%f",val);

		if (err != 0)
			OMEGA_THROW(err);
	}

	template <typename T>
	string_t fmt_currency(const T& val, int precision, EXTRA_LCID)
	{
		if (precision < 0)
		{
#if defined(_WIN32)
			DWORD v = 0;
			if (!GetLocaleInfoA(lcid,LOCALE_IDIGITS | LOCALE_RETURN_NUMBER,(LPSTR)&v,sizeof(v)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}

			precision = static_cast<long>(v);
#else
			precision = 2;
			if (lc && lc->frac_digits != CHAR_MAX)
				precision = lc->frac_digits;
#endif
		}

		OOBase::LocalString ret;
		fmt_fixed_i(ret,val,precision);
		if (!ret.empty() && (ret[0]=='-' || ret[0]=='+'))
			erase(ret,0,1);

		return do_intl_mon(ret,val < 0.0,PASS_LCID);
	}

	template <typename T>
	string_t fmt_number(const T& val, int precision, EXTRA_LCID)
	{
		if (precision < 0)
		{
#if defined(_WIN32)
			DWORD v = 0;
			if (!GetLocaleInfoA(lcid,LOCALE_IDIGITS | LOCALE_RETURN_NUMBER,(LPSTR)&v,sizeof(v)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}

			precision = static_cast<long>(v);
#else
			precision = 2;
			if (lc && lc->frac_digits != CHAR_MAX)
				precision = lc->frac_digits;
#endif
		}

		OOBase::LocalString ret;
		fmt_fixed_i(ret,val,precision);

		do_intl(ret,true,PASS_LCID);

		return ret.c_str();
	}

	void fmt_decimal_i(OOBase::LocalString& str, const int64_t& val, int precision)
	{
		int err;
		if (precision >= 0)
			err = str.printf("%.*" PRId64,precision,val);
		else
			err = str.printf("%" PRId64,val);

		if (err != 0)
			OMEGA_THROW(err);
	}

	void fmt_decimal_i(OOBase::LocalString& str, System::Internal::optimal_param<uint64_t>::type val, int precision)
	{
		int err;
		if (precision >= 0)
			err = str.printf("%.*" PRIu64,precision,val);
		else
			err = str.printf("%" PRIu64,val);

		if (err != 0)
			OMEGA_THROW(err);
	}

	void fmt_decimal_i(OOBase::LocalString& str, System::Internal::optimal_param<double>::type val, int precision)
	{
		fmt_decimal_i(str,static_cast<int64_t>(val),precision);
	}

	template <typename T>
	string_t fmt_decimal(const T& val, int precision)
	{
		OOBase::LocalString str;
		fmt_decimal_i(str,val,precision);
		return str.c_str();
	}

	void fmt_hex_i(OOBase::LocalString& str, System::Internal::optimal_param<uint64_t>::type val, bool capital, int precision)
	{
		int err;
		if (precision >= 0)
			err = str.printf((capital ? "%.*" PRIX64 : "%.*" PRIx64),precision,val);
		else
			err = str.printf((capital ? "%" PRIX64 : "%" PRIx64),val);

		if (err != 0)
			OMEGA_THROW(err);
	}

	void fmt_hex_i(OOBase::LocalString& str, const int64_t& val, bool capital, int precision)
	{
		fmt_hex_i(str,static_cast<uint64_t>(val),capital,precision);
	}

	void fmt_hex_i(OOBase::LocalString& str, System::Internal::optimal_param<double>::type val, bool capital, int precision)
	{
		fmt_hex_i(str,static_cast<uint64_t>(val),capital,precision);
	}

	template <typename T>
	string_t fmt_hex(const T& val, bool capital, int precision)
	{
		OOBase::LocalString str;
		fmt_hex_i(str,val,capital,precision);
		return str.c_str();
	}

	template <typename T>
	string_t fmt_fixed(const T& val, int precision, EXTRA_LCID)
	{
		if (precision < 0)
		{
#if defined(_WIN32)
			DWORD v = 0;
			if (!GetLocaleInfoA(lcid,LOCALE_IDIGITS | LOCALE_RETURN_NUMBER,(LPSTR)&v,sizeof(v)))
			{
				DWORD dwErr = GetLastError();
				OMEGA_THROW(dwErr);
			}

			precision = static_cast<long>(v);
#else
			precision = 2;
			if (lc && lc->frac_digits != CHAR_MAX)
				precision = lc->frac_digits;
#endif
		}

		OOBase::LocalString ret;
		fmt_fixed_i(ret,val,precision);

		do_intl(ret,false,PASS_LCID);

		return ret.c_str();
	}

	void exp_strip(OOBase::LocalString& str, int precision, bool show_plus)
	{
		size_t pos = strcspn(str.c_str(),"Ee");
		if (pos >= str.length()-1)
			return;

		// Remove + if !exp_plus
		++pos;
		if (str[pos] == '+')
		{
			if (!show_plus)
				erase(str,pos,1);
			else
				++pos;
		}
		else if (str[pos] == '-')
			++pos;

		// Remove leading zeros if > precision
		while (pos < str.length()-precision && str[pos] == '0')
			erase(str,pos,1);

		// Insert any extra zero's required
		if (str.length() < pos+precision)
			insert(str,pos,pos+precision-str.length(),'0');
	}

	void fmt_scientific_i(OOBase::LocalString& str, System::Internal::optimal_param<double>::type val, bool capital, int precision, EXTRA_LCID)
	{
		int err;
		if (precision >= 0)
			err = str.printf((capital ? "%.*E" : "%.*e"),precision,val);
		else
			err = str.printf((capital ? "%E" : "%e"),val);

		if (err != 0)
			OMEGA_THROW(err);

		do_intl(str,false,PASS_LCID);
		exp_strip(str,3,true);
	}

	template <typename T>
	void fmt_scientific_i(OOBase::LocalString& str, const T& val, bool capital, int precision, EXTRA_LCID)
	{
		fmt_scientific_i(str,static_cast<double>(val),capital,precision,PASS_LCID);
	}

	template <typename T>
	string_t fmt_scientific(const T& val, bool capital, int precision, EXTRA_LCID)
	{
		if (precision < 0)
			precision = 6;

		OOBase::LocalString str;
		fmt_scientific_i(str,val,capital,precision,PASS_LCID);
		return str.c_str();
	}

	void fmt_general_i(OOBase::LocalString& str, const int64_t& val, bool /*capital*/, int precision)
	{
		int err;
		if (precision >= 0)
			err = str.printf("%.*" PRId64,precision,val);
		else
			err = str.printf("%" PRId64,val);

		if (err != 0)
			OMEGA_THROW(err);
	}

	void fmt_general_i(OOBase::LocalString& str, System::Internal::optimal_param<uint64_t>::type val, bool /*capital*/, int precision)
	{
		int err;
		if (precision >= 0)
			err = str.printf("%0*" PRIu64,precision,val);
		else
			err = str.printf("%" PRIu64,val);

		if (err != 0)
			OMEGA_THROW(err);
	}

	void fmt_general_i(OOBase::LocalString& str, System::Internal::optimal_param<double>::type val, bool capital, int precision)
	{
		int err;
		if (precision >= 0)
			err = str.printf((capital ? "%.*G" : "%.*g"),precision,val);
		else
			err = str.printf((capital ? "%G" : "%g"),val);

		if (err != 0)
			OMEGA_THROW(err);

		exp_strip(str,2,true);
	}

	template <typename T>
	string_t fmt_general(const T& val, bool capital, int precision, bool use_locale, EXTRA_LCID)
	{
		OOBase::LocalString str;
		fmt_general_i(str,val,capital,precision);

		if (use_locale)
			do_intl(str,false,PASS_LCID);
		else
			undo_intl(str,PASS_LCID);

		return string_t(str.c_str());
	}

	template <typename T>
	string_t fmt_round_trip(const T& val, int /*precision*/, EXTRA_LCID)
	{
		OMEGA_UNUSED_ARG(lc);
#if defined(_WIN32)
		OMEGA_UNUSED_ARG(lcid);
#endif
		return fmt_decimal(val,0);
	}

	string_t fmt_round_trip(System::Internal::optimal_param<double>::type val, int precision, EXTRA_LCID)
	{
		string_t s;
		for (int i=0; i<2; i+=2)
		{
			s = fmt_general(val,false,precision+i,false,PASS_LCID);

			const char* end = NULL;
			if (val == OOCore::strtod(s.c_str(),end))
				break;
		}

		return s;
	}

	size_t find_skip_quote(const string_t& str, size_t start, string_t strFind)
	{
		for (size_t pos = start;;)
		{
			size_t found = str.FindOneOf(strFind + string_t::constant("'\""),pos);
			if (found == string_t::npos)
				return found;

			if (str[found] == '\'')
			{
				if (found==0 || str[found-1] != '\\')
				{
					for (;;)
					{
						found = str.Find('\'',found+1);
						if (found == string_t::npos)
							return found;

						if (str[found-1] != '\\')
							break;
					}
				}

				pos = found+1;
			}
			else if (str[found] == '"')
			{
				if (found==0 || str[found-1] != '\\')
				{
					for (;;)
					{
						found = str.Find('"',found+1);
						if (found == string_t::npos)
							return found;

						if (str[found-1] != '\\')
							break;
					}
				}

				pos = found+1;
			}
			else
				return found;
		}
	}

	size_t parse_custom(const string_t& str, OOBase::Stack<string_t,OOBase::LocalAllocator>& parts)
	{
		int err = 0;
		size_t pos = find_skip_quote(str,0,string_t::constant(";"));
		if (pos == string_t::npos)
			err = parts.push(str);
		else
		{
			err = parts.push(str.Left(pos++));
			if (err == 0)
			{
				size_t pos2 = find_skip_quote(str,pos,string_t::constant(";"));
				if (pos2 == string_t::npos)
					err = parts.push(str.Mid(pos));
				else
				{
					err = parts.push(str.Mid(pos,pos2-pos));
					if (err == 0)
						err = parts.push(str.Mid(pos2+1));
				}
			}
		}
		if (err != 0)
			OMEGA_THROW(err);

		return parts.size();
	}

	template <typename T>
	string_t fmt_custom(const T& val, const string_t& strFormat, int def_precision, EXTRA_LCID);

	template <typename T>
	string_t fmt_recurse(const T& val, const string_t& strFormat, int def_precision, bool bRecurse, EXTRA_LCID);

	template <typename T>
	string_t fmt_rnd_away(const T& val)
	{
		return fmt_decimal(val,0);
	}

	string_t fmt_rnd_away(System::Internal::optimal_param<double>::type val)
	{
		double res = ceil(fabs(val));
		return fmt_decimal(res < 0.0 ? -res : res,0);
	}

	template <typename T>
	T quick_abs(const T& val, bool& negative)
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
	string_t fmt_custom_i(const T& val, const string_t& strFormat, EXTRA_LCID)
	{
		if (strFormat == "##" || strFormat == "00")
			return fmt_rnd_away(val);

		bool negative;
		T abs_val = quick_abs(val,negative);

		// Work out precision and mode...
		size_t sci = string_t::npos;
		int precision = 0;
		int width = 0;
		int exp_digits = 0;
		bool seen_decimal = false;
		bool group = false;

		for (size_t pos = 0; pos < strFormat.Length();)
		{
			size_t found = find_skip_quote(strFormat,pos,string_t::constant("0#Ee.,\\"));
			if (found == string_t::npos)
				break;

			switch (strFormat[found])
			{
			case '\\':
				++found;
				break;

			case 'E':
			case 'e':
				if (found < strFormat.Length()-1)
				{
					if (strFormat[found+1] == '-' || strFormat[found+1] == '+')
					{
						if (found < strFormat.Length()-2 && strFormat[found+2] == '0')
						{
							sci = found;
							found += 2;
						}
					}
					else if (strFormat[found+1] == '0')
					{
						sci = found;
						++found;
					}

					if (sci != string_t::npos)
					{
						for (; strFormat[found] == '0' && found<strFormat.Length(); ++found)
							++exp_digits;

						break;
					}
				}
				break;

			case '#':
			case '0':
				if (seen_decimal)
					++precision;
				else
					++width;
				break;

			case '.':
				seen_decimal = true;
				break;

			case ',':
				group = true;
				break;

			default:
				break;
			}

			pos = found + 1;
		}

		// Create the base number string
		OOBase::LocalString strNumber;
		if (sci != string_t::npos)
		{
			fmt_scientific_i(strNumber,abs_val,strFormat[sci] == 'E',precision,PASS_LCID);
			exp_strip(strNumber,exp_digits,strFormat[sci+1] == '+');
		}
		else if (seen_decimal)
			fmt_fixed_i(strNumber,abs_val,precision);
		else
			fmt_decimal_i(strNumber,abs_val,width);

#if defined(_WIN32)
		char decimal_char[5] = {0};
		if (!GetLocaleInfoA(lcid,LOCALE_SDECIMAL,decimal_char,5))
		{
			DWORD dwErr = GetLastError();
			OMEGA_THROW(dwErr);
		}
#else
		const char* decimal_char = ".";
		if (lc)
			decimal_char = lc->decimal_point;
#endif
		size_t dp = strNumber.find(decimal_char);

		if (sci != string_t::npos || dp != strNumber.npos)
		{
			// Add extra leading zero's
			if (dp == strNumber.npos)
			{
				dp = strNumber.length();
				int err = strNumber.append(decimal_char);
				if (err != 0)
					OMEGA_THROW(err);
			}

			if (static_cast<size_t>(width) > dp)
				insert(strNumber,0,width-dp,'0');
		}

		if (negative)
			insert(strNumber,0,"-");

		// Translate to intl format
		dp = do_intl(strNumber,group,PASS_LCID);

		// Transpose strNumber into format
		string_t res;
		bool sig_zero = false;
		size_t numpos = 0;
		seen_decimal = false;

		for (size_t pos = 0; pos < strFormat.Length(); ++pos)
		{
			char c = strFormat[pos];
			switch (c)
			{
			case '\\':
				res += strFormat[++pos];
				break;

			case '\'':
				{
					size_t start = pos + 1;
					for (;;)
					{
						pos = strFormat.Find('\'',pos+1);
						if (pos == string_t::npos)
							break;

						if (strFormat[pos-1] != '\\')
							break;
					}

					res += strFormat.Mid(start,pos);
				}
				break;

			case '"':
				{
					size_t start = pos + 1;
					for (;;)
					{
						pos = strFormat.Find('"',pos+1);
						if (pos == string_t::npos)
							break;

						if (strFormat[pos-1] != '\\')
							break;
					}

					res += strFormat.Mid(start,pos);
				}
				break;

			case '#':
				while (!priv_isdigit(strNumber[numpos]))
				{
					if (sig_zero)
						res += string_t(strNumber.c_str()+numpos,1);

					++numpos;
				}

				if (sig_zero || strNumber[numpos] != '0')
				{
					res += string_t(strNumber.c_str()+numpos,1);
					sig_zero = !seen_decimal;
				}
				++numpos;
				break;

			case '0':
				while (!priv_isdigit(strNumber[numpos]))
				{
					res += string_t(strNumber.c_str()+numpos,1);
					++numpos;
				}

				res += string_t(strNumber.c_str()+numpos,1);
				++numpos;
				sig_zero = !seen_decimal;
				break;

			case 'E':
			case 'e':
				if (sci != strNumber.npos)
				{
					size_t len = 1;
					if (strFormat[pos+1] == '-' || strFormat[pos+1] == '+')
						++pos;

					if (strNumber[numpos+1] == '-' || strFormat[numpos+1] == '+')
						++len;

					len += exp_digits;
					pos += exp_digits;

					res += string_t(strNumber.c_str()+numpos,len);
					numpos += len;

					sci = strNumber.npos;
				}
				else
					res += c;
				break;

			case '.':
				if (!seen_decimal)
				{
					if (numpos < dp)
						res += string_t(strNumber.c_str()+numpos,dp-numpos);

					res += decimal_char;
					numpos = dp + strlen(decimal_char);

					sig_zero = true;
					seen_decimal = true;
				}
				else
					res += c;
				break;

			case ',':
				if (!seen_decimal &&
						pos > 0 && pos < strFormat.Length()-1 &&
						(strFormat[pos-1]=='#' || strFormat[pos-1]=='0') &&
						(strFormat[pos+1]=='#' || strFormat[pos+1]=='0'))
				{
					// Already added
				}
				else
					res += c;
				break;

			default:
				res += c;
				break;
			}
		}

		if (numpos < strNumber.length())
			res += strNumber.c_str()+numpos;

		return res;
	}

	template <typename T>
	string_t fmt_recurse(const T& val, const string_t& strFormat, int def_precision, bool bRecurse, EXTRA_LCID)
	{
		num_fmt fmt;
		bool capital;
		int precision;
		if (parse_numfmt(strFormat,fmt,capital,precision))
		{
			switch (fmt)
			{
			case currency:
				return fmt_currency(val,precision,PASS_LCID);

			case decimal:
				return fmt_decimal(val,precision);

			case scientific:
				return fmt_scientific(val,capital,precision,PASS_LCID);

			case fixed_point:
				return fmt_fixed(val,precision,PASS_LCID);

			case number:
				return fmt_number(val,precision,PASS_LCID);

			case hexadecimal:
				return fmt_hex(val,capital,precision);

			case round_trip:
				return fmt_round_trip(val,def_precision,PASS_LCID);

			case general:
				if (precision <= 0)
					precision = def_precision;

				return fmt_general(val,capital,precision,true,PASS_LCID);

			default:
				return fmt_general(val,capital,-1,true,PASS_LCID);
			}
		}
		else if (bRecurse)
		{
			return fmt_custom(val,strFormat,def_precision,PASS_LCID);
		}
		else
		{
			return fmt_custom_i(val,strFormat,PASS_LCID);
		}
	}

	template <typename T>
	string_t fmt_custom(const T& val, const string_t& strFormat, int def_precision, EXTRA_LCID)
	{
		OOBase::Stack<string_t,OOBase::LocalAllocator> parts;
		switch (parse_custom(strFormat,parts))
		{
		case 3:
			if (val == 0)
				return fmt_recurse(val,*parts.at(2),def_precision,false,PASS_LCID);

			// Intentional fall-through

		case 2:
#if defined(__clang__)
			{
 				bool neg;
				return fmt_recurse(quick_abs(val,neg),neg ? *parts.at(1) : *parts.at(0),def_precision,false,PASS_LCID);
 			}
#else
			if (val < 0)
 			{
 				bool neg;
				return fmt_recurse(quick_abs(val,neg),*parts.at(1),def_precision,false,PASS_LCID);
 			}
			else
				return fmt_recurse(val,*parts.at(0),def_precision,false,PASS_LCID);
#endif

		default:
			return fmt_custom_i(val,strFormat,PASS_LCID);
		}
	}
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

#if defined(_WIN32)
	LCID lcid = GetThreadLocale();
#endif
	const lconv* lc = localeconv();

	return fmt_recurse(val,strFormat,def_precision,true,PASS_LCID);
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

#if defined(_WIN32)
	LCID lcid = GetThreadLocale();
#endif
	const lconv* lc = localeconv();

	return fmt_recurse(val,strFormat,def_precision,true,PASS_LCID);
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

#if defined(_WIN32)
	LCID lcid = GetThreadLocale();
#endif
	const lconv* lc = localeconv();

	return fmt_recurse(val,strFormat,def_precision,true,PASS_LCID);
}

OMEGA_DEFINE_EXPORTED_FUNCTION(string_t,OOCore_to_string_bool_t,2,((in),bool_t,val,(in),const string_t&,strFormat))
{
	// These need internationalisation...
	if (!strFormat.IsEmpty())
	{
		OOBase::Stack<string_t,OOBase::LocalAllocator> parts;
		if (parse_custom(strFormat,parts) == 2)
			return val ? *parts.at(0) : *parts.at(1);
	}

	return (val ? string_t::constant("true") : string_t::constant("false"));
}

long OOCore::strtol(const char* sz, char const*& endptr, unsigned int base)
{
	return ::strtol(sz,const_cast<char**>(&endptr),base);
}

unsigned long OOCore::strtoul(const char* sz, char const*& endptr, unsigned int base)
{
	return ::strtoul(sz,const_cast<char**>(&endptr),base);
}

int64_t OOCore::strto64(const char* sz, char const*& endptr, unsigned int base)
{
#if defined(HAVE_STRTOLL)
	static_assert(sizeof(::strtoll(sz,const_cast<char**>(&endptr),base)),"Non-standard strtoll");

	return ::strtoll(sz,const_cast<char**>(&endptr),base);
#elif defined(HAVE__STRTOI64)
	static_assert(sizeof(::_strtoi64(sz,const_cast<char**>(&endptr),base)) == sizeof(int64_t),"Non-standard _strtoi64");

	return ::_strtoi64(sz,const_cast<char**>(&endptr),base);
#else
	static_assert(sizeof(::strtol(sz,const_cast<char**>(&endptr),base)) == sizeof(int64_t),"long is too short!");

	return ::strtol(sz,const_cast<char**>(&endptr),base);
#endif
}

uint64_t OOCore::strtou64(const char* sz, char const*& endptr, unsigned int base)
{
#if defined(HAVE_STRTOULL)
	static_assert(sizeof(::strtoull(sz,const_cast<char**>(&endptr),base)) == sizeof(int64_t),"Non-standard strtoull");

	return ::strtoull(sz,const_cast<char**>(&endptr),base);
#elif defined(HAVE__STRTOUI64)
	static_assert(sizeof(::_strtoui64(sz,const_cast<char**>(&endptr),base)) == sizeof(int64_t),"Non-standard _strtoui64");

	return ::_strtoui64(sz,const_cast<char**>(&endptr),base);
#else
	static_assert(sizeof(::strtoul(sz,const_cast<char**>(&endptr),base)) == sizeof(int64_t),"long is too short!");

	return ::strtoul(sz,const_cast<char**>(&endptr),base);
#endif
}

float8_t OOCore::strtod(const char* sz, char const*& endptr)
{
	static_assert(sizeof(::strtod(sz,const_cast<char**>(&endptr))) == sizeof(float8_t),"Non-standard strtod");

	return ::strtod(sz,const_cast<char**>(&endptr));
}

OMEGA_DEFINE_EXPORTED_FUNCTION(int64_t,OOCore_strto64,3,((in),const string_t&,str,(out),size_t&,end_pos,(in),unsigned int,base))
{
	const char* start = str.c_str();
	const char* end = start;
	int64_t v = OOCore::strto64(start,end,base);

	end_pos = static_cast<size_t>(end - start);
	if (end_pos >= str.Length())
		end_pos = string_t::npos;

	return v;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(uint64_t,OOCore_strtou64,3,((in),const string_t&,str,(out),size_t&,end_pos,(in),unsigned int,base))
{
	const char* start = str.c_str();
	const char* end = start;
	uint64_t v = OOCore::strtou64(start,end,base);

	end_pos = static_cast<size_t>(end - start);
	if (end_pos >= str.Length())
		end_pos = string_t::npos;

	return v;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(float8_t,OOCore_strtod,2,((in),const string_t&,str,(out),size_t&,end_pos))
{
	const char* start = str.c_str();
	const char* end = start;
	float8_t v = OOCore::strtod(start,end);

	end_pos = static_cast<size_t>(end - start);
	if (end_pos >= str.Length())
		end_pos = string_t::npos;

	return v;
}

namespace
{
	struct insert_t
	{
		unsigned long  index;
		long            alignment;
		string_t        strFormat;
		string_t        strSuffix;
	};

	struct format_state_t
	{
		OOBase::Bag<insert_t> m_inserts;
		string_t              m_strPrefix;
	};

	size_t find_brace(const string_t& strIn, size_t start, char brace)
	{
		for (;;)
		{
			size_t found = strIn.Find(brace,start);
			if (found == string_t::npos)
				return string_t::npos;

			if (found == strIn.Length()-1 || strIn[found+1] != brace)
				return found;

			// Skip {{
			start = found + 2;
		}
	}

	void merge_braces(string_t& str)
	{
		for (size_t pos = 0;;)
		{
			pos = str.Find('{',pos);
			if (pos == string_t::npos)
				break;

			str = str.Left(pos) + str.Mid(pos+1);
		}

		for (size_t pos = 0;;)
		{
			pos = str.Find('}',pos);
			if (pos == string_t::npos)
				break;

			str = str.Left(pos) + str.Mid(pos+1);
		}
	}

	bool parse_arg(const string_t& strIn, size_t& pos, insert_t& ins)
	{
		size_t end = find_brace(strIn,pos,'}');
		if (end == string_t::npos)
			return false;

		size_t comma = strIn.Find(',',pos);
		size_t colon = strIn.Find(':',pos);
		if (comma == pos || colon == pos)
		{
			pos = end + 1;
			return false;
		}

		const char* endp = 0;
		ins.index = OOCore::strtoul(strIn.c_str()+pos,endp,10);

		ins.alignment = 0;
		if (comma < end && comma < colon)
		{
			pos = comma++;
			ins.alignment = OOCore::strtol(strIn.c_str()+comma,endp,10);
		}

		if (colon < end)
		{
			ins.strFormat = strIn.Mid(colon+1,end-colon-1);
			merge_braces(ins.strFormat);
		}

		pos = end + 1;
		return true;
	}

	void parse_format(const string_t& strIn, string_t& strPrefix, OOBase::Bag<insert_t>& inserts)
	{
		// Prefix first
		size_t pos = find_brace(strIn,0,'{');
		if (pos == string_t::npos)
			return;

		strPrefix = strIn.Left(pos++);
		merge_braces(strPrefix);

		// Parse args
		for (;;)
		{
			insert_t ins;
			size_t found = string_t::npos;

			if (!parse_arg(strIn,pos,ins))
				ins.strSuffix = strIn.Mid(pos);
			else
			{
				found = find_brace(strIn,pos,'{');
				if (found == string_t::npos)
					ins.strSuffix = strIn.Mid(pos);
				else
					ins.strSuffix = strIn.Mid(pos,found-pos);
			}

			merge_braces(ins.strSuffix);

			int err = inserts.add(ins);
			if (err != 0)
				OMEGA_THROW(err);

			if (found == string_t::npos)
				break;

			pos = found + 1;
		}
	}

	string_t align(const string_t& str, long align)
	{
		unsigned long width = (align < 0 ? -align : align);
		if (str.Length() >= width)
			return str;

		string_t strFill;
		for (unsigned long i=0;i<width-str.Length();++i)
			strFill += ' ';

		if (align < 0)
			return str + strFill;
		else
			return strFill + str;
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,OOCore_formatter_t__ctor1,1,((in),const Omega::string_t&,format))
{
	OOBase::SmartPtr<format_state_t> s = new (OOCore::throwing) format_state_t();

	// Split up the string
	parse_format(format,s->m_strPrefix,s->m_inserts);

	return s.detach();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(void*,OOCore_formatter_t__ctor2,1,((in),const void*,handle))
{
	const format_state_t* s = static_cast<const format_state_t*>(handle);
	if (!s)
		return NULL;

	OOBase::SmartPtr<format_state_t> s_new = new (OOCore::throwing) format_state_t();

	s_new->m_strPrefix = s->m_strPrefix;

	bool pushed = false;
	for (size_t i=0; i!=s->m_inserts.size(); ++i)
	{
		const insert_t* ins = s->m_inserts.at(i);
		if (!pushed && ins->index == (unsigned long)-1)
		{
			s_new->m_strPrefix += ins->strFormat + ins->strSuffix;
		}
		else
		{
			int err = s_new->m_inserts.add(*ins);
			if (err != 0)
				OMEGA_THROW(err);

			pushed = true;
		}
	}

	return s_new.detach();
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_formatter_t__dctor,1,((in),void*,handle))
{
	format_state_t* s = static_cast<format_state_t*>(handle);
	if (s)
		delete s;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_formatter_t_get_arg,3,((in),const void*,handle,(out),unsigned long&,index,(out),Omega::string_t&,fmt))
{
	index = (unsigned long)-1;
	fmt.Clear();

	const format_state_t* s = static_cast<const format_state_t*>(handle);
	if (s)
	{
		// Find the lowest index (from left to right)
		for (size_t i=0; i!=s->m_inserts.size(); ++i)
		{
			const insert_t* ins = s->m_inserts.at(i);
			if (ins->index < index)
			{
				index = ins->index;
				fmt = ins->strFormat;
			}
		}
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_formatter_t_set_arg,3,((in),void*,handle,(in),unsigned long,index,(in),const Omega::string_t&,arg))
{
	format_state_t* s = static_cast<format_state_t*>(handle);
	if (s)
	{
		// Update 'index'
		for (size_t i=0;i<s->m_inserts.size();++i)
		{
			insert_t* ins = s->m_inserts.at(i);
			if (ins->index == index)
			{
				ins->strFormat = align(arg,ins->alignment);
				ins->index = (unsigned long)-1;
				return;
			}
		}
	
		// Just append it
		insert_t ins2;
		ins2.alignment = 0;
		ins2.index = (unsigned long)-1;
		ins2.strFormat = " " + arg;

		int err = s->m_inserts.add(ins2);
		if (err != 0)
			OMEGA_THROW(err);
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::string_t,OOCore_formatter_t_cast,1,((in),const void*,handle))
{
	string_t strPrefix;

	format_state_t* s = const_cast<format_state_t*>(static_cast<const format_state_t*>(handle));
	if (s)
	{
		strPrefix += s->m_strPrefix;

		for (size_t i=0;i<s->m_inserts.size();++i)
		{
			insert_t* ins = s->m_inserts.at(i);
			if (ins->index != (unsigned long)-1)
			{
				strPrefix += '{' + Formatting::ToString(ins->index);
				if (ins->alignment != 0)
					strPrefix += ',' + Formatting::ToString(ins->alignment);

				strPrefix += ins->strFormat + '}';
			}
			else
			{
				strPrefix += ins->strFormat + ins->strSuffix;
			}
		}
	}

	return strPrefix;
}
