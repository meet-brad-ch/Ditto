/**
 * @file GzipStreamTests.cpp
 * @brief Unit tests for DittoCore::GzipStream.
 */
#include "GzipStream.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using DittoCore::GzipStream;

namespace
{
	std::string Compressed(const std::string& data)
	{
		std::istringstream in(data);
		std::ostringstream out;
		GzipStream::Compress(in, out, {});
		return out.str();
	}

	std::string Uncompressed(const std::string& gzip)
	{
		std::istringstream in(gzip);
		std::ostringstream out;
		GzipStream::Uncompress(in, out);
		return out.str();
	}

	// Several chunks of data that compress, with every byte value present
	std::string LargeData()
	{
		std::string data(3 * 65536 + 17, '\0');
		for (std::size_t i = 0; i < data.size(); i++)
		{
			data[i] = static_cast<char>((i * 7919) % 251);
		}
		return data;
	}
}

TEST(GzipStream, RoundTripsEmptyData)
{
	EXPECT_EQ(Uncompressed(Compressed("")), "");
}

TEST(GzipStream, RoundTripsDataOfSeveralChunks)
{
	const std::string data = LargeData();

	EXPECT_EQ(Uncompressed(Compressed(data)), data);
}

// The backups (.zdb) written by upstream's gzwrite are gzip: the stream must be gzip too.
TEST(GzipStream, WritesGzipHeader)
{
	const std::string gzip = Compressed("Ditto");

	ASSERT_GE(gzip.size(), 2u);
	EXPECT_EQ(static_cast<unsigned char>(gzip[0]), 0x1Fu);
	EXPECT_EQ(static_cast<unsigned char>(gzip[1]), 0x8Bu);
}

TEST(GzipStream, ReportsProgressUpToAllBytes)
{
	const std::string data = LargeData();
	std::istringstream in(data);
	std::ostringstream out;
	std::vector<std::uint64_t> reported;

	GzipStream::Compress(in, out, [&reported](std::uint64_t bytesDone)
						 { reported.push_back(bytesDone); });

	ASSERT_EQ(reported.size(), 4u);
	EXPECT_EQ(reported.front(), 65536u);
	EXPECT_EQ(reported.back(), data.size());
}

// Regression: RestoreDB wrote gzread's -1 (a read error) to the file as a byte count.
TEST(GzipStream, RejectsDataThatIsNotGzip)
{
	EXPECT_THROW(Uncompressed("this is not a gzip stream"), std::runtime_error);
}

TEST(GzipStream, RejectsTruncatedStream)
{
	const std::string gzip = Compressed(LargeData());

	EXPECT_THROW(Uncompressed(gzip.substr(0, gzip.size() / 2)), std::runtime_error);
}

TEST(GzipStream, RejectsEmptyInput)
{
	EXPECT_THROW(Uncompressed(""), std::runtime_error);
}

TEST(GzipStream, RejectsCorruptedStream)
{
	std::string gzip = Compressed(LargeData());
	gzip[gzip.size() / 2] = static_cast<char>(gzip[gzip.size() / 2] ^ 0x55);

	EXPECT_THROW(Uncompressed(gzip), std::runtime_error);
}

TEST(GzipStream, FailsWhenTheOutputCannotBeWritten)
{
	std::istringstream in("Ditto");
	std::ostringstream out;
	out.setstate(std::ios::badbit);

	EXPECT_THROW(GzipStream::Compress(in, out, {}), std::runtime_error);
}
