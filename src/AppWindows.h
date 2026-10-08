#pragma once

#include "Misc.h"

class CAppState;
class CMainFrame;
class CQPasteWnd;

/**
 * @brief The application's main windows as the services see them: the main frame, its handle and
 *        the quick paste window, and the updates the services send to the quick paste window.
 *
 * Owned by CAppServices (Windows()). CCP_MainApp sets the main frame and its handle when it
 * creates them; until then (and in the no-database mode) MainFrame() is null.
 */
class CAppWindows
{
public:
	/**
	 * @brief Creates the link without windows.
	 * @param state The application state (the refresh mode and the status text); must outlive this object.
	 */
	explicit CAppWindows(CAppState& state);

	CAppWindows(const CAppWindows&) = delete;
	CAppWindows& operator=(const CAppWindows&) = delete;

	/**
	 * @brief The main frame window.
	 * @return The frame (not owned: it deletes itself), or null before it is created.
	 */
	CMainFrame* MainFrame() const;

	/**
	 * @brief Sets the main frame window (CCP_MainApp::CreateMainWnd).
	 * @param frame The frame, or null when it could not be created.
	 */
	void SetMainFrame(CMainFrame* frame);

	/**
	 * @brief The main frame's window handle.
	 * @return The handle, or NULL before the frame's window exists.
	 */
	HWND MainHwnd() const;

	/**
	 * @brief Sets the main frame's window handle (CCP_MainApp::AfterMainCreate).
	 * @param hWnd The handle, or NULL.
	 */
	void SetMainHwnd(HWND hWnd);

	/**
	 * @brief The quick paste window.
	 * @return The window, or null when the frame or the window does not exist.
	 */
	CQPasteWnd* QPasteWnd() const;

	/**
	 * @brief The quick paste window's handle.
	 * @return The handle, or NULL when the window does not exist.
	 */
	HWND QPastehWnd() const;

	/**
	 * @brief Refreshes the clip list when the quick paste window exists (posted or sent, see
	 *        CAppState::m_bAsynchronousRefreshView).
	 * @param copyReason Why the list changed.
	 */
	void RefreshView(CopyReasonEnum::CopyReason copyReason = CopyReasonEnum::COPY_TO_UNKOWN);

	/**
	 * @brief Reloads one clip in the quick paste window when it exists (posted or sent).
	 * @param clipId The clip.
	 * @param updateFlags CClipRefreshFlags values.
	 */
	void RefreshClipInUI(int clipId, int updateFlags);

	/**
	 * @brief Tells the quick paste window that a clip was deleted (posted).
	 * @param lID The clip.
	 */
	void OnDeleteID(long lID);

	/**
	 * @brief Sets the status text and updates the quick paste window's caption.
	 * @param status The text; NULL clears it.
	 * @param bRepaintImmediately True: repaint the caption now.
	 */
	void SetStatus(const TCHAR* status = NULL, bool bRepaintImmediately = false);

	/** @brief Shows or hides the main frame's taskbar button as the settings say (when the frame exists). */
	void RefreshShowInTaskBar();

	/**
	 * @brief Dispatches the messages waiting for a window (or for this thread).
	 * @param hWnd The window; NULL for every window of this thread.
	 */
	static void PumpMessages(HWND hWnd = NULL);

private:
	/** @brief The application state (not owned). */
	CAppState& m_state;
	/** @brief The main frame (not owned: it deletes itself). */
	CMainFrame* m_pMainFrame{};
	/** @brief The main frame's window handle. */
	HWND m_mainHwnd{};
};
