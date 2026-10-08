/**
 * @file ISettingsStore.h
 * @brief Declares DittoCore::ISettingsStore.
 */
#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace DittoCore
{
	/**
	 * @brief The storage of Ditto's named settings values, grouped in sections.
	 *
	 * An empty section is the default section: the "Ditto" section of the ini file, or the root
	 * key (HKCU\\Software\\Ditto) in the registry. A named section is an ini section or a
	 * registry sub key of the root key. The app uses RegistrySettingsStore or IniSettingsStore;
	 * tests use InMemorySettingsStore.
	 */
	class ISettingsStore
	{
	public:
		/** @brief The maxSize of GetString that reads the whole value. */
		static constexpr int NoMaxSize{ -1 };

		/// Destroys the store.
		virtual ~ISettingsStore() = default;

		/**
		 * @brief Reads a number.
		 * @param section The section; empty for the default section.
		 * @param name The value name.
		 * @param defaultValue The result when the value is missing.
		 * @return The value, or @p defaultValue when it is missing (each store documents its
		 *         other cases).
		 */
		virtual long GetLong(const std::wstring& section, const std::wstring& name, long defaultValue) const = 0;

		/**
		 * @brief Writes a number.
		 * @param section The section; empty for the default section.
		 * @param name The value name.
		 * @param value The value.
		 * @return True when the value was written.
		 */
		virtual bool SetLong(const std::wstring& section, const std::wstring& name, long value) = 0;

		/**
		 * @brief Reads a text.
		 * @param section The section; empty for the default section.
		 * @param name The value name.
		 * @param defaultValue The result when the value is missing.
		 * @param maxSize The size limit of the read (each store documents its unit and effect);
		 *        negative (NoMaxSize) for none.
		 * @return The text, or @p defaultValue when it is missing.
		 */
		virtual std::wstring GetString(const std::wstring& section, const std::wstring& name, const std::wstring& defaultValue, int maxSize) const = 0;

		/**
		 * @brief Writes a text.
		 * @param section The section; empty for the default section.
		 * @param name The value name.
		 * @param value The text.
		 * @return True when the value was written (see each store for the cases it reports).
		 */
		virtual bool SetString(const std::wstring& section, const std::wstring& name, const std::wstring& value) = 0;

		/**
		 * @brief Reads a binary value.
		 * @param section The section; empty for the default section.
		 * @param name The value name.
		 * @return The bytes; empty when the value is missing or cannot be read.
		 */
		virtual std::vector<std::byte> GetData(const std::wstring& section, const std::wstring& name) const = 0;

		/**
		 * @brief Writes a binary value.
		 * @param section The section; empty for the default section.
		 * @param name The value name.
		 * @param data The bytes.
		 * @return True when the value was written (see each store for the cases it reports).
		 */
		virtual bool SetData(const std::wstring& section, const std::wstring& name, std::span<const std::byte> data) = 0;

		/**
		 * @brief Deletes a named section with all its values; a missing section is no error.
		 * @param section The section; must not be empty.
		 * @throws std::invalid_argument When @p section is empty (the default section holds all settings).
		 * @throws std::runtime_error When the section exists and cannot be deleted.
		 */
		virtual void DeleteSection(const std::wstring& section) = 0;
	};
}
