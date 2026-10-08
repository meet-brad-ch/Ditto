/**
 * @file RegistrySettingsStore.cpp
 * @brief Implements DittoCore::RegistrySettingsStore.
 */
#include "RegistrySettingsStore.h"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace DittoCore
{
	RegistrySettingsStore::RegistrySettingsStore(std::wstring rootPath)
		: m_rootPath(std::move(rootPath))
	{
	}

	long RegistrySettingsStore::GetLong(const std::wstring& section, const std::wstring& name, long defaultValue) const
	{
		const KeyHandle key{ OpenForRead(section) };
		if (!key)
		{
			return defaultValue;
		}

		DWORD value{};
		DWORD length{ sizeof(value) };
		if (::RegQueryValueExW(key.get(), name.c_str(), nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &length) != ERROR_SUCCESS)
		{
			return defaultValue;
		}
		return static_cast<long>(value);
	}

	bool RegistrySettingsStore::SetLong(const std::wstring& section, const std::wstring& name, long value)
	{
		const KeyHandle key{ OpenForWrite(section) };
		if (!key)
		{
			return false;
		}

		const DWORD bits{ static_cast<DWORD>(value) };
		return ::RegSetValueExW(key.get(), name.c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&bits), sizeof(bits)) == ERROR_SUCCESS;
	}

	std::wstring RegistrySettingsStore::GetString(const std::wstring& section, const std::wstring& name, const std::wstring& defaultValue, int maxSize) const
	{
		const KeyHandle key{ OpenForRead(section) };
		if (!key)
		{
			return defaultValue;
		}

		DWORD byteLength{};
		if (::RegQueryValueExW(key.get(), name.c_str(), nullptr, nullptr, nullptr, &byteLength) != ERROR_SUCCESS)
		{
			return defaultValue;
		}
		if (byteLength == 0)
		{
			return {};
		}
		return ReadString(key.get(), name, byteLength, maxSize).value_or(defaultValue);
	}

	std::optional<std::wstring> RegistrySettingsStore::ReadString(HKEY key, const std::wstring& name, DWORD byteLength, int maxSize)
	{
		// The limit is compared with the size in bytes, as CGetSetOptions did; a value that needs
		// more room than the limit allows fails with ERROR_MORE_DATA below.
		if (maxSize > NoMaxSize && static_cast<DWORD>(maxSize) < byteLength)
		{
			byteLength = static_cast<DWORD>(maxSize);
		}
		byteLength++;

		// one wchar_t per byte: twice the room the read may fill, zero filled, so always terminated
		std::vector<wchar_t> buffer(byteLength);
		if (::RegQueryValueExW(key, name.c_str(), nullptr, nullptr, reinterpret_cast<LPBYTE>(buffer.data()), &byteLength) != ERROR_SUCCESS)
		{
			return std::nullopt;
		}
		return std::wstring(buffer.data());
	}

	bool RegistrySettingsStore::SetString(const std::wstring& section, const std::wstring& name, const std::wstring& value)
	{
		const DWORD size{ ValueSize(value.size() * sizeof(wchar_t)) };
		const KeyHandle key{ OpenForWrite(section) };
		if (!key)
		{
			return false;
		}

		// todo (kept from CGetSetOptions): the write's own result is ignored
		static_cast<void>(::RegSetValueExW(key.get(), name.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), size));
		return true;
	}

	std::vector<std::byte> RegistrySettingsStore::GetData(const std::wstring& section, const std::wstring& name) const
	{
		const KeyHandle key{ OpenForRead(section) };
		if (!key)
		{
			return {};
		}

		DWORD length{};
		if (::RegQueryValueExW(key.get(), name.c_str(), nullptr, nullptr, nullptr, &length) != ERROR_SUCCESS)
		{
			return {};
		}

		std::vector<std::byte> data(length);
		if (::RegQueryValueExW(key.get(), name.c_str(), nullptr, nullptr, reinterpret_cast<LPBYTE>(data.data()), &length) != ERROR_SUCCESS)
		{
			return {};
		}
		data.resize(length);
		return data;
	}

	bool RegistrySettingsStore::SetData(const std::wstring& section, const std::wstring& name, std::span<const std::byte> data)
	{
		const DWORD size{ ValueSize(data.size()) };
		const KeyHandle key{ OpenForWrite(section) };
		if (!key)
		{
			return false;
		}

		// todo (kept from CGetSetOptions): the write's own result is ignored
		static_cast<void>(::RegSetValueExW(key.get(), name.c_str(), 0, REG_BINARY, reinterpret_cast<const BYTE*>(data.data()), size));
		return true;
	}

	void RegistrySettingsStore::DeleteSection(const std::wstring& section)
	{
		if (section.empty())
		{
			throw std::invalid_argument("the default settings section cannot be deleted");
		}

		const LSTATUS result{ ::RegDeleteTreeW(HKEY_CURRENT_USER, KeyPath(section).c_str()) };
		if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND)
		{
			throw std::runtime_error("the settings registry key could not be deleted (error " + std::to_string(result) + ")");
		}
	}

	std::wstring RegistrySettingsStore::KeyPath(const std::wstring& section) const
	{
		if (section.empty())
		{
			return m_rootPath;
		}
		return m_rootPath + L"\\" + section;
	}

	RegistrySettingsStore::KeyHandle RegistrySettingsStore::OpenForRead(const std::wstring& section) const
	{
		HKEY key{};
		if (::RegOpenKeyExW(HKEY_CURRENT_USER, KeyPath(section).c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
		{
			return KeyHandle(nullptr, &::RegCloseKey);
		}
		return KeyHandle(key, &::RegCloseKey);
	}

	RegistrySettingsStore::KeyHandle RegistrySettingsStore::OpenForWrite(const std::wstring& section) const
	{
		HKEY key{};
		DWORD disposition{};
		if (::RegCreateKeyExW(HKEY_CURRENT_USER, KeyPath(section).c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &key, &disposition) != ERROR_SUCCESS)
		{
			return KeyHandle(nullptr, &::RegCloseKey);
		}
		return KeyHandle(key, &::RegCloseKey);
	}

	DWORD RegistrySettingsStore::ValueSize(std::size_t bytes)
	{
		if (bytes > (std::numeric_limits<DWORD>::max)())
		{
			throw std::length_error("a settings value is too large for the registry");
		}
		return static_cast<DWORD>(bytes);
	}
}
