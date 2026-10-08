/**
 * @file ImageHelperTests.cpp
 * @brief Tests of ImageHelper's CImage loading from clipboard data.
 */
#include "stdafx.h"
#include "ImageHelper.h"

#include <gtest/gtest.h>

#include <cstring>
#include <memory>
#include <new>
#include <vector>

/**
 * @brief A movable global memory block that the test owns and frees.
 */
class TestGlobalBlock
{
public:
	/**
	 * @brief Copies bytes into a new movable global block.
	 * @param bytes The data.
	 */
	explicit TestGlobalBlock(const std::vector<BYTE>& bytes)
		: m_hGlobal(::GlobalAlloc(GMEM_MOVEABLE, bytes.size()))
	{
		if (m_hGlobal == NULL)
		{
			throw std::bad_alloc();
		}
		void* const pData{ ::GlobalLock(m_hGlobal) };
		if (pData == nullptr)
		{
			::GlobalFree(m_hGlobal);
			throw std::bad_alloc();
		}
		std::memcpy(pData, bytes.data(), bytes.size());
		::GlobalUnlock(m_hGlobal);
	}

	TestGlobalBlock(const TestGlobalBlock&) = delete;
	TestGlobalBlock& operator=(const TestGlobalBlock&) = delete;

	/** @brief Frees the block. */
	~TestGlobalBlock()
	{
		::GlobalFree(m_hGlobal);
	}

	/**
	 * @brief The block.
	 * @return The handle (still owned by this object).
	 */
	HGLOBAL Handle() const
	{
		return m_hGlobal;
	}

private:
	/** @brief The owned block. */
	HGLOBAL m_hGlobal{};
};

/**
 * @brief Encodes a small image as PNG bytes.
 */
class TestPng
{
public:
	/**
	 * @brief The PNG bytes of a 2x3 pixel image.
	 * @return The file bytes.
	 */
	static std::vector<BYTE> Bytes()
	{
		CImage image;
		EXPECT_TRUE(image.Create(2, 3, 24));

		CComPtr<IStream> stream;
		EXPECT_EQ(::CreateStreamOnHGlobal(NULL, TRUE, &stream), S_OK);
		EXPECT_EQ(image.Save(stream, Gdiplus::ImageFormatPNG), S_OK);

		STATSTG stat{};
		EXPECT_EQ(stream->Stat(&stat, STATFLAG_NONAME), S_OK);
		HGLOBAL hStream{};
		EXPECT_EQ(::GetHGlobalFromStream(stream, &hStream), S_OK);

		const size_t size{ static_cast<size_t>(stat.cbSize.QuadPart) };
		std::vector<BYTE> bytes(size);
		const void* const pData{ ::GlobalLock(hStream) };
		if (pData == nullptr)
		{
			ADD_FAILURE() << "the PNG stream's memory cannot be locked";
			return {};
		}
		std::memcpy(bytes.data(), pData, size);
		::GlobalUnlock(hStream);
		return bytes;
	}
};

TEST(ImageHelper, LoadsAPngImage)
{
	const TestGlobalBlock block(TestPng::Bytes());

	const std::shared_ptr<CImage> image{ PNGImageHelper::CImageFromHGLOBAL(block.Handle()) };

	ASSERT_NE(image, nullptr);
	EXPECT_EQ(image->GetWidth(), 2);
	EXPECT_EQ(image->GetHeight(), 3);
}

// Regression: CImageFromHGLOBAL ignored CImage::Load's result and returned an empty image for data
// that is not an image, so the caller could not tell the failure.
TEST(ImageHelper, DataThatIsNoImageGivesNoImage)
{
	const std::vector<BYTE> notAnImage{ 'n', 'o', 't', ' ', 'a', 'n', ' ', 'i', 'm', 'a', 'g', 'e' };
	const TestGlobalBlock block(notAnImage);

	EXPECT_EQ(PNGImageHelper::CImageFromHGLOBAL(block.Handle()), nullptr);
}
