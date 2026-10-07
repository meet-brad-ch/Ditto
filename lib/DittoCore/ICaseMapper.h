/**
 * @file ICaseMapper.h
 * @brief Declares DittoCore::ICaseMapper.
 */
#pragma once

#include <string>
#include <string_view>

namespace DittoCore
{
	/**
	 * @brief Upper- and lower-case mapping of UTF-16 text, injected into CaseTransforms (the app
	 *        uses ICU; tests use the C runtime).
	 */
	class ICaseMapper
	{
	public:
		/// Destroys the mapper.
		virtual ~ICaseMapper() = default;

		/**
		 * @brief Whether a character is upper case.
		 * @param c The character.
		 * @return True for an upper-case letter.
		 */
		virtual bool IsUpper(wchar_t c) const = 0;

		/**
		 * @brief The upper-case form of a character.
		 * @param c The character.
		 * @return Its upper-case form, or @p c when it has none.
		 */
		virtual wchar_t ToUpper(wchar_t c) const = 0;

		/**
		 * @brief The lower-case form of a character.
		 * @param c The character.
		 * @return Its lower-case form, or @p c when it has none.
		 */
		virtual wchar_t ToLower(wchar_t c) const = 0;

		/**
		 * @brief The upper-case form of a text; it may be longer (German sharp s becomes "SS").
		 * @param text The text.
		 * @return The upper-case text.
		 */
		virtual std::wstring ToUpper(std::wstring_view text) const = 0;

		/**
		 * @brief The lower-case form of a text.
		 * @param text The text.
		 * @return The lower-case text.
		 */
		virtual std::wstring ToLower(std::wstring_view text) const = 0;
	};
}
