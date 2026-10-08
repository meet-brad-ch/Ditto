/**
 * @file DatabasePath.cpp
 * @brief Implements DittoCore::DatabasePath.
 */
#include "DatabasePath.h"

namespace DittoCore
{
	std::filesystem::path DatabasePath::Resolve(const std::filesystem::path& configured, const std::filesystem::path& defaultDirectory)
	{
		if (!configured.empty())
		{
			return configured;
		}
		return defaultDirectory / DefaultFileName;
	}

	std::filesystem::path DatabasePath::MarkedAsBad(const std::filesystem::path& database)
	{
		std::filesystem::path marked{ database };
		marked.replace_filename(database.stem().native() + L"_BAD" + database.extension().native());
		return marked;
	}
}
