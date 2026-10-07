/**
 * @file CaseTransforms.h
 * @brief Declares DittoCore::CaseTransforms.
 */
#pragma once

#include "ICaseMapper.h"

#include <string>
#include <string_view>

namespace DittoCore
{
	/**
	 * @brief The special-paste case changes of UTF-16 text.
	 */
	class CaseTransforms
	{
	public:
		/**
		 * @brief Creates the transforms.
		 * @param cases The case mapping; it must outlive this object.
		 */
		explicit CaseTransforms(const ICaseMapper& cases) noexcept;

		/**
		 * @brief The text in upper case.
		 * @param text The text.
		 * @return The upper-case text.
		 */
		std::wstring Upper(std::wstring_view text) const;

		/**
		 * @brief The text in lower case.
		 * @param text The text.
		 * @return The lower-case text.
		 */
		std::wstring Lower(std::wstring_view text) const;

		/**
		 * @brief Swaps the case of every character.
		 * @param text The text.
		 * @return The text with upper-case characters lowered and all others raised.
		 */
		std::wstring InvertCase(std::wstring_view text) const;

		/**
		 * @brief Camel case: each space-separated word starts upper case, its other letters are
		 *        lower case, and the spaces are removed ("hello big world" gives "HelloBigWorld").
		 * @param text The text.
		 * @return The camel-case text.
		 */
		std::wstring CamelCase(std::wstring_view text) const;

		/**
		 * @brief Lowers the text and raises the first letter of every space-separated word.
		 * @param text The text.
		 * @return The capitalized text.
		 */
		std::wstring Capitalize(std::wstring_view text) const;

		/**
		 * @brief Lowers the text and raises the first letter of every sentence: the start of the
		 *        text and the first non-white-space character after '.', '!' or '?'.
		 * @param text The text.
		 * @return The sentence-case text.
		 */
		std::wstring SentenceCase(std::wstring_view text) const;

	private:
		/// The case mapping.
		const ICaseMapper& m_cases;

		/**
		 * @brief Lowers the text, then raises the first character and the first character that
		 *        follows a run of separators.
		 * @param text The text.
		 * @param isSeparator Whether a character starts a new run.
		 * @param isSkipped Whether a character after a separator is passed over (stays in the run).
		 * @return The text.
		 */
		template <typename Separator, typename Skipped>
		std::wstring RaiseAfter(std::wstring_view text, Separator isSeparator, Skipped isSkipped) const;
	};
}
