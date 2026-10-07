/**
 * @file GzipStream.h
 * @brief Declares DittoCore::GzipStream.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <istream>
#include <ostream>

struct z_stream_s;

namespace DittoCore
{
	/**
	 * @brief Compresses and uncompresses whole streams in gzip format: the format of Ditto's
	 *        database backups (`.zdb`).
	 *
	 * Both directions work in chunks, so a database of any size needs only two chunk buffers.
	 * A failure throws: nothing is reported as done after a read, write or zlib error.
	 */
	class GzipStream
	{
	public:
		/// Called after each chunk with the number of input bytes compressed so far.
		using Progress = std::function<void(std::uint64_t bytesDone)>;

		/**
		 * @brief Compresses all of @p in into one gzip member written to @p out.
		 * @param in The data, read to its end.
		 * @param out Receives the gzip stream.
		 * @param progress Called after each chunk; may be empty.
		 * @throws std::runtime_error When reading @p in or writing @p out fails, or zlib fails.
		 */
		static void Compress(std::istream& in, std::ostream& out, const Progress& progress);

		/**
		 * @brief Uncompresses one gzip member from @p in into @p out.
		 * @param in The gzip stream.
		 * @param out Receives the original data.
		 * @throws std::runtime_error When @p in is not gzip data, ends before the gzip member
		 *         does, reading or writing fails, or zlib fails.
		 */
		static void Uncompress(std::istream& in, std::ostream& out);

	private:
		/// Bytes read or written per step.
		static constexpr std::size_t ChunkSize{ 65536 };
		/// zlib window bits for a gzip (not zlib) header and trailer: 15 + 16.
		static constexpr int GzipWindowBits{ 31 };
		/// zlib's default memory level for deflate.
		static constexpr int MemoryLevel{ 8 };

		/**
		 * @brief Reads up to one chunk from @p in.
		 * @param in The stream.
		 * @param chunk Receives the bytes; holds ChunkSize bytes.
		 * @return The number of bytes read; 0 at the end of @p in.
		 * @throws std::runtime_error When reading fails.
		 */
		static std::size_t ReadChunk(std::istream& in, char* chunk);

		/**
		 * @brief Writes bytes to @p out.
		 * @param out The stream.
		 * @param bytes The bytes.
		 * @param count How many.
		 * @throws std::runtime_error When writing fails.
		 */
		static void Write(std::ostream& out, const char* bytes, std::size_t count);

		/**
		 * @brief Deflates the stream's pending input into @p out until zlib needs more input.
		 * @param stream The deflate stream, with its input set.
		 * @param flush Z_NO_FLUSH, or Z_FINISH for the last input.
		 * @param out Receives the compressed bytes.
		 * @throws std::runtime_error When zlib fails or writing fails.
		 */
		static void DeflateInput(z_stream_s& stream, int flush, std::ostream& out);

		/**
		 * @brief Inflates the stream's pending input into @p out until zlib needs more input.
		 * @param stream The inflate stream, with its input set.
		 * @param out Receives the uncompressed bytes.
		 * @return Whether the gzip member ended.
		 * @throws std::runtime_error When the data is not valid gzip, zlib fails or writing fails.
		 */
		static bool InflateInput(z_stream_s& stream, std::ostream& out);
	};
}
