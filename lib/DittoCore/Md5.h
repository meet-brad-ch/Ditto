/**
 * @file Md5.h
 * @brief Declares DittoCore::Md5.
 */
#pragma once

#include <cstddef>
#include <span>
#include <string>

namespace DittoCore
{
	/**
	 * @brief MD5 of a block of bytes through Windows' CNG (BCrypt), as the 32 upper-case hex
	 *        digits Ditto stores with saved file contents and shows in the clip properties.
	 *
	 * MD5 is used here only to check that stored file contents were not damaged, not for security.
	 */
	class Md5
	{
	public:
		/**
		 * @brief The MD5 of the bytes in upper-case hex.
		 * @param bytes The data; may be empty.
		 * @return 32 hex digits, upper case.
		 * @throws std::runtime_error When CNG fails.
		 */
		static std::string Hex(std::span<const std::byte> bytes);

	private:
		/**
		 * @brief Turns a failed CNG call into an exception.
		 * @param status The NTSTATUS the call returned.
		 * @param call The name of the call, for the message.
		 * @throws std::runtime_error When status is a failure.
		 */
		static void Check(long status, const char* call);
	};
}
