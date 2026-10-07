/**
 * @file GzipStream.cpp
 * @brief Implements DittoCore::GzipStream.
 */
#include "GzipStream.h"

#include <zlib.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace DittoCore
{
	void GzipStream::Compress(std::istream& in, std::ostream& out, const Progress& progress)
	{
		z_stream stream{};
		const int initResult = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, GzipWindowBits, MemoryLevel, Z_DEFAULT_STRATEGY);
		if (initResult != Z_OK)
		{
			throw std::runtime_error("zlib could not start compressing (error " + std::to_string(initResult) + ")");
		}
		const std::unique_ptr<z_stream, decltype(&deflateEnd)> streamEnd{ &stream, &deflateEnd };

		std::vector<char> chunk(ChunkSize);
		std::uint64_t bytesDone{};
		int flush{ Z_NO_FLUSH };
		while (flush != Z_FINISH)
		{
			const std::size_t count = ReadChunk(in, chunk.data());
			flush = count == 0 ? Z_FINISH : Z_NO_FLUSH;
			stream.next_in = reinterpret_cast<Bytef*>(chunk.data());
			stream.avail_in = static_cast<uInt>(count);
			DeflateInput(stream, flush, out);
			bytesDone += count;
			if (progress && count > 0)
			{
				progress(bytesDone);
			}
		}
	}

	void GzipStream::Uncompress(std::istream& in, std::ostream& out)
	{
		z_stream stream{};
		const int initResult = inflateInit2(&stream, GzipWindowBits);
		if (initResult != Z_OK)
		{
			throw std::runtime_error("zlib could not start uncompressing (error " + std::to_string(initResult) + ")");
		}
		const std::unique_ptr<z_stream, decltype(&inflateEnd)> streamEnd{ &stream, &inflateEnd };

		std::vector<char> chunk(ChunkSize);
		bool ended{};
		while (!ended)
		{
			const std::size_t count = ReadChunk(in, chunk.data());
			if (count == 0)
			{
				throw std::runtime_error("the data ends before its gzip stream does");
			}
			stream.next_in = reinterpret_cast<Bytef*>(chunk.data());
			stream.avail_in = static_cast<uInt>(count);
			ended = InflateInput(stream, out);
		}
	}

	std::size_t GzipStream::ReadChunk(std::istream& in, char* chunk)
	{
		in.read(chunk, static_cast<std::streamsize>(ChunkSize));
		if (in.bad())
		{
			throw std::runtime_error("reading the data failed");
		}
		return static_cast<std::size_t>(in.gcount());
	}

	void GzipStream::Write(std::ostream& out, const char* bytes, std::size_t count)
	{
		out.write(bytes, static_cast<std::streamsize>(count));
		if (!out)
		{
			throw std::runtime_error("writing the result failed");
		}
	}

	void GzipStream::DeflateInput(z_stream_s& stream, int flush, std::ostream& out)
	{
		std::vector<char> chunk(ChunkSize);
		do
		{
			stream.next_out = reinterpret_cast<Bytef*>(chunk.data());
			stream.avail_out = static_cast<uInt>(ChunkSize);
			const int result = deflate(&stream, flush);
			if (result == Z_STREAM_ERROR)
			{
				throw std::runtime_error("zlib could not compress the data");
			}
			Write(out, chunk.data(), ChunkSize - stream.avail_out);
		} while (stream.avail_out == 0);
	}

	bool GzipStream::InflateInput(z_stream_s& stream, std::ostream& out)
	{
		std::vector<char> chunk(ChunkSize);
		int result{ Z_OK };
		do
		{
			stream.next_out = reinterpret_cast<Bytef*>(chunk.data());
			stream.avail_out = static_cast<uInt>(ChunkSize);
			result = inflate(&stream, Z_NO_FLUSH);
			// Z_BUF_ERROR only says no progress was possible yet; every other code is fatal
			if (result != Z_OK && result != Z_STREAM_END && result != Z_BUF_ERROR)
			{
				throw std::runtime_error("the data is not valid gzip (zlib error " + std::to_string(result) + ")");
			}
			Write(out, chunk.data(), ChunkSize - stream.avail_out);
		} while (stream.avail_out == 0 && result != Z_STREAM_END);
		return result == Z_STREAM_END;
	}
}
