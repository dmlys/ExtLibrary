#pragma once
#include <string>
#include <string_view>
#include <ostream>

namespace ext
{
	/// ascii case insensitive(aci) char_traits в рамках набора символов ascii(первые 127)
	struct aci_char_traits : std::char_traits<char>
	{
	public:
		/// std::toupper/std::tolower(c, std::locale::classic())
		/// достаточно медленны, как минимум на msvs 2010
		/// поэтому используем ручную реализацию
		static char toupper(char c);
		static char tolower(char c);

		static bool eq(char c1, char c2) { return toupper(c1) == toupper(c2); }
		static bool lt(char c1, char c2) { return toupper(c1) < toupper(c2);  }

		static int compare(const char * s1, const char * s2, size_t n);

		static const char * find(const char * s, std::size_t n, char a);
	};

	using aci_string      = std::basic_string     <char, aci_char_traits>;
	using aci_string_view = std::basic_string_view<char, aci_char_traits>;


	inline char aci_char_traits::toupper(char c)
	{
		if (c >= 'a' && c <= 'z')
			return c + ('A' - 'a');
		else
			return c;
	}

	inline char aci_char_traits::tolower(char c)
	{
		if (c >= 'A' && c <= 'Z')
			return c + ('a' - 'A');
		else
			return c;
	}

	inline int aci_char_traits::compare(const char * s1, const char * s2, size_t n)
	{
		while ( n-- )
		{
			//if (lt(*s1, *s2)) return -1;
			//if (lt(*s2, *s1)) return +1;
			char c1 = toupper(*s1);
			char c2 = toupper(*s2);
			if (c1 < c2) return -1;
			if (c2 < c1) return +1;

			++s1; ++s2;
		}
		return 0;
	}

	inline const char * aci_char_traits::find(const char * s, std::size_t n, char a)
	{
		const char * end = s + n;
		for (; s < end; ++s)
		{
			if (eq(*s, a))
				return s;
		}

		return nullptr;
	}

	inline std::ostream & operator <<(std::ostream & os, aci_string const & str)
	{
		return os << str.c_str();
	}
}
