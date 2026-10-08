/**
 * @file InMemorySettingsStore.h
 * @brief Declares DittoCore::InMemorySettingsStore.
 */
#pragma once

#include "ISettingsStore.h"

#include <compare>
#include <map>
#include <variant>

namespace DittoCore
{
	/**
	 * @brief Settings held in memory, for tests of code that reads and writes settings.
	 *
	 * Every value keeps the type it was written with: a number read as a text (or the other way
	 * round) reads as missing. Section and value names are case-sensitive here (the registry and
	 * ini file are not), and the empty section is a section of its own. GetString cuts texts to a
	 * size limit as IniSettingsStore does.
	 */
	class InMemorySettingsStore final : public ISettingsStore
	{
	public:
		/**
		 * @brief Reads a number.
		 * @param section The section.
		 * @param name The value name.
		 * @param defaultValue The result when no number is stored under the name.
		 * @return The number, or @p defaultValue.
		 */
		long GetLong(const std::wstring& section, const std::wstring& name, long defaultValue) const override;

		/**
		 * @brief Stores a number, replacing any value of the name.
		 * @param section The section.
		 * @param name The value name.
		 * @param value The value.
		 * @return True.
		 */
		bool SetLong(const std::wstring& section, const std::wstring& name, long value) override;

		/**
		 * @brief Reads a text.
		 * @param section The section.
		 * @param name The value name.
		 * @param defaultValue The result when no text is stored under the name.
		 * @param maxSize The limit in characters: the result (also the default) is cut to
		 *        maxSize - 1 characters; 0 gives an empty text. Negative (NoMaxSize) for no limit.
		 * @return The text, or @p defaultValue.
		 */
		std::wstring GetString(const std::wstring& section, const std::wstring& name, const std::wstring& defaultValue, int maxSize) const override;

		/**
		 * @brief Stores a text, replacing any value of the name.
		 * @param section The section.
		 * @param name The value name.
		 * @param value The text.
		 * @return True.
		 */
		bool SetString(const std::wstring& section, const std::wstring& name, const std::wstring& value) override;

		/**
		 * @brief Reads a binary value.
		 * @param section The section.
		 * @param name The value name.
		 * @return The bytes; empty when no binary value is stored under the name.
		 */
		std::vector<std::byte> GetData(const std::wstring& section, const std::wstring& name) const override;

		/**
		 * @brief Stores a binary value, replacing any value of the name.
		 * @param section The section.
		 * @param name The value name.
		 * @param data The bytes.
		 * @return True.
		 */
		bool SetData(const std::wstring& section, const std::wstring& name, std::span<const std::byte> data) override;

		/**
		 * @brief Removes every value of a section.
		 * @param section The section; must not be empty.
		 * @throws std::invalid_argument When @p section is empty.
		 */
		void DeleteSection(const std::wstring& section) override;

	private:
		/** @brief Where a value is stored. */
		struct ValueKey
		{
			/** @brief The section. */
			std::wstring section{};
			/** @brief The value name. */
			std::wstring name{};

			/**
			 * @brief Orders keys by section, then name.
			 * @param other The key compared with.
			 * @return The ordering.
			 */
			auto operator<=>(const ValueKey& other) const = default;
		};

		/** @brief A stored value: a number, a text or bytes. */
		using Value = std::variant<long, std::wstring, std::vector<std::byte>>;

		/**
		 * @brief The value stored under a name, when it has the type asked for.
		 * @tparam T The type: long, std::wstring or std::vector<std::byte>.
		 * @param section The section.
		 * @param name The value name.
		 * @return The value; null when nothing of type T is stored under the name.
		 */
		template <typename T>
		const T* Find(const std::wstring& section, const std::wstring& name) const
		{
			const auto found{ m_values.find(ValueKey{ section, name }) };
			return found == m_values.end() ? nullptr : std::get_if<T>(&found->second);
		}

		/// The stored values.
		std::map<ValueKey, Value> m_values{};
	};
}
