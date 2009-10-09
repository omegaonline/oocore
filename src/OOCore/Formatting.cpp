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
	class LocaleHolder
	{
	public:
		LocaleHolder()
		{			
#if defined(_WIN32)
			// Sync Win32 locale with internal locale
			LCID lcid = GetThreadLocale();

			char buffer[256] = {0};
			if (!GetLocaleInfoA(lcid,LOCALE_SENGLANGUAGE,buffer,255))
				OMEGA_THROW(GetLastError());

			std::string str = buffer;

			if (!GetLocaleInfoA(lcid,LOCALE_SENGCOUNTRY,buffer,255))
				OMEGA_THROW(GetLastError());
		
			str += "_";
			str += buffer;

			if (!GetLocaleInfoA(lcid,LOCALE_IDEFAULTANSICODEPAGE,buffer,255))
				OMEGA_THROW(GetLastError());
			
			if (strcmp(buffer,"0") != 0)
			{
				str += ".";
				str += buffer;
			}
			else
			{
				if (!GetLocaleInfoA(lcid,LOCALE_IDEFAULTCODEPAGE,buffer,255))
					OMEGA_THROW(GetLastError());

				if (strcmp(buffer,"1") != 0)
				{
					str += ".";
					str += buffer;
				}
			}
			
			// Sync the crt locale with the Win32 one
			m_prev_locale = setlocale(LC_ALL,str.c_str());
#endif
		}

		~LocaleHolder()
		{
#if defined(_WIN32)
			setlocale(LC_ALL,m_prev_locale.c_str());
#endif
		}

	private:
		std::string m_prev_locale;
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

	void do_intl_mon(std::string& str, bool negative)
	{
#if defined(_WIN32)
		LCID lcid = GetThreadLocale();

		char decimal_point[5] = {0};
		if (!GetLocaleInfoA(lcid,LOCALE_SMONDECIMALSEP,decimal_point,sizeof(decimal_point)))
			OMEGA_THROW(GetLastError());

		char grouping_buf[128] = {0};
		if (!GetLocaleInfoA(lcid,LOCALE_SMONGROUPING,grouping_buf,sizeof(grouping_buf)))
			OMEGA_THROW(GetLastError());

		// Build a crt style grouping
		char grouping_trans[128] = {0};
		const char* grouping = grouping_trans;
		size_t g = 0;
		for (const char* c = grouping_buf;*c != '\0' && g<sizeof(grouping_trans)-1;++g)
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
			OMEGA_THROW(GetLastError());
#else
		const lconv* lc = localeconv();
		if (!lc)
			return;

		const char* decimal_point = lc->mon_decimal_point;
		const char* grouping = lc->mon_grouping;
		const char* thousands_sep = lc->mon_thousands_sep;
#endif

		size_t pos = str.find('.');
		if (pos != std::string::npos)
			str.replace(pos,1,decimal_point);

		for (int grp = CHAR_MAX;;)
		{
			char n_grp = *(grouping++);
			if (n_grp != 0)
				grp = n_grp;

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
				OMEGA_THROW(GetLastError());
			sep_by_space = (val == 1);

			if (!GetLocaleInfoA(lcid,LOCALE_INEGSYMPRECEDES | LOCALE_RETURN_NUMBER,(LPSTR)&val,sizeof(val)))
				OMEGA_THROW(GetLastError());
			cs_precedes = (val == 1);

			if (!GetLocaleInfoA(lcid,LOCALE_INEGSIGNPOSN | LOCALE_RETURN_NUMBER,(LPSTR)&val,sizeof(val)))
				OMEGA_THROW(GetLastError());
			posn = static_cast<int>(val);

			char sign_symbol[6];
			if (!GetLocaleInfoA(lcid,LOCALE_SNEGATIVESIGN,sign_symbol,sizeof(sign_symbol)))
				OMEGA_THROW(GetLastError());

			sign = sign_symbol;
		}
		else
		{
			DWORD val = 0;
			if (!GetLocaleInfoA(lcid,LOCALE_IPOSSEPBYSPACE | LOCALE_RETURN_NUMBER,(LPSTR)&val,sizeof(val)))
				OMEGA_THROW(GetLastError());
			sep_by_space = (val == 1);

			if (!GetLocaleInfoA(lcid,LOCALE_IPOSSYMPRECEDES | LOCALE_RETURN_NUMBER,(LPSTR)&val,sizeof(val)))
				OMEGA_THROW(GetLastError());
			cs_precedes = (val == 1);

			if (!GetLocaleInfoA(lcid,LOCALE_IPOSSIGNPOSN | LOCALE_RETURN_NUMBER,(LPSTR)&val,sizeof(val)))
				OMEGA_THROW(GetLastError());
			posn = static_cast<int>(val);

			char sign_symbol[6];
			if (!GetLocaleInfoA(lcid,LOCALE_SPOSITIVESIGN,sign_symbol,sizeof(sign_symbol)))
				OMEGA_THROW(GetLastError());

			sign = sign_symbol;
		}

		char currency_buf[8];
		if (!GetLocaleInfoA(lcid,LOCALE_SCURRENCY,currency_buf,sizeof(currency)))
			OMEGA_THROW(GetLastError());

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
	
	string_t fmt_currency(const double& val, int precision)
	{
		std::ostringstream ss;
		ss.imbue(std::locale::classic());
		
		ss.setf(std::ios_base::fixed,std::ios_base::floatfield);
		
		if (precision < 0)
		{
#if defined(_WIN32)
			LCID lcid = GetThreadLocale();

			DWORD v = 0;
			if (!GetLocaleInfoA(lcid,LOCALE_IDIGITS | LOCALE_RETURN_NUMBER,(LPSTR)&v,sizeof(v)))
				OMEGA_THROW(GetLastError());

			precision = static_cast<int>(v);
#else
			precision = 2;
			const lconv* lc = localeconv();
			if (lc && lc->frac_digits != CHAR_MAX)
				precision = lc->frac_digits;
#endif
		}
					
		ss.precision(precision);

		ss << val;

		std::string ret = ss.str();

		do_intl_mon(ret,val < 0.0);
		
		return string_t(ret.c_str(),false);
	}

	template <typename T>
	string_t fmt_currency(T val, int precision)
	{
		return fmt_currency(static_cast<double>(val),precision);
	}

	void do_intl(std::string& str, bool bThou)
	{
#if defined(_WIN32)
		LCID lcid = GetThreadLocale();

		char decimal_point[5] = {0};
		if (!GetLocaleInfoA(lcid,LOCALE_SDECIMAL,decimal_point,sizeof(decimal_point)))
			OMEGA_THROW(GetLastError());

		
		char thousands_sep[5] = {0};
		char trans_grouping[128] = {0};	
		const char* grouping = trans_grouping;

		if (bThou)
		{
			char grouping_buf[128] = {0};
			if (!GetLocaleInfoA(lcid,LOCALE_SGROUPING,grouping_buf,sizeof(grouping_buf)))
				OMEGA_THROW(GetLastError());

			// Build a crt style grouping
			
			size_t g = 0;
			for (const char* c = grouping_buf;*c != '\0' && g<sizeof(trans_grouping)-1;++g)
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
				OMEGA_THROW(GetLastError());
		}
#else
		const lconv* lc = localeconv();
		if (!lc)
			return;

		const char* decimal_point = lc->decimal_point;
		const char* grouping = lc->grouping;
		const char* thousands_sep = lc->thousands_sep;
#endif

		size_t pos = str.find('.');
		if (pos != std::string::npos)
			str.replace(pos,1,decimal_point);

		if (!bThou)
			return;
		
		for (int grp = CHAR_MAX;;)
		{
			char n_grp = *(grouping++);
			if (n_grp != 0)
				grp = n_grp;

			if (grp == CHAR_MAX)
				break;
		
			if (pos <= static_cast<size_t>(grp))
				break;

			pos -= grp;
			str.insert(pos,thousands_sep);
		}
	}

	string_t fmt_number(const double& val, int precision)
	{
		std::ostringstream ss;
		ss.imbue(std::locale::classic());
		
		ss.setf(std::ios_base::fixed,std::ios_base::floatfield);
		
		if (precision < 0)
		{
#if defined(_WIN32)
			LCID lcid = GetThreadLocale();

			DWORD v = 0;
			if (!GetLocaleInfoA(lcid,LOCALE_IDIGITS | LOCALE_RETURN_NUMBER,(LPSTR)&v,sizeof(v)))
				OMEGA_THROW(GetLastError());

			precision = static_cast<int>(v);
#else
			precision = 2;
			const lconv* lc = localeconv();
			if (lc && lc->frac_digits != CHAR_MAX)
				precision = lc->frac_digits;
#endif
		}
					
		ss.precision(precision);

		ss << val;

		std::string ret = ss.str();

		do_intl(ret,true);
		
		return string_t(ret.c_str(),false);
	}

	template <typename T>
	string_t fmt_number(T val, int precision)
	{
		return fmt_number(static_cast<double>(val),precision);
	}

	template <typename T>
	string_t fmt_decimal(T val, int precision)
	{
		std::ostringstream ss;
		ss.imbue(std::locale::classic());
		
		if (precision >= 0)
		{
			ss.fill('0');
			ss.width(precision);
		}

		ss << val;

		return string_t(ss.str().c_str(),false);
	}

	string_t fmt_decimal(const double& val, int precision)
	{
		return fmt_decimal(static_cast<uint64_t>(val),precision);
	}

	string_t fmt_hex(uint64_t val, bool capital, int precision)
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

	string_t fmt_hex(const double& val, bool capital, int precision)
	{
		return fmt_hex(*reinterpret_cast<const uint64_t*>(&val),capital,precision);
	}

	string_t fmt_fixed(const double& val, int precision)
	{
		std::ostringstream ss;
		ss.imbue(std::locale::classic());

		ss.setf(std::ios_base::fixed,std::ios_base::floatfield);
		
		if (precision < 0)
		{
#if defined(_WIN32)
			LCID lcid = GetThreadLocale();

			DWORD v = 0;
			if (!GetLocaleInfoA(lcid,LOCALE_IDIGITS | LOCALE_RETURN_NUMBER,(LPSTR)&v,sizeof(v)))
				OMEGA_THROW(GetLastError());

			precision = static_cast<int>(v);
#else
			precision = 2;
			const lconv* lc = localeconv();
			if (lc && lc->frac_digits != CHAR_MAX)
				precision = lc->frac_digits;
#endif
		}

		ss.precision(precision);

		ss << val;

		std::string ret = ss.str();

		do_intl(ret,false);

		return string_t(ret.c_str(),false);
	}

	template <typename T>
	string_t fmt_fixed(T val, int precision)
	{
		return fmt_fixed(static_cast<double>(val),precision);
	}

	string_t fmt_scientific(const double& val, bool capital, int precision)
	{
		std::ostringstream ss;
		ss.imbue(std::locale::classic());

		ss.setf(std::ios_base::scientific,std::ios_base::floatfield);
		
		if (precision >= 0)
			ss.precision(precision);

		if (capital)
			ss.setf(std::ios_base::uppercase);
		
		ss << val;

		std::string ret = ss.str();

		do_intl(ret,false);

		return string_t(ret.c_str(),false);
	}

	template <typename T>
	string_t fmt_scientific(T val, bool capital, int precision)
	{
		return fmt_scientific(static_cast<double>(val),capital,precision);
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

	string_t fmt_round_trip(double val)
	{
		void* TODO;
		return fmt_fixed(val,-1);
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
	string_t fmt_custom(T val, const string_t& strFormat, int def_precision);

	template <typename T>
	string_t fmt_recurse(T val, const string_t& strFormat, int def_precision, bool bRecurse);

	template <typename T>
	std::string fmt_custom_sci(T val, const string_t& strFormat, size_t found, int def_precision)
	{
		bool capital = (strFormat[found++] == L'E');

		bool bPlus = false;
		if (strFormat[found] == L'+')
		{
			bPlus = true;
			++found;
		}
		else if (strFormat[found] == L'-')
			++found;

		// Count the zeros
		int exp_precision = 0;
		for (;strFormat[found++]==L'0';++exp_precision)
			;

		std::ostringstream ss;
		ss.imbue(std::locale::classic());

		ss.setf(std::ios_base::scientific,std::ios_base::floatfield);
		
		ss.precision(def_precision);

		if (capital)
			ss.setf(std::ios_base::uppercase);
		
		ss << val;

		return ss.str();
	}

	template <typename T>
	string_t fmt_rnd_away(T val)
	{
		return fmt_decimal(val,-1);
	}

	string_t fmt_rnd_away(const double& val)
	{
		if (val < 0.0)
			return fmt_decimal(static_cast<int64_t>(val-0.5),-1);
		else
			return fmt_decimal(static_cast<int64_t>(val+0.5),-1);
	}

	template <typename T>
	string_t fmt_custom_i(T val, const string_t& strFormat, int def_precision)
	{
		if (strFormat == L"##" || strFormat == L"00")
			return fmt_rnd_away(val);
		
#if defined(_WIN32)
		LCID lcid = GetThreadLocale();

		wchar_t decimal_point[5] = {0};
		if (!GetLocaleInfoW(lcid,LOCALE_SDECIMAL,decimal_point,5))
			OMEGA_THROW(GetLastError());

		string_t decimal(decimal_point);
		
		wchar_t thousands_sep[5] = {0};
		if (!GetLocaleInfoW(lcid,LOCALE_STHOUSAND,thousands_sep,5))
			OMEGA_THROW(GetLastError());

		string_t thousands(thousands_sep);
#else
		const lconv* lc = localeconv();
		if (!lc)
			return;

		string_t decimal(lc->decimal_point,false);
		string_t thousands(lc->thousands_sep,false);
#endif

		string_t strFind = L"0#Ee";
		strFind += decimal[0];
		strFind += thousands[0];

		// Work out precision and mode...
		size_t prefix = size_t(-1);
		bool sci = false;
		bool exp_plus = false;
		size_t exp_prec = 0;
		int precision = 0;
		int width = 0;
		bool dp_prec = false;
		bool group = false;

		for (size_t pos = 0;;)
		{
			size_t found = find_skip_quote(strFormat,pos,strFind);
			if (found == string_t::npos)
				break;

			if (strFormat[found] == L'E' || strFormat[found] == L'e')
			{
				if (found >= strFormat.Length()-1)
					break;

				if (strFormat[found+1] == L'-' || strFormat[found+1] == L'+')
				{
					if (found >= strFormat.Length()-2)
						break;

					exp_plus = (strFormat[found+1] == L'+');

					if (strFormat[found+2] == L'0')
					{
						exp_prec = found+2;
						sci = true;
					}
				}
				else if (strFormat[found+1] == L'0')
				{
					sci = true;
					exp_prec = found+1;
				}
				
				if (sci)
				{
					if (prefix == size_t(-1))
						prefix = found;

					for (found = exp_prec;strFormat[found]==L'0' || strFormat[found]==L'#';++found)
						;

					break;
				}
			}
			else if (strFormat[found] == L'#')
			{
				if (width > 0 && !dp_prec)
					break;

				if (prefix == size_t(-1))
					prefix = found;

				if (dp_prec)
					++precision;
			}
			else if (strFormat[found] == L'0')
			{
				if (prefix == size_t(-1))
					prefix = found;

				if (dp_prec)
					++precision;
				
				++width;
			}
			else if (strFormat.Compare(decimal,found,decimal.Length()) == 0)
			{
				if (dp_prec)
					break;

				if (prefix == size_t(-1))
					prefix = found;

				dp_prec = true;
				precision = 0;
			}
			else if (strFormat.Compare(thousands,found,thousands.Length()) == 0)
			{
				if (dp_prec || group)
					break;

				if (found > 0 && found < strFormat.Length()-1 &&
					(strFormat[found-1]==L'#' || strFormat[found-1]==L'0') &&
					(strFormat[found+1]==L'#' || strFormat[found+1]==L'0'))
				{
					group = true;
				}				
			}

			pos = found + 1;
		}

		std::string str;

		std::ostringstream ss;
		ss.imbue(std::locale::classic());

		ss.width(width);
		ss.fill('0');

		if (sci)
		{
			ss.setf(std::ios_base::scientific,std::ios_base::floatfield);
		
			ss.precision(precision);

			//if (capital)
			//	ss.setf(std::ios_base::uppercase);
			
			ss << val;

			str = ss.str();
		}
		else
		{
			if (dp_prec)
				ss.setf(std::ios_base::fixed,std::ios_base::floatfield);
			
			ss.precision(precision);

			ss << val;

			str = ss.str();
		}

		do_intl(str,group);

		return string_t(str.c_str(),false);
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
				return fmt_hex(static_cast<uint64_t>(val),capital,precision);

			case round_trip:

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
			return fmt_custom_i(val,strFormat,def_precision);
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
				return fmt_recurse(val,parts[1],def_precision,false);
			else
				return fmt_recurse(val,parts[0],def_precision,false);

		default:
			return fmt_custom_i(val,strFormat,def_precision);
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
	if (strFormat.IsEmpty())
	{
		std::ostringstream ss;
		ss.setf(std::ios_base::boolalpha);
		ss << val;
		
		return string_t(ss.str().c_str(),false);
	}

	std::vector<string_t> parts;
	if (parse_custom(strFormat,parts) != 2)
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
