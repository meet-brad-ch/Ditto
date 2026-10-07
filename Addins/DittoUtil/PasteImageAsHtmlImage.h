#pragma once

#include "..\..\Shared\DittoDefines.h"
#include "..\..\Shared\IClip.h"

#include <cstddef>
#include <span>


class CPasteImageAsHtmlImage
{
public:
	CPasteImageAsHtmlImage(void);
	~CPasteImageAsHtmlImage(void);

	bool ConvertPathToHtmlImageTag(const CDittoInfo &DittoInfo, IClip *pClip);
	static bool CleanupPastedImages();

private:
	// Saves the clip's CF_DIB as the next .bmp in the image folder and sets csIMG to its <IMG>
	// tag; shows a message box and returns false when the image is malformed or not saved.
	static bool DibImageTag(HWND owner, IClipFormat* pCF_DIB, CString& csIMG);
	// Saves a CF_DIB as a .bmp file; throws DittoCore::ClipboardFormatError when the DIB is
	// malformed, returns false with errorMessage set when the file cannot be created.
	static bool WriteDibToFile(const CString& csPath, std::span<const std::byte> dib, CString& errorMessage);
	static void CreateLocalPath(bool bCreateDir);
};
