#pragma once

#include "ClipboardSaveRestore.h"
#include "afxmt.h"

#include <memory>

class CClipboardSaveRestoreCopyBuffer : public CClipboardSaveRestore
{
public:
	CClipboardSaveRestoreCopyBuffer()
	{
		m_lRestoreDelay = 0;
	}
	long m_lRestoreDelay;
};

class CGetSetOptions;

class CDittoCopyBuffer
{
public:
	/**
	 * @brief Creates the inactive copy buffer handler.
	 * @param settings The application's settings (buffer options, restore delay); must outlive this object.
	 */
	explicit CDittoCopyBuffer(CGetSetOptions& settings);
	~CDittoCopyBuffer(void);

	bool Active()	{ return m_bActive; }
	bool StartCopy(long lCopyBuffer, bool bCut = false);
	bool EndCopy(long lID);
	bool PastCopyBuffer(long lCopyBuffer);

	/**
	 * @brief Stores a clip as the clip of a copy buffer (plays the sound when the buffer asks).
	 * @param settings The application's settings (the buffer's options).
	 * @param lClipId The clip.
	 * @param lBuffer The copy buffer.
	 * @return true on success; false after showing the database error.
	 */
	static bool PutClipOnDittoCopyBuffer(CGetSetOptions& settings, long lClipId, long lBuffer);
	static UINT DelayRestoreClipboard(LPVOID pParam);
	static UINT StartCopyTimer(LPVOID pParam);

protected:
	void EndRestoreThread();

protected:
	/// The application's settings (not owned).
	CGetSetOptions& m_settings;
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
