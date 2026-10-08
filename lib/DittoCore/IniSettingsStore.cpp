/**
 * @file IniSettingsStore.cpp
 * @brief Implements DittoCore::IniSettingsStore.
 */
#include "IniSettingsStore.h"

#include <stdexcept>
#include <string>
#include <utility>

#include <windows.h>

namespace DittoCore
{
	IniSettingsStore::IniSettingsStore(std::wstring filePath) :
		m_filePath(std::move(filePath))
	{
	}

	long IniSettingsStore::GetLong(const std::wstring& section, const std::wstring& name, long defaultValue) const
	{
		// GetPrivateProfileInt returns the 32-bit pattern of a negative value as UINT
		return static_cast<long>(::GetPrivateProfileIntW(SectionName(section), name.c_str(), static_cast<INT>(defaultValue), m_filePath.c_str()));
	}

	bool IniSettingsStore::SetLong(const std::wstring& section, const std::wstring& name, long value)
	{
		return ::WritePrivateProfileStringW(SectionName(section), name.c_str(), std::to_wstring(value).c_str(), m_filePath.c_str()) != FALSE;
	}

	std::wstring IniSettingsStore::GetString(const std::wstring& section, const std::wstring& name, const std::wstring& defaultValue, int maxSize) const
	{
		if (maxSize == 0)
		{
			return {};
		}

		DWORD bufferLength{ InitialBufferLength };
		for (;;)
		{
			const bool limited{ maxSize > NoMaxSize && static_cast<DWORD>(maxSize) < bufferLength };
			if (limited)
			{
				bufferLength = static_cast<DWORD>(maxSize);
			}

			std::vector<wchar_t> buffer(bufferLength);
			const DWORD readLength{ ::GetPrivateProfileStringW(SectionName(section), name.c_str(), defaultValue.c_str(), buffer.data(), bufferLength, m_filePath.c_str()) };
			// a read that filled the buffer may have been cut: read again with twice the room
			if (limited || readLength < bufferLength - 1)
			{
				return std::wstring(buffer.data());
			}
			bufferLength *= 2;
		}
	}

	bool IniSettingsStore::SetString(const std::wstring& section, const std::wstring& name, const std::wstring& value)
	{
		return ::WritePrivateProfileStringW(SectionName(section), name.c_str(), value.c_str(), m_filePath.c_str()) != FALSE;
	}

	std::vector<std::byte> IniSettingsStore::GetData([[maybe_unused]] const std::wstring& section, [[maybe_unused]] const std::wstring& name) const
	{
		throw std::logic_error("binary settings values are not supported in the ini file");
	}

	bool IniSettingsStore::SetData([[maybe_unused]] const std::wstring& section, [[maybe_unused]] const std::wstring& name, [[maybe_unused]] std::span<const std::byte> data)
	{
		throw std::logic_error("binary settings values are not supported in the ini file");
	}

	void IniSettingsStore::DeleteSection(const std::wstring& section)
	{
		if (section.empty())
		{
			throw std::invalid_argument("the default settings section cannot be deleted");
		}

		if (!::WritePrivateProfileStringW(section.c_str(), nullptr, nullptr, m_filePath.c_str()))
		{
			throw std::runtime_error("the settings section could not be removed from the ini file (error " + std::to_string(::GetLastError()) + ")");
		}
	}

	const wchar_t* IniSettingsStore::SectionName(const std::wstring& section)
	{
		return section.empty() ? DefaultSection : section.c_str();
	}
}
