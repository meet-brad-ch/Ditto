#pragma once

#include "Clip.h"
#include "CopyThread.h"

class CAppState;
class CAppWindows;
class CDittoCopyBuffer;
class CDittoDb;
class CGetSetOptions;
class CMultiLanguage;
class CQPasteWnd;

/**
 * @brief Watches the clipboard: owns the copy thread, keeps the connection to the clipboard (with
 *        the tray icon and the caption that show it) and handles a finished copy.
 *
 * Owned by CAppServices (Clipboard()).
 */
class CClipboardMonitor
{
public:
	/**
	 * @brief Creates the monitor; the copy thread starts in Start.
	 * @param settings The application's settings; must outlive this object.
	 * @param database The clip database (the saved types); must outlive this object.
	 * @param language The UI texts; must outlive this object.
	 * @param state The application state (whether the quick paste window shows); must outlive this object.
	 * @param windows The application's windows (tray icon, caption, refresh); must outlive this object.
	 * @param copyBuffer The copy buffers (a finished copy ends a buffer copy); must outlive this object.
	 */
	CClipboardMonitor(CGetSetOptions& settings, CDittoDb& database, CMultiLanguage& language, CAppState& state, CAppWindows& windows, CDittoCopyBuffer& copyBuffer);

	CClipboardMonitor(const CClipboardMonitor&) = delete;
	CClipboardMonitor& operator=(const CClipboardMonitor&) = delete;

	/**
	 * @brief Starts the copy thread for the main window (CAppWindows::MainHwnd must be set).
	 * @param connectOnStartup TRUE or FALSE from the command line (/Connect, /Disconnect), -1 when not given.
	 * @return False when the clip types could not be read or the thread could not start (reported).
	 */
	bool Start(int connectOnStartup);

	/** @brief Stops copying and asks the copy thread to quit. */
	void Stop();

	/**
	 * @brief The copy thread's clipboard viewer window (for posting messages).
	 * @return The window handle.
	 */
	HWND GetClipboardViewer();

	/**
	 * @brief Turns copying on clipboard changes on or off.
	 * @param bState True: copy.
	 * @return The previous value.
	 */
	bool EnableCbCopy(bool bState);

	/**
	 * @brief Whether the clipboard viewer is connected now.
	 * @return True when connected.
	 */
	bool IsClipboardViewerConnected();

	/**
	 * @brief Whether Ditto is meant to be connected to the clipboard (it might not be yet: see
	 *        IsClipboardViewerConnected).
	 * @return True when connected.
	 */
	bool GetConnectCV();

	/**
	 * @brief Connects to or disconnects from the clipboard, saves it in the settings and shows it in
	 *        the tray icon and the quick paste window's caption.
	 * @param bConnect True: connect.
	 */
	void SetConnectCV(bool bConnect);

	/**
	 * @brief Switches the clipboard connection.
	 * @return The new state (it might not be connected yet: see IsClipboardViewerConnected).
	 */
	bool ToggleConnectCV();

	/**
	 * @brief Sets a menu entry to the available connect command (the opposite of the current state).
	 * @param pMenu The menu; NULL does nothing.
	 * @param nMenuID The entry.
	 */
	void UpdateMenuConnectCV(CMenu* pMenu, UINT nMenuID);

	/**
	 * @brief Turns the persistent quick paste window on or off and shows it in the caption.
	 * @param bVal True: persistent.
	 */
	void ShowPersistent(bool bVal);

	/** @brief Reads the clipboard types again and hands them to the copy thread (when the read worked). */
	void ReloadTypes();

	/**
	 * @brief Handles copied clips that were saved: counts them, ends a copy-buffer copy, refreshes the view.
	 * @param lLastID The id of the last saved clip.
	 * @param count The number of saved clips; <= 0 does nothing.
	 * @param copyReason Why the copy happened.
	 */
	void OnCopyCompleted(long lLastID, int count = 1, CopyReasonEnum::CopyReason copyReason = CopyReasonEnum::COPY_TO_UNKOWN);

private:
	/**
	 * @brief Shows the persistent and connection state in the quick paste window's caption.
	 * @param pasteWnd The quick paste window.
	 */
	void RefreshCaption(CQPasteWnd& pasteWnd);

	/** @brief The application's settings (not owned). */
	CGetSetOptions& m_settings;
	/** @brief The clip database (not owned). */
	CDittoDb& m_database;
	/** @brief The UI texts (not owned). */
	CMultiLanguage& m_language;
	/** @brief The application state (not owned). */
	CAppState& m_state;
	/** @brief The application's windows (not owned). */
	CAppWindows& m_windows;
	/** @brief The copy buffers (not owned). */
	CDittoCopyBuffer& m_copyBuffer;
	/** @brief The copy thread (copy and paste management); not auto-deleted. */
	CCopyThread m_CopyThread;
};
