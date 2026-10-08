#pragma once

/** @brief A four-part product version. */
class VersionInfo
{
public:
	/** @brief The major version. */
	int Major{};
	/** @brief The minor version. */
	int Minor{};
	/** @brief The revision. */
	int Revision{};
	/** @brief The build number. */
	int Build{};
};

/** @brief The version of the running Ditto.exe and its text form. */
class CAppVersion
{
public:
	/**
	 * @brief The product version from Ditto.exe's version resource.
	 * @param exeFileName The path of the running Ditto.exe (the settings' GetExeFileName()).
	 * @return The version.
	 * @throws std::runtime_error when the version resource is missing or unreadable.
	 */
	static VersionInfo GetRunningVersion(const CString& exeFileName);

	/**
	 * @brief The text form of a version, "MM.mm.rr.bb" (two digits at least per part).
	 * @param version The version.
	 * @return The text.
	 */
	static CString GetVersionString(VersionInfo version);
};
