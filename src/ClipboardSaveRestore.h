#pragma once

#include "Clip.h"

class CAppWindows;
class CRegisteredClipboardFormats;

class CClipboardSaveRestore
{
public:
	/**
	 * @brief Creates an empty saved clipboard.
	 * @param windows The application's windows (the main window opens the clipboard); must outlive this object.
	 * @param formats The registered clipboard formats (the ignore format a restore adds); must outlive this object.
	 */
	CClipboardSaveRestore(CAppWindows& windows, const CRegisteredClipboardFormats& formats);
	~CClipboardSaveRestore(void);

	bool Save(BOOL textOnly);
	bool Restore();
	void Clear()	{ m_Clipboard.RemoveAll(); }
	bool RestoreTextOnly();

	CClipFormats m_Clipboard;

private:
	/** @brief The application's windows (not owned): the main window opens the clipboard. */
	CAppWindows& m_windows;
	/** @brief The registered clipboard formats (not owned). */
	const CRegisteredClipboardFormats& m_formats;

	/**
	 * @brief Tells whether Save keeps a clipboard format.
	 * @param textOnly TRUE to keep only CF_TEXT, CF_UNICODETEXT and CF_HDROP.
	 * @param nFormat the clipboard format.
	 * @return true if the format is saved.
	 */
	static bool IsFormatToSave(BOOL textOnly, UINT nFormat);
	/**
	 * @brief Save's step: copies the data of one format of the open clipboard into m_Clipboard.
	 * @param nFormat the clipboard format.
	 * @param cf the work record reused for every format (its data pointer is NULL again afterwards).
	 */
	void SaveFormat(UINT nFormat, CClipFormat& cf);
	/**
	 * @brief Tells whether a saved format holds data.
	 * @param pCF the saved format, may be NULL.
	 * @return true if pCF has a non-empty HGLOBAL.
	 */
	static bool HasValidData(const CClipFormat *pCF);
	/**
	 * @brief Tells whether a clipboard format is CF_TEXT or CF_UNICODETEXT.
	 * @param cfType the clipboard format.
	 * @return true for a text format.
	 */
	static bool IsTextFormat(CLIPFORMAT cfType);
	/**
	 * @brief RestoreTextOnly's step: looks for saved text formats and a saved file list.
	 * @param hDropIndex receives the index of the (last) CF_HDROP format; unchanged if there is none.
	 * @return true if a text format with data was saved.
	 */
	bool FindTextFormats(int& hDropIndex);
	/**
	 * @brief RestoreTextOnly's step: lists the files (not folders) of the saved file list.
	 * @param hDropIndex the index of the CF_HDROP format in m_Clipboard.
	 * @return the file paths, each followed by "\r\n".
	 * @throws DittoCore::ClipboardFormatError for a malformed file list.
	 */
	CString GetHDropFilePaths(int hDropIndex);
	/**
	 * @brief RestoreTextOnly's step: puts copies of the saved text formats on the open clipboard.
	 */
	void SetTextFormatCopies();
};
