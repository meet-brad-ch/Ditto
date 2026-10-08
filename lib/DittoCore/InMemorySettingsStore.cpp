/**
 * @file InMemorySettingsStore.cpp
 * @brief Implements DittoCore::InMemorySettingsStore.
 */
#include "InMemorySettingsStore.h"

#include <cstddef>
#include <stdexcept>

namespace DittoCore
{
	long InMemorySettingsStore::GetLong(const std::wstring& section, const std::wstring& name, long defaultValue) const
	{
		const long* value{ Find<long>(section, name) };
		return value == nullptr ? defaultValue : *value;
	}

	bool InMemorySettingsStore::SetLong(const std::wstring& section, const std::wstring& name, long value)
	{
		m_values[ValueKey{ section, name }] = value;
		return true;
	}

	std::wstring InMemorySettingsStore::GetString(const std::wstring& section, const std::wstring& name, const std::wstring& defaultValue, int maxSize) const
	{
		const std::wstring* value{ Find<std::wstring>(section, name) };
		std::wstring text{ value == nullptr ? defaultValue : *value };
		if (maxSize > NoMaxSize && text.size() >= static_cast<std::size_t>(maxSize))
		{
			// as the ini file: a buffer of maxSize characters holds maxSize - 1 and the terminator
			text.resize(maxSize == 0 ? 0 : static_cast<std::size_t>(maxSize) - 1);
		}
		return text;
	}

	bool InMemorySettingsStore::SetString(const std::wstring& section, const std::wstring& name, const std::wstring& value)
	{
		m_values[ValueKey{ section, name }] = value;
		return true;
	}

	std::vector<std::byte> InMemorySettingsStore::GetData(const std::wstring& section, const std::wstring& name) const
	{
		const std::vector<std::byte>* value{ Find<std::vector<std::byte>>(section, name) };
		return value == nullptr ? std::vector<std::byte>{} : *value;
	}

	bool InMemorySettingsStore::SetData(const std::wstring& section, const std::wstring& name, std::span<const std::byte> data)
	{
		m_values[ValueKey{ section, name }] = std::vector<std::byte>(data.begin(), data.end());
		return true;
	}

	void InMemorySettingsStore::DeleteSection(const std::wstring& section)
	{
		if (section.empty())
		{
			throw std::invalid_argument("the default settings section cannot be deleted");
		}

		std::erase_if(m_values, [&section](const auto& entry)
					  { return entry.first.section == section; });
	}
}
