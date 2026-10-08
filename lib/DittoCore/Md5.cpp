/**
 * @file Md5.cpp
 * @brief Implements DittoCore::Md5.
 */
#include "Md5.h"

#include <windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace DittoCore
{
	void Md5::Check(long status, const char* call)
	{
		if (!BCRYPT_SUCCESS(status))
		{
			char code[16]{};
			std::snprintf(code, sizeof(code), "0x%08lX", static_cast<unsigned long>(status));
			throw std::runtime_error(std::string("MD5: ") + call + " failed with NTSTATUS " + code);
		}
	}

	std::string Md5::Hex(std::span<const std::byte> bytes)
	{
		BCRYPT_ALG_HANDLE algorithmHandle{};
		Check(BCryptOpenAlgorithmProvider(&algorithmHandle, BCRYPT_MD5_ALGORITHM, nullptr, 0), "BCryptOpenAlgorithmProvider");
		const auto closeAlgorithm = [](BCRYPT_ALG_HANDLE handle)
		{ BCryptCloseAlgorithmProvider(handle, 0); };
		const std::unique_ptr<std::remove_pointer_t<BCRYPT_ALG_HANDLE>, decltype(closeAlgorithm)> algorithm{ algorithmHandle, closeAlgorithm };

		BCRYPT_HASH_HANDLE hashHandle{};
		Check(BCryptCreateHash(algorithm.get(), &hashHandle, nullptr, 0, nullptr, 0, 0), "BCryptCreateHash");
		const std::unique_ptr<std::remove_pointer_t<BCRYPT_HASH_HANDLE>, decltype(&BCryptDestroyHash)> hash{ hashHandle, &BCryptDestroyHash };

		// BCryptHashData takes the length as ULONG: hash larger blocks in parts
		while (!bytes.empty())
		{
			const std::size_t part{ (std::min)(bytes.size(), static_cast<std::size_t>(0x40000000)) };
			Check(BCryptHashData(hash.get(), reinterpret_cast<PUCHAR>(const_cast<std::byte*>(bytes.data())), static_cast<ULONG>(part), 0), "BCryptHashData");
			bytes = bytes.subspan(part);
		}

		std::array<UCHAR, 16> digest{};
		Check(BCryptFinishHash(hash.get(), digest.data(), static_cast<ULONG>(digest.size()), 0), "BCryptFinishHash");

		static constexpr char digits[]{ "0123456789ABCDEF" };
		std::string hex;
		hex.reserve(digest.size() * 2);
		for (const UCHAR value : digest)
		{
			hex.push_back(digits[value >> 4]);
			hex.push_back(digits[value & 0x0F]);
		}
		return hex;
	}
}
