/**
 * @file DtoCodec.h
 * @brief Declares DittoCore::DtoCodec.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace DittoCore
{
	/**
	 * @brief Compresses and uncompresses the clip formats stored in exported `.dto` files (zlib).
	 *
	 * A `.dto` file stores each format's compressed bytes with its original size. Both come from
	 * the file, so the size is untrusted: it is checked against MaxOriginalSize before anything is
	 * allocated, and against the uncompressed result.
	 */
	class DtoCodec
	{
	public:
		/// The largest original size accepted from a file (1 GiB): larger than any clip Ditto keeps.
		static constexpr std::int64_t MaxOriginalSize{ 1LL << 30 };

		/**
		 * @brief Compresses a format's bytes.
		 * @param data The bytes.
		 * @return The zlib stream.
		 * @throws ClipboardFormatError When @p data is larger than MaxOriginalSize or zlib fails.
		 */
		static std::vector<std::byte> Compress(std::span<const std::byte> data);

		/**
		 * @brief Uncompresses a format's bytes.
		 * @param compressed The zlib stream.
		 * @param originalSize The original size the file declares.
		 * @return Exactly @p originalSize bytes.
		 * @throws ClipboardFormatError When @p originalSize is negative, above MaxOriginalSize or
		 *         more than deflate can produce from @p compressed, the stream is not valid zlib
		 *         data, or it uncompresses to another size.
		 */
		static std::vector<std::byte> Uncompress(std::span<const std::byte> compressed, std::int64_t originalSize);

	private:
		/// The most deflate can expand data: each compressed byte yields at most about 1032 bytes.
		static constexpr std::uint64_t MaxDeflateRatio{ 1032 };
		/// Room for the zlib header and the last block of a tiny stream.
		static constexpr std::uint64_t DeflateSlack{ 1024 };
	};
}
