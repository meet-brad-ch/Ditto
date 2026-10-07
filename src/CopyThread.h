#pragma once

#include "ClipboardViewer.h"
#include <afxmt.h>
#include <memory>

struct CCopyConfig
{
public:
	// WM_CLIPBOARD_COPIED is sent to this window when a copy is made.
	HWND        m_hClipHandler{};
	// true to use PostMessage (asynchronous)
	// false to use SendMessage (synchronous)
	bool        m_bAsyncCopy{};
	// true to create a copy of the clipboard contents when it changes
	// false to ignore changes in the clipboard
	bool        m_bCopyOnChange{};
	// the supported types which are copied from the clipboard when it changes; this config owns them
	std::unique_ptr<CClipTypes> m_pSupportedTypes{}; // ONLY accessed from CopyThread

	CCopyConfig( HWND hClipHandler = NULL,
	             bool bAsyncCopy = false,
				 bool bCopyOnChange = false,
				 std::unique_ptr<CClipTypes> pSupportedTypes = nullptr )
		: m_hClipHandler(hClipHandler),
		  m_bAsyncCopy(bAsyncCopy),
		  m_bCopyOnChange(bCopyOnChange),
		  m_pSupportedTypes(std::move(pSupportedTypes))
	{
	}

	// Copies the settings of another config; the supported types stay with their owner
	void CopySettingsFrom(const CCopyConfig& other)
	{
		m_hClipHandler = other.m_hClipHandler;
		m_bAsyncCopy = other.m_bAsyncCopy;
		m_bCopyOnChange = other.m_bCopyOnChange;
	}
};

class CCopyThread : public CWinThread
{
	DECLARE_DYNCREATE(CCopyThread)
public:
	CCopyThread();
	virtual ~CCopyThread();

// Attributes
public:

// Operations
public:

	bool m_bQuit;
	bool m_connectOnStartup;

	CCriticalSection m_cs;

	// CopyThread Local (accessed from this CopyThread)
	// window owned by this thread which handles clipboard viewer messages
	std::unique_ptr<CClipboardViewer> m_pClipboardViewer{}; // permanent during lifetime of thread
	CCopyConfig         m_LocalConfig;

	// Called within Copy Thread:
	void OnClipboardChange(CString activeWindow); // called by ClipboardViewer
	void SyncConfig(); // safely syncs m_LocalConfig with m_SharedConfig

// Shared (use thread-safe access functions below)
	CCopyConfig         m_SharedConfig; 
	bool                m_bConfigChanged; // true if m_SharedConfig was changed.

	// Called within Main thread:
	bool IsClipboardViewerConnected();
	bool GetConnectCV();
	void SetConnectCV(bool bConnect);

	void SetSupportedTypes(std::unique_ptr<CClipTypes> pTypes); // CopyThread owns pTypes from now on
	HWND SetClipHandler(HWND hWnd); // returns previous value
	HWND GetClipHandler();
	bool SetCopyOnChange(bool bVal); // returns previous value
	bool GetCopyOnChange();
	bool SetAsyncCopy(bool bVal); // returns previous value
	bool GetAsyncCopy();

	void Init(CCopyConfig cfg);
	bool Quit();

	virtual BOOL InitInstance();
	virtual int ExitInstance();

private:
	/**
	 * @brief OnClipboardChange's load step: loads the clipboard into the clip, once more after the
	 * retry delay when nothing was found and the delay is set.
	 * @param clip The new clip.
	 * @param pSupportedTypes The types to load.
	 * @param activeWindow The source application.
	 * @return The result of the last CClip::LoadFromClipboard (TRUE, FALSE or -1).
	 * @throws DittoCore::ClipboardFormatError When the clipboard data is malformed.
	 */
	int LoadClipWithRetry(CClip& clip, CClipTypes* pSupportedTypes, const CString& activeWindow);

	/**
	 * @brief OnClipboardChange's last step: posts or sends the clip to the clip handler window.
	 * @param pClip The clip; released when the handler takes it (the handler then owns it).
	 */
	void HandOverClip(std::unique_ptr<CClip>& pClip);
};
