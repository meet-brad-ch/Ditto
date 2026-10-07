/**
 * @file CfHtmlFragment.h
 * @brief Declares DittoCore::CfHtmlFragment.
 */
#pragma once

#include <string>

namespace DittoCore
{
	/**
	 * @brief The parts of a CF_HTML ("HTML Format") block that Ditto keeps.
	 *
	 * All text is UTF-8, as the format defines it.
	 */
	struct CfHtmlFragment
	{
		/// The Version header value, empty when the block has none.
		std::string version{};
		/// The SourceURL header value, empty when the block has none.
		std::string sourceUrl{};
		/// The HTML between StartFragment and EndFragment, with surrounding whitespace removed.
		std::string fragment{};
	};
}
