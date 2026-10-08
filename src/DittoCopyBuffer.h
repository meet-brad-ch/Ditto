#pragma once

#include "ClipboardSaveRestore.h"
#include "afxmt.h"

#include <memory>

class CAppWindows;
class CDittoDb;
class CGetSetOptions;
class CRegisteredClipboardFormats;
class ExternalWindowTracker;

class CClipboardSaveRestoreCopyBuffer : public CClipboardSaveRestore
{
public:
	/**
	 * @brief Creates an empty saved clipboard with no restore delay.
	 * @param windows The application's windows (the main window opens the clipboard); must outlive this object.
	 * @param formats The registered clipboard formats; must outlive this object.
	 */
	CClipboardSaveRestoreCopyBuffer(CAppWindows& windows, const CRegisteredClipboardFormats& formats) :
		CClipboardSaveRestore(windows, formats)
	{
		m_lRestoreDelay = 0;
	}
	long m_lRestoreDelay;
};

class CDittoCopyBuffer
{
public:
	/**
	 * @brief Creates the inactive copy buffer handler.
	 * @param settings The application's settings (buffer options, restore delay); must outlive this object.
	 * @param database The clip database (the CopyBuffers table); must outlive this object.
	 * @param activeWindow The tracker that sends the copy or cut to the active window; must outlive this object.
	 * @param windows The application's windows (the main frame pastes a buffer); must outlive this object.
	 * @param clipboardFormats The registered clipboard formats (the saved clipboard's restore); must outlive this object.
	 */
	CDittoCopyBuffer(CGetSetOptions& settings, CDittoDb& database, ExternalWindowTracker& activeWindow, CAppWindows& windows, const CRegisteredClipboardFormats& clipboardFormats);
	~CDittoCopyBuffer(void);

	CDittoCopyBuffer(const CDittoCopyBuffer&) = delete;
	CDittoCopyBuffer& operator=(const CDittoCopyBuffer&) = delete;

	bool Active() { return m_bActive; }
	bool StartCopy(long lCopyBuffer, bool bCut = false);
	bool EndCopy(long lID);
	bool PastCopyBuffer(long lCopyBuffer);

	/**
	 * @brief Stores a clip as the clip of a copy buffer (plays the sound when the buffer asks).
	 * @param settings The application's settings (the buffer's options).
	 * @param database The clip database (the CopyBuffers table).
	 * @param lClipId The clip.
	 * @param lBuffer The copy buffer.
	 * @return true on success; false after showing the database error.
	 */
	static bool PutClipOnDittoCopyBuffer(CGetSetOptions& settings, CDittoDb& database, long lClipId, long lBuffer);
	static UINT DelayRestoreClipboard(LPVOID pParam);
	static UINT StartCopyTimer(LPVOID pParam);

protected:
	void EndRestoreThread();

protected:
	/// The application's settings (not owned).
	CGetSetOptions& m_settings;
	/// The clip database (not owned).
	CDittoDb& m_database;
	/// The tracker that sends the copy or cut to the active window (not owned).
	ExternalWindowTracker& m_activeWindow;
	/// The application's windows (not owned).
	CAppWindows& m_windows;
	/// The registered clipboard formats (not owned).
	const CRegisteredClipboardFormats& m_clipboardFormats;
	long m_lCurrentDittoBuffer{};
	CClipboardSaveRestore m_SavedClipboard;
	bool m_bActive;
	DWORD m_dwLastPaste;
	CEvent m_ActiveTimer;
	CEvent m_RestoreTimer;
	CEvent m_Pasting;
	// The clipboard saved by PastCopyBuffer; DelayRestoreClipboard takes it over and restores it
	std::unique_ptr<CClipboardSaveRestoreCopyBuffer> m_pClipboard{};
};
