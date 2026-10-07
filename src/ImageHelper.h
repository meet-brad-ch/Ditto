#include "stdafx.h"
#include "Misc.h"
#include "GlobalBytes.h"
#include <memory>
#include <span>

template <class Concrete, class GdipImage>
class ImageHelper abstract
{
public:
	static GdipImage* GdipImageFromHGLOBAL(HGLOBAL hGlobal) {
		CComPtr<IStream> stream = StreamFromHGLOBAL(hGlobal);
		if (!stream)
			return NULL;

		return GdipImage::FromStream(stream);
	};
	static std::shared_ptr<CImage> CImageFromHGLOBAL(HGLOBAL hGlobal) {
		CComPtr<IStream> stream = StreamFromHGLOBAL(hGlobal);
		if (!stream)
			return NULL;

		std::shared_ptr<CImage> cImage = std::make_shared<CImage>();
		cImage->Load(stream);

		return cImage;
	};
	// Returns NULL for a null handle or when the stream cannot be written; throws
	// DittoCore::ClipboardFormatError when the image data is malformed.
	static CComPtr<IStream> StreamFromHGLOBAL(HGLOBAL hGlobal) {
		if (!hGlobal)
			return NULL;

		const DittoCore::GlobalBytes block(hGlobal);
		const std::span<const std::byte> bytes = block.Bytes();

		CComPtr<IStream> stream;
		if (CreateStreamOnHGlobal(NULL, TRUE, &stream) != S_OK)
			return NULL;

		if (!Concrete::prependStream(stream, bytes))
			return NULL;

		if (stream->Write(bytes.data(), static_cast<ULONG>(bytes.size()), NULL) != S_OK)
			return NULL;

		return stream;
	};
};

class BitmapImageHelper : public ImageHelper<BitmapImageHelper, Gdiplus::Bitmap>
{
public:
	static bool prependStream(IStream*, std::span<const std::byte>) { return true; };
};

class PNGImageHelper : public BitmapImageHelper
{
};

class DIBImageHelper : public ImageHelper<DIBImageHelper, Gdiplus::Bitmap>
{
public:
	// Writes the BITMAPFILEHEADER that turns the DIB into a .bmp stream; throws
	// DittoCore::ClipboardFormatError when the DIB is malformed.
	static bool prependStream(IStream* pIStream, std::span<const std::byte> dib);
};
