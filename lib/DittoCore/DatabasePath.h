/**
 * @file DatabasePath.h
 * @brief Declares DittoCore::DatabasePath.
 */
#pragma once

#include <filesystem>

namespace DittoCore
{
	/**
	 * @brief Decides which database file Ditto opens.
	 *
	 * With no database path in the settings (first run, or settings removed by an uninstall),
	 * the database is Ditto.db in the default location. That file may hold the existing clip
	 * history, so it is used, never a new, empty database next to it.
	 */
	class DatabasePath
	{
	public:
		/** @brief File name of the database in the default location. */
		static constexpr const wchar_t* DefaultFileName{ L"Ditto.db" };

		/**
		 * @brief The database to open.
		 * @param configured The database path from the settings (DBPath3); empty when none is set.
		 * @param defaultDirectory The default location: the app data folder, or empty (the current
		 *        directory) for a portable install.
		 * @return @p configured when it is set, otherwise Ditto.db in @p defaultDirectory, whether
		 *         or not that file exists.
		 */
		static std::filesystem::path Resolve(const std::filesystem::path& configured, const std::filesystem::path& defaultDirectory);

		/**
		 * @brief The name a damaged database is renamed to: "_BAD" before the file's extension, in
		 *        the same folder.
		 * @param database The damaged database file.
		 * @return E.g. C:\\a.b\\Ditto_BAD.db for C:\\a.b\\Ditto.db (dots in folder names stay);
		 *         the file name with "_BAD" appended when it has no extension.
		 */
		static std::filesystem::path MarkedAsBad(const std::filesystem::path& database);
	};
}
