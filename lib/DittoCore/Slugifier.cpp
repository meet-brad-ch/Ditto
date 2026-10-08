/**
 * @file Slugifier.cpp
 * @brief Implements DittoCore::Slugifier.
 */
#include "Slugifier.h"

#include <cwctype>
#include <stdexcept>
#include <utility>

namespace DittoCore
{
	std::wstring Slugifier::Slugify(std::wstring_view text, std::wstring_view separator)
	{
		std::wstring slug;
		bool gap = false;
		for (const wchar_t c : ToSlugCharacters(text))
		{
			if (std::iswspace(c) || separator.find(c) != std::wstring_view::npos)
			{
				gap = true;
				continue;
			}
			if (gap && !slug.empty())
			{
				slug += separator;
			}
			gap = false;
			slug += c;
		}
		return slug;
	}

	std::wstring Slugifier::ToSlugCharacters(std::wstring_view text)
	{
		const std::unordered_map<wchar_t, std::wstring_view>& table = Transliterations();
		std::wstring ascii;
		for (const wchar_t c : text)
		{
			const auto found = table.find(c);
			if (found != table.end())
			{
				ascii += found->second;
			}
			else
			{
				ascii += c;
			}
		}

		std::wstring kept;
		for (wchar_t c : ascii)
		{
			if (c >= L'A' && c <= L'Z')
			{
				c = static_cast<wchar_t>(c - L'A' + L'a');
			}
			if (IsSlugCharacter(c))
			{
				kept += c;
			}
		}
		return kept;
	}

	bool Slugifier::IsSlugCharacter(wchar_t c)
	{
		return (c >= L'a' && c <= L'z') || (c >= L'0' && c <= L'9') || c == L'-' || std::iswspace(c);
	}

	const std::unordered_map<wchar_t, std::wstring_view>& Slugifier::Transliterations()
	{
		// upstream Ditto's table; the first entry wins where a letter is listed twice
		static const std::pair<std::wstring_view, std::wstring_view> pairs[]{
			// latin
			{ L"À", L"A" },
			{ L"Á", L"A" },
			{ L"Â", L"A" },
			{ L"Ã", L"A" },
			{ L"Ä", L"A" },
			{ L"Å", L"A" },
			{ L"Æ", L"AE" },
			{ L"Ç", L"C" },
			{ L"È", L"E" },
			{ L"É", L"E" },
			{ L"Ê", L"E" },
			{ L"Ë", L"E" },
			{ L"Ì", L"I" },
			{ L"Í", L"I" },
			{ L"Î", L"I" },
			{ L"Ï", L"I" },
			{ L"Ð", L"D" },
			{ L"Ñ", L"N" },
			{ L"Ò", L"O" },
			{ L"Ó", L"O" },
			{ L"Ô", L"O" },
			{ L"Õ", L"O" },
			{ L"Ö", L"O" },
			{ L"Ő", L"O" },
			{ L"Ø", L"O" },
			{ L"Ù", L"U" },
			{ L"Ú", L"U" },
			{ L"Û", L"U" },
			{ L"Ü", L"U" },
			{ L"Ű", L"U" },
			{ L"Ý", L"Y" },
			{ L"Þ", L"TH" },
			{ L"ß", L"ss" },
			{ L"à", L"a" },
			{ L"á", L"a" },
			{ L"â", L"a" },
			{ L"ã", L"a" },
			{ L"ä", L"a" },
			{ L"å", L"a" },
			{ L"æ", L"ae" },
			{ L"ç", L"c" },
			{ L"è", L"e" },
			{ L"é", L"e" },
			{ L"ê", L"e" },
			{ L"ë", L"e" },
			{ L"ì", L"i" },
			{ L"í", L"i" },
			{ L"î", L"i" },
			{ L"ï", L"i" },
			{ L"ð", L"d" },
			{ L"ñ", L"n" },
			{ L"ò", L"o" },
			{ L"ó", L"o" },
			{ L"ô", L"o" },
			{ L"õ", L"o" },
			{ L"ö", L"o" },
			{ L"ő", L"o" },
			{ L"ø", L"o" },
			{ L"ù", L"u" },
			{ L"ú", L"u" },
			{ L"û", L"u" },
			{ L"ü", L"u" },
			{ L"ű", L"u" },
			{ L"ý", L"y" },
			{ L"þ", L"th" },
			{ L"ÿ", L"y" },
			{ L"ẞ", L"SS" },
			// greek
			{ L"α", L"a" },
			{ L"β", L"b" },
			{ L"γ", L"g" },
			{ L"δ", L"d" },
			{ L"ε", L"e" },
			{ L"ζ", L"z" },
			{ L"η", L"h" },
			{ L"θ", L"8" },
			{ L"ι", L"i" },
			{ L"κ", L"k" },
			{ L"λ", L"l" },
			{ L"μ", L"m" },
			{ L"ν", L"n" },
			{ L"ξ", L"3" },
			{ L"ο", L"o" },
			{ L"π", L"p" },
			{ L"ρ", L"r" },
			{ L"σ", L"s" },
			{ L"τ", L"t" },
			{ L"υ", L"y" },
			{ L"φ", L"f" },
			{ L"χ", L"x" },
			{ L"ψ", L"ps" },
			{ L"ω", L"w" },
			{ L"ά", L"a" },
			{ L"έ", L"e" },
			{ L"ί", L"i" },
			{ L"ό", L"o" },
			{ L"ύ", L"y" },
			{ L"ή", L"h" },
			{ L"ώ", L"w" },
			{ L"ς", L"s" },
			{ L"ϊ", L"i" },
			{ L"ΰ", L"y" },
			{ L"ϋ", L"y" },
			{ L"ΐ", L"i" },
			{ L"Α", L"A" },
			{ L"Β", L"B" },
			{ L"Γ", L"G" },
			{ L"Δ", L"D" },
			{ L"Ε", L"E" },
			{ L"Ζ", L"Z" },
			{ L"Η", L"H" },
			{ L"Θ", L"8" },
			{ L"Ι", L"I" },
			{ L"Κ", L"K" },
			{ L"Λ", L"L" },
			{ L"Μ", L"M" },
			{ L"Ν", L"N" },
			{ L"Ξ", L"3" },
			{ L"Ο", L"O" },
			{ L"Π", L"P" },
			{ L"Ρ", L"R" },
			{ L"Σ", L"S" },
			{ L"Τ", L"T" },
			{ L"Υ", L"Y" },
			{ L"Φ", L"F" },
			{ L"Χ", L"X" },
			{ L"Ψ", L"PS" },
			{ L"Ω", L"W" },
			{ L"Ά", L"A" },
			{ L"Έ", L"E" },
			{ L"Ί", L"I" },
			{ L"Ό", L"O" },
			{ L"Ύ", L"Y" },
			{ L"Ή", L"H" },
			{ L"Ώ", L"W" },
			{ L"Ϊ", L"I" },
			{ L"Ϋ", L"Y" },
			// turkish
			{ L"ş", L"s" },
			{ L"Ş", L"S" },
			{ L"ı", L"i" },
			{ L"İ", L"I" },
			{ L"ç", L"c" },
			{ L"Ç", L"C" },
			{ L"ü", L"u" },
			{ L"Ü", L"U" },
			{ L"ö", L"o" },
			{ L"Ö", L"O" },
			{ L"ğ", L"g" },
			{ L"Ğ", L"G" },
			// russian
			{ L"а", L"a" },
			{ L"б", L"b" },
			{ L"в", L"v" },
			{ L"г", L"g" },
			{ L"д", L"d" },
			{ L"е", L"e" },
			{ L"ё", L"yo" },
			{ L"ж", L"zh" },
			{ L"з", L"z" },
			{ L"и", L"i" },
			{ L"й", L"j" },
			{ L"к", L"k" },
			{ L"л", L"l" },
			{ L"м", L"m" },
			{ L"н", L"n" },
			{ L"о", L"o" },
			{ L"п", L"p" },
			{ L"р", L"r" },
			{ L"с", L"s" },
			{ L"т", L"t" },
			{ L"у", L"u" },
			{ L"ф", L"f" },
			{ L"х", L"h" },
			{ L"ц", L"c" },
			{ L"ч", L"ch" },
			{ L"ш", L"sh" },
			{ L"щ", L"sh" },
			{ L"ъ", L"u" },
			{ L"ы", L"y" },
			{ L"ь", L"" },
			{ L"э", L"e" },
			{ L"ю", L"yu" },
			{ L"я", L"ya" },
			{ L"А", L"A" },
			{ L"Б", L"B" },
			{ L"В", L"V" },
			{ L"Г", L"G" },
			{ L"Д", L"D" },
			{ L"Е", L"E" },
			{ L"Ё", L"Yo" },
			{ L"Ж", L"Zh" },
			{ L"З", L"Z" },
			{ L"И", L"I" },
			{ L"Й", L"J" },
			{ L"К", L"K" },
			{ L"Л", L"L" },
			{ L"М", L"M" },
			{ L"Н", L"N" },
			{ L"О", L"O" },
			{ L"П", L"P" },
			{ L"Р", L"R" },
			{ L"С", L"S" },
			{ L"Т", L"T" },
			{ L"У", L"U" },
			{ L"Ф", L"F" },
			{ L"Х", L"H" },
			{ L"Ц", L"C" },
			{ L"Ч", L"Ch" },
			{ L"Ш", L"Sh" },
			{ L"Щ", L"Sh" },
			{ L"Ъ", L"U" },
			{ L"Ы", L"Y" },
			{ L"Ь", L"" },
			{ L"Э", L"E" },
			{ L"Ю", L"Yu" },
			{ L"Я", L"Ya" },
			// ukranian
			{ L"Є", L"Ye" },
			{ L"І", L"I" },
			{ L"Ї", L"Yi" },
			{ L"Ґ", L"G" },
			{ L"є", L"ye" },
			{ L"і", L"i" },
			{ L"ї", L"yi" },
			{ L"ґ", L"g" },
			// czech
			{ L"č", L"c" },
			{ L"ď", L"d" },
			{ L"ě", L"e" },
			{ L"ň", L"n" },
			{ L"ř", L"r" },
			{ L"š", L"s" },
			{ L"ť", L"t" },
			{ L"ů", L"u" },
			{ L"ž", L"z" },
			{ L"Č", L"C" },
			{ L"Ď", L"D" },
			{ L"Ě", L"E" },
			{ L"Ň", L"N" },
			{ L"Ř", L"R" },
			{ L"Š", L"S" },
			{ L"Ť", L"T" },
			{ L"Ů", L"U" },
			{ L"Ž", L"Z" },
			// polish
			{ L"ą", L"a" },
			{ L"ć", L"c" },
			{ L"ę", L"e" },
			{ L"ł", L"l" },
			{ L"ń", L"n" },
			{ L"ó", L"o" },
			{ L"ś", L"s" },
			{ L"ź", L"z" },
			{ L"ż", L"z" },
			{ L"Ą", L"A" },
			{ L"Ć", L"C" },
			{ L"Ę", L"e" },
			{ L"Ł", L"L" },
			{ L"Ń", L"N" },
			{ L"Ś", L"S" },
			{ L"Ź", L"Z" },
			{ L"Ż", L"Z" },
			// latvian
			{ L"ā", L"a" },
			{ L"č", L"c" },
			{ L"ē", L"e" },
			{ L"ģ", L"g" },
			{ L"ī", L"i" },
			{ L"ķ", L"k" },
			{ L"ļ", L"l" },
			{ L"ņ", L"n" },
			{ L"š", L"s" },
			{ L"ū", L"u" },
			{ L"ž", L"z" },
			{ L"Ā", L"A" },
			{ L"Č", L"C" },
			{ L"Ē", L"E" },
			{ L"Ģ", L"G" },
			{ L"Ī", L"i" },
			{ L"Ķ", L"k" },
			{ L"Ļ", L"L" },
			{ L"Ņ", L"N" },
			{ L"Š", L"S" },
			{ L"Ū", L"u" },
			{ L"Ž", L"Z" },
			// currency
			{ L"€", L"euro" },
			{ L"₢", L"cruzeiro" },
			{ L"₣", L"french franc" },
			{ L"£", L"pound" },
			{ L"₤", L"lira" },
			{ L"₥", L"mill" },
			{ L"₦", L"naira" },
			{ L"₧", L"peseta" },
			{ L"₨", L"rupee" },
			{ L"₩", L"won" },
			{ L"₪", L"new shequel" },
			{ L"₫", L"dong" },
			{ L"₭", L"kip" },
			{ L"₮", L"tugrik" },
			{ L"₯", L"drachma" },
			{ L"₰", L"penny" },
			{ L"₱", L"peso" },
			{ L"₲", L"guarani" },
			{ L"₳", L"austral" },
			{ L"₴", L"hryvnia" },
			{ L"₵", L"cedi" },
			{ L"¢", L"cent" },
			{ L"¥", L"yen" },
			{ L"元", L"yuan" },
			{ L"円", L"yen" },
			{ L"﷼", L"rial" },
			{ L"₠", L"ecu" },
			{ L"¤", L"currency" },
			{ L"฿", L"baht" },
			{ L"$", L"dollar" },
			// symbols
			{ L"©", L"(c)" },
			{ L"œ", L"oe" },
			{ L"Œ", L"OE" },
			{ L"∑", L"sum" },
			{ L"®", L"(r)" },
			{ L"†", L"+" },
			{ L"“", L"\"" },
			{ L"∂", L"d" },
			{ L"ƒ", L"f" },
			{ L"™", L"tm" },
			{ L"℠", L"sm" },
			{ L"…", L"..." },
			{ L"˚", L"o" },
			{ L"º", L"o" },
			{ L"ª", L"a" },
			{ L"•", L"*" },
			{ L"∆", L"delta" },
			{ L"∞", L"infinity" },
			{ L"♥", L"love" },
			{ L"&", L"and" },
			{ L"|", L"or" },
			{ L"<", L"less" },
			{ L">", L"greater" }
		};
		static const std::unordered_map<wchar_t, std::wstring_view> table = []()
		{
			std::unordered_map<wchar_t, std::wstring_view> built;
			for (const auto& [from, to] : pairs)
			{
				if (from.size() != 1)
				{
					throw std::logic_error("a slug table key is not one UTF-16 character");
				}
				built.emplace(from.front(), to);
			}
			return built;
		}();
		return table;
	}
}
