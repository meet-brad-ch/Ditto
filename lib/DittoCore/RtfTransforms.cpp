/**
 * @file RtfTransforms.cpp
 * @brief Implements DittoCore::RtfTransforms.
 */
#include "RtfTransforms.h"
#include "RtfJoin.h"

namespace DittoCore
{
	std::string RtfTransforms::RemoveLineFeeds(std::string_view rtf)
	{
		std::string result(rtf);
		ReplaceAll(result, "\\par\r\n", " ");
		ReplaceAll(result, "\\par ", " ");
		ReplaceAll(result, "\\line ", " ");
		return result;
	}

	std::string RtfTransforms::AddLineFeeds(std::string_view rtf, int count)
	{
		std::string breaks;
		for (int i = 0; i < count; i++)
		{
			breaks += "\\par\r\n";
		}
		return InsertBeforeEnd(rtf, breaks);
	}

	std::string RtfTransforms::AddDateTime(std::string_view rtf, std::wstring_view formattedTime)
	{
		return InsertBeforeEnd(rtf, "\\par\r\n\\par\r\n" + RtfJoin::Escape(formattedTime));
	}

	std::string RtfTransforms::InsertBeforeEnd(std::string_view rtf, std::string_view text)
	{
		std::string result(rtf);
		const std::size_t end = result.rfind('}');
		if (end != std::string::npos)
		{
			result.insert(end, text);
		}
		return result;
	}

	void RtfTransforms::ReplaceAll(std::string& text, std::string_view from, std::string_view to)
	{
		std::size_t pos = text.find(from);
		while (pos != std::string::npos)
		{
			text.replace(pos, from.size(), to);
			pos = text.find(from, pos + to.size());
		}
	}
}
