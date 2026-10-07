#pragma once

#include "..\..\Shared\DittoDefines.h"
#include "..\..\Shared\IClip.h"

#include <array>
#include <cstddef>
#include <span>


class CPasteImageAsHtmlImage
{
public:
	CPasteImageAsHtmlImage(void);
	~CPasteImageAsHtmlImage(void);
	CPasteImageAsHtmlImage(const CPasteImageAsHtmlImage&) = delete;
	CPasteImageAsHtmlImage& operator=(const CPasteImageAsHtmlImage&) = delete;

	bool ConvertPathToHtmlImageTag(const CDittoInfo &DittoInfo, IClip *pClip);
	/**
	 * @brief Deletes the image folder and the images saved to it (when the add-in DLL exits).
	 * @return false (always; the result of removing the folder is ignored).
	 */
	bool CleanupPastedImages();

private:
	/** @brief The folder pasted CF_DIB images are saved to (%TMP%\ditto); empty until first used. */
	CString m_dibImagePath{};
	/** @brief The number of the next saved CF_DIB image file (1.bmp, 2.bmp, ... for the DLL's lifetime). */
	int m_nextDibImageName{1};

	/** @brief File name parts that mark an image file (searched anywhere in the lower-case path). */
	static constexpr std::array<const TCHAR*, 10> m_imageExtensions
	{
		_T(".bmp"), _T(".dib"), _T(".jpg"), _T(".jpeg"), _T(".jpe"),
		_T(".jfif"), _T(".gif"), _T(".tif"), _T(".tiff"), _T(".png")
	};

	/**
	 * @brief ConvertPathToHtmlImageTag's step: builds the <IMG> tags of the clip's CF_DIB, or else
	 * of the image files in its CF_HDROP.
	 * @param owner the Ditto window, owner of error message boxes.
	 * @param pFormats the clip formats.
	 * @param csIMG receives the tags (unchanged when the clip has neither format).
	 * @return false if the image or file list is malformed or not saved (a message box was shown).
	 */
	bool GetImageTags(HWND owner, IClipFormats *pFormats, CString& csIMG);
	/**
	 * @brief Appends an <IMG> tag (with <br> between them) for every image file of a CF_HDROP.
	 * @param owner the Ditto window, owner of the error message box.
	 * @param pHDrop the CF_HDROP format.
	 * @param csIMG the tags to append to.
	 * @return false if the file list is malformed (a message box was shown).
	 */
	static bool HDropImageTags(HWND owner, IClipFormat* pHDrop, CString& csIMG);
	/**
	 * @brief Tells whether a path names an image file.
	 * @param csFile the lower-case path.
	 * @return true if the path contains one of m_imageExtensions.
	 */
	static bool IsImageFile(const CString& csFile);
	// Saves the clip's CF_DIB as the next .bmp in the image folder and sets csIMG to its <IMG>
	// tag; shows a message box and returns false when the image is malformed or not saved.
	bool DibImageTag(HWND owner, IClipFormat* pCF_DIB, CString& csIMG);
	// Saves a CF_DIB as a .bmp file; throws DittoCore::ClipboardFormatError when the DIB is
	// malformed, returns false with errorMessage set when the file cannot be created.
	static bool WriteDibToFile(const CString& csPath, std::span<const std::byte> dib, CString& errorMessage);
	/**
	 * @brief Sets the image folder to %TMP%\ditto.
	 * @param bCreateDir true to also create the folder.
	 */
	void CreateLocalPath(bool bCreateDir);
};
