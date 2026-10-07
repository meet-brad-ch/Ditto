#include "stdafx.h"
#include "ImageHelper.h"
#include "DibHeader.h"

bool DIBImageHelper::prependStream(IStream* pIStream, std::span<const std::byte> dib)
{
	const auto header = DittoCore::DibHeader::FileHeader(DittoCore::DibHeader::Read(dib), dib.size());
	return pIStream->Write(header.data(), static_cast<ULONG>(header.size()), NULL) == S_OK;
}
