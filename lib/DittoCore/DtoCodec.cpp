/**
 * @file DtoCodec.cpp
 * @brief Implements DittoCore::DtoCodec.
 */
#include "DtoCodec.h"
#include "ClipboardFormatError.h"

#include <zlib.h>

#include <limits>
#include <string>

namespace DittoCore
{
	std::vector<std::byte> DtoCodec::Compress(std::span<const std::byte> data)
	{
		if (data.size() > static_cast<std::size_t>(MaxOriginalSize))
		{
			throw ClipboardFormatError("a format of " + std::to_string(data.size()) + " bytes is too large to export");
		}
		uLongf compressedSize = compressBound(static_cast<uLong>(data.size()));
		std::vector<std::byte> compressed(compressedSize);
		const int result = compress(reinterpret_cast<Bytef*>(compressed.data()), &compressedSize,
			reinterpret_cast<const Bytef*>(data.data()), static_cast<uLong>(data.size()));
		if (result != Z_OK)
		{
			throw ClipboardFormatError("zlib could not compress the format (error " + std::to_string(result) + ")");
		}
		compressed.resize(compressedSize);
		return compressed;
	}

	std::vector<std::byte> DtoCodec::Uncompress(std::span<const std::byte> compressed, std::int64_t originalSize)
	{
		if (originalSize < 0 || originalSize > MaxOriginalSize)
		{
			throw ClipboardFormatError("the file declares an original size of " + std::to_string(originalSize) + " bytes");
		}
		if (compressed.size() > std::numeric_limits<uLong>::max())
		{
			throw ClipboardFormatError("the compressed format of " + std::to_string(compressed.size()) + " bytes is too large");
		}
		// a size deflate cannot reach from this many bytes is a lie; refuse it before allocating
		if (static_cast<std::uint64_t>(originalSize) > compressed.size() * MaxDeflateRatio + DeflateSlack)
		{
			throw ClipboardFormatError("the file declares " + std::to_string(originalSize) + " bytes for " + std::to_string(compressed.size()) + " compressed bytes");
		}
		std::vector<std::byte> data(static_cast<std::size_t>(originalSize));
		uLongf size = static_cast<uLongf>(originalSize);
		// uncompress needs a non-null destination even for an empty format
		Bytef empty{};
		Bytef* destination = data.empty() ? &empty : reinterpret_cast<Bytef*>(data.data());
		const int result = uncompress(destination, &size, reinterpret_cast<const Bytef*>(compressed.data()), static_cast<uLong>(compressed.size()));
		if (result != Z_OK)
		{
			throw ClipboardFormatError("the format is not valid zlib data, or larger than its declared " + std::to_string(originalSize) + " bytes (zlib error " + std::to_string(result) + ")");
		}
		if (size != static_cast<uLongf>(originalSize))
		{
			throw ClipboardFormatError("the format uncompresses to " + std::to_string(size) + " bytes, not the declared " + std::to_string(originalSize));
		}
		return data;
	}
}
