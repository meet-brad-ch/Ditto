/**
 * @file RegistrySettingsStore.h
 * @brief Declares DittoCore::RegistrySettingsStore.
 */
#pragma once

#include "ISettingsStore.h"

#include <memory>
#include <optional>
#include <type_traits>

#include <windows.h>

namespace DittoCore
{
	/**
	 * @brief Settings in a registry key under HKEY_CURRENT_USER (Ditto's default storage,
	 *        HKCU\\Software\\Ditto).
	 *
	 * The default section is the root key itself; a named section is its sub key. Numbers are
	 * REG_DWORD, texts REG_SZ (written without a terminating null, as Ditto always wrote them)
	 * and binary values REG_BINARY. The read and write rules are the ones CGetSetOptions used
	 * before this class existed; each member documents them.
	 */
	class RegistrySettingsStore final : public ISettingsStore
	{
	public:
		/**
		 * @brief Creates the store; no key is opened or created until a value is read or written.
		 * @param rootPath The root key's path under HKEY_CURRENT_USER, e.g. Software\\Ditto.
		 */
		explicit RegistrySettingsStore(std::wstring rootPath);

		/**
		 * @brief Reads a number: the first 4 bytes of the value, whatever its type.
		 * @param section The sub key; empty for the root key.
		 * @param name The value name.
		 * @param defaultValue The result when the key or value is missing or longer than 4 bytes.
		 * @return The value as a signed 32-bit number, or @p defaultValue.
		 */
		long GetLong(const std::wstring& section, const std::wstring& name, long defaultValue) const override;

		/**
		 * @brief Writes a REG_DWORD (the key is created when missing).
		 * @param section The sub key; empty for the root key.
		 * @param name The value name.
		 * @param value The value (stored as its 32-bit pattern).
		 * @return True when the value was written.
		 */
		bool SetLong(const std::wstring& section, const std::wstring& name, long value) override;

		/**
		 * @brief Reads a text, up to its first null.
		 * @param section The sub key; empty for the root key.
		 * @param name The value name.
		 * @param defaultValue The result when the key or value is missing, or the read fails.
		 * @param maxSize The read limit in BYTES (not characters): a value that needs a larger
		 *        buffer than maxSize + 1 bytes is not read, and @p defaultValue is returned.
		 *        Negative (NoMaxSize) for no limit.
		 * @return The text; empty for a value of 0 bytes.
		 */
		std::wstring GetString(const std::wstring& section, const std::wstring& name, const std::wstring& defaultValue, int maxSize) const override;

		/**
		 * @brief Writes a REG_SZ of the text's characters, without a terminating null.
		 * @param section The sub key; empty for the root key.
		 * @param name The value name.
		 * @param value The text.
		 * @return True when the key was opened; the result of the write itself is not checked
		 *         (as before).
		 * @throws std::length_error When the text is larger than a registry value can hold.
		 */
		bool SetString(const std::wstring& section, const std::wstring& name, const std::wstring& value) override;

		/**
		 * @brief Reads a binary value (of any type).
		 * @param section The sub key; empty for the root key.
		 * @param name The value name.
		 * @return The bytes; empty when the key or value is missing or the read fails.
		 */
		std::vector<std::byte> GetData(const std::wstring& section, const std::wstring& name) const override;

		/**
		 * @brief Writes a REG_BINARY.
		 * @param section The sub key; empty for the root key.
		 * @param name The value name.
		 * @param data The bytes.
		 * @return True when the key was opened; the result of the write itself is not checked
		 *         (as before).
		 * @throws std::length_error When the data is larger than a registry value can hold.
		 */
		bool SetData(const std::wstring& section, const std::wstring& name, std::span<const std::byte> data) override;

		/**
		 * @brief Deletes a sub key with everything below it; a missing sub key is no error.
		 * @param section The sub key; must not be empty.
		 * @throws std::invalid_argument When @p section is empty.
		 * @throws std::runtime_error When the sub key exists and cannot be deleted.
		 */
		void DeleteSection(const std::wstring& section) override;

	private:
		/** @brief An open registry key, closed when it goes out of scope; null when not open. */
		using KeyHandle = std::unique_ptr<std::remove_pointer_t<HKEY>, decltype(&::RegCloseKey)>;

		/**
		 * @brief The path of a section's key under HKEY_CURRENT_USER.
		 * @param section The sub key; empty for the root key.
		 * @return The root path, or the root path, a backslash and @p section.
		 */
		std::wstring KeyPath(const std::wstring& section) const;

		/**
		 * @brief Opens a section's key for reading.
		 * @param section The sub key; empty for the root key.
		 * @return The key; null when it does not exist or cannot be opened.
		 */
		KeyHandle OpenForRead(const std::wstring& section) const;

		/**
		 * @brief Opens a section's key for writing, creating it when missing.
		 * @param section The sub key; empty for the root key.
		 * @return The key; null when it cannot be opened or created.
		 */
		KeyHandle OpenForWrite(const std::wstring& section) const;

		/**
		 * @brief GetString's read of an existing value.
		 * @param key The open key.
		 * @param name The value name.
		 * @param byteLength The value's size in bytes, as RegQueryValueEx reports it (more than 0).
		 * @param maxSize The read limit in bytes; negative for none.
		 * @return The text; nothing when the read fails (also when the value does not fit the limit).
		 */
		static std::optional<std::wstring> ReadString(HKEY key, const std::wstring& name, DWORD byteLength, int maxSize);

		/**
		 * @brief The size of a value to write, as the registry API takes it.
		 * @param bytes The size in bytes.
		 * @return @p bytes as a DWORD.
		 * @throws std::length_error When @p bytes does not fit in a DWORD.
		 */
		static DWORD ValueSize(std::size_t bytes);

		/// The root key's path under HKEY_CURRENT_USER.
		std::wstring m_rootPath{};
	};
}
