/**
 * @file IniSettingsStore.h
 * @brief Declares DittoCore::IniSettingsStore.
 */
#pragma once

#include "ISettingsStore.h"

namespace DittoCore
{
	/**
	 * @brief Settings in an ini file (Ditto.Settings: portable, Windows Store and Chocolatey
	 *        installs), read and written with the Windows profile API.
	 *
	 * The default section is [Ditto]. Section and value names are not case-sensitive. Numbers
	 * are read as GetPrivateProfileInt reads them (see GetLong). The file is UTF-16 when it was
	 * created with a byte order mark (CGetSetOptions creates it so); without one, Windows writes
	 * it in the ANSI code page. Binary values are not supported. The read and write rules are the
	 * ones CGetSetOptions used before this class existed.
	 */
	class IniSettingsStore final : public ISettingsStore
	{
	public:
		/** @brief The section of the empty section name. */
		static constexpr const wchar_t* DefaultSection{ L"Ditto" };

		/**
		 * @brief Creates the store; the file is not opened or created until a value is read or
		 *        written.
		 * @param filePath The ini file.
		 */
		explicit IniSettingsStore(std::wstring filePath);

		/**
		 * @brief Reads a number as GetPrivateProfileInt does: optional leading blanks and sign,
		 *        then decimal digits up to the first other character ("42abc" is 42, "abc" is 0),
		 *        or hexadecimal after a lower-case "0x"; the result wraps to 32 bits.
		 * @param section The section; empty for [Ditto].
		 * @param name The value name.
		 * @param defaultValue The result when the file, section or value is missing, or the value
		 *        is empty.
		 * @return The value, or @p defaultValue.
		 */
		long GetLong(const std::wstring& section, const std::wstring& name, long defaultValue) const override;

		/**
		 * @brief Writes a number as its decimal text.
		 * @param section The section; empty for [Ditto].
		 * @param name The value name.
		 * @param value The value.
		 * @return True when the file was written.
		 */
		bool SetLong(const std::wstring& section, const std::wstring& name, long value) override;

		/**
		 * @brief Reads a text as GetPrivateProfileString does (blanks around it and one pair of
		 *        enclosing quotes removed). The read buffer starts at 10000 characters and doubles
		 *        until the value fits.
		 * @param section The section; empty for [Ditto].
		 * @param name The value name.
		 * @param defaultValue The result when the file, section or value is missing.
		 * @param maxSize The buffer limit in characters: the result (also the default) is cut to
		 *        maxSize - 1 characters; 0 gives an empty text. Negative (NoMaxSize) for no limit.
		 * @return The text, or @p defaultValue.
		 */
		std::wstring GetString(const std::wstring& section, const std::wstring& name, const std::wstring& defaultValue, int maxSize) const override;

		/**
		 * @brief Writes a text.
		 * @param section The section; empty for [Ditto].
		 * @param name The value name.
		 * @param value The text.
		 * @return True when the file was written.
		 */
		bool SetString(const std::wstring& section, const std::wstring& name, const std::wstring& value) override;

		/**
		 * @brief Not supported: an ini file holds no binary values.
		 * @param section The section.
		 * @param name The value name.
		 * @return Never returns.
		 * @throws std::logic_error Always.
		 */
		std::vector<std::byte> GetData(const std::wstring& section, const std::wstring& name) const override;

		/**
		 * @brief Not supported: an ini file holds no binary values.
		 * @param section The section.
		 * @param name The value name.
		 * @param data The bytes.
		 * @return Never returns.
		 * @throws std::logic_error Always.
		 */
		bool SetData(const std::wstring& section, const std::wstring& name, std::span<const std::byte> data) override;

		/**
		 * @brief Deletes a section with all its values; a missing section is no error.
		 * @param section The section; must not be empty.
		 * @throws std::invalid_argument When @p section is empty.
		 * @throws std::runtime_error When the file cannot be written.
		 */
		void DeleteSection(const std::wstring& section) override;

	private:
		/** @brief The first size of GetString's read buffer, in characters. */
		static constexpr unsigned long InitialBufferLength{ 10000 };

		/**
		 * @brief The ini section of a section name.
		 * @param section The section; empty for [Ditto].
		 * @return @p section, or DefaultSection when it is empty.
		 */
		static const wchar_t* SectionName(const std::wstring& section);

		/// The ini file.
		std::wstring m_filePath{};
	};
}
