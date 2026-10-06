/**
 * @file DatabasePathTests.cpp
 * @brief Unit tests for DittoCore::DatabasePath, including the empty-setting regression.
 */
#include "DatabasePath.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

using DittoCore::DatabasePath;

namespace
{
	/** A fresh directory under the temp folder, removed again at the end of the test. */
	class TempDirectory
	{
	public:
		explicit TempDirectory(const std::string& name)
			: m_path{ std::filesystem::temp_directory_path() / ("DittoTests_" + name) }
		{
			std::filesystem::remove_all(m_path);
			std::filesystem::create_directories(m_path);
		}
		~TempDirectory() { std::filesystem::remove_all(m_path); }
		TempDirectory(const TempDirectory&) = delete;
		TempDirectory& operator=(const TempDirectory&) = delete;

		const std::filesystem::path& Path() const { return m_path; }

	private:
		std::filesystem::path m_path{};
	};
}

TEST(DatabasePath, ConfiguredPathIsUsedAsIs)
{
	const std::filesystem::path configured{ L"D:\\Clips\\work.db" };

	EXPECT_EQ(DatabasePath::Resolve(configured, L"C:\\Users\\me\\AppData\\Roaming\\Ditto"), configured);
}

TEST(DatabasePath, EmptySettingUsesDittoDbInDefaultDirectory)
{
	const std::filesystem::path directory{ L"C:\\Users\\me\\AppData\\Roaming\\Ditto" };

	EXPECT_EQ(DatabasePath::Resolve({}, directory), directory / L"Ditto.db");
}

// Regression: with the settings removed by an uninstall, Ditto created Ditto_1.db next to the
// existing Ditto.db and showed an empty history.
TEST(DatabasePath, EmptySettingKeepsExistingDittoDb)
{
	const TempDirectory directory("EmptySettingKeepsExistingDittoDb");
	std::ofstream(directory.Path() / L"Ditto.db") << "history";

	const std::filesystem::path resolved = DatabasePath::Resolve({}, directory.Path());

	EXPECT_EQ(resolved, directory.Path() / L"Ditto.db");
	EXPECT_TRUE(std::filesystem::exists(resolved));
	EXPECT_FALSE(std::filesystem::exists(directory.Path() / L"Ditto_1.db"));
}

TEST(DatabasePath, PortableEmptySettingUsesRelativeDittoDb)
{
	EXPECT_EQ(DatabasePath::Resolve({}, {}), std::filesystem::path(L"Ditto.db"));
}
