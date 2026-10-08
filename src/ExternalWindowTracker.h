#pragma once

#include "Misc.h"

#include <memory>

class CAppState;
class CAppWindows;
class CGetSetOptions;
class CIdleTime;
class CSendKeys;
class CUAC_Thread;

class ExternalWindowTracker
{
public:
	/**
	 * @brief Creates the tracker and loads oleacc.dll for the caret lookup.
	 * @param settings The application's settings (focus tracking and send-keys options); must
	 *        outlive this object.
	 * @param idleTime The user's idle time (focus tracking waits for it); must outlive this object.
	 * @param state The application state (the copy reason of a sent copy); must outlive this object.
	 * @param windows The application's windows (the quick paste window's status); must outlive this object.
	 */
	ExternalWindowTracker(CGetSetOptions& settings, CIdleTime& idleTime, CAppState& state, CAppWindows& windows);
	~ExternalWindowTracker(void);

	ExternalWindowTracker(const ExternalWindowTracker&) = delete;
	ExternalWindowTracker& operator=(const ExternalWindowTracker&) = delete;

	/**
	 * @brief Runs this process as the elevated helper of another Ditto: pastes, copies and cuts when
	 *        that Ditto asks, until it ends or asks this helper to exit (blocks until then).
	 * @param parentProcessId The process id of the Ditto that started this helper.
	 */
	void RunUacHelper(int parentProcessId);

	/** @brief Ends the elevated helper thread (asks it to exit when it never started) and frees it. */
	void StopUacThread();

	HWND ActiveWnd() const { return m_activeWnd; }
	HWND FocusWnd() const { return m_focusWnd; }
	bool DittoHasFocus() const { return m_dittoHasFocus; }
	bool DesktopHasFocus() const { return m_desktopHasFocus; }

	CString ActiveWndName();
	CString WndName(HWND hWnd);
	bool TrackActiveWnd(bool force);
	bool ActivateTarget();
	bool ReleaseFocus();
	CPoint FocusCaret();

	void SendPaste(bool activateTarget);
	void SendCut();
	void SendCopy(CopyReasonEnum::CopyReason copyReason);

	bool NotifyTrayhWnd(HWND hWnd);

protected:
	/// The application's settings (not owned).
	CGetSetOptions& m_settings;
	/// The user's idle time (not owned).
	CIdleTime& m_idleTime;
	/// The application state (not owned).
	CAppState& m_state;
	/// The application's windows (not owned).
	CAppWindows& m_windows;
	/// The elevated helper thread; created on the first paste, copy or cut as administrator (or by RunUacHelper).
	std::unique_ptr<CUAC_Thread> m_pUacPasteThread{};

	typedef HRESULT(__stdcall *AccessibleObjectFromWindow)(_In_ HWND hwnd, _In_ DWORD dwId, _In_ REFIID riid, _Outptr_ void** ppvObject);

	HWND m_activeWnd;
	HWND m_focusWnd;
	bool m_dittoHasFocus;
	bool m_desktopHasFocus;
	HMODULE m_hOleacc;
	AccessibleObjectFromWindow m_AccessibleObjectFromWindow;
	
protected:
	bool WaitForActiveWnd(HWND hwndToHaveFocus, int timeout);
	void ActivateFocus(const HWND active_wnd, const HWND focus_wnd);

private:
	/** @brief Finds the focus window of the thread of the active window.
	 *  @param newActive The foreground window.
	 *  @return The focus window, or NULL if it cannot be read. */
	HWND GetFocusOfActiveWnd(HWND newActive);

	/** @brief If only one of the focus and active windows is known, uses it for both.
	 *  @param newFocus Focus window (may be NULL).
	 *  @param newActive Active window (may be NULL). */
	static void FillMissingWnd(HWND& newFocus, HWND& newActive);

	/** @brief Tells if the focus or the active window is missing or no window.
	 *  @param newFocus Focus window.
	 *  @param newActive Active window.
	 *  @return true if one of them is NULL or not a window. */
	static bool HasInvalidWnd(HWND newFocus, HWND newActive);

	/** @brief Tells if the active or the focus window is the notification area (tray).
	 *  @param newActive Active window.
	 *  @param newFocus Focus window.
	 *  @return true if one of them is the tray window. */
	bool HasNotifyTrayWnd(HWND newActive, HWND newFocus);

	/** @brief Tells if the focus or the active window belongs to Ditto.
	 *  @param newFocus Focus window.
	 *  @param newActive Active window.
	 *  @return true if one of them is a Ditto window. */
	static bool HasAppWnd(HWND newFocus, HWND newActive);

	/** @brief Marks that Ditto has the focus and logs the change once.
	 *  @param fromHook Value logged as FromHook. */
	void SetDittoHasFocus(BOOL fromHook);

	/** @brief Tells if a window is the desktop (class "Progman").
	 *  @param newActive Window to check.
	 *  @return true for the desktop window. */
	static bool IsDesktopWnd(HWND newActive);

	/** @brief Activates the target window for a paste and waits until it is in the foreground.
	 *  @param activeWnd The target window. */
	void ActivateTargetForPaste(HWND activeWnd);

	/**
	 * @brief The elevated helper thread for this process; created on first use.
	 * @return The thread, owned by this tracker.
	 */
	CUAC_Thread& UacThread();

	/**
	 * @brief Whether the elevated helper thread runs.
	 * @return False when it was never created or does not run.
	 */
	bool UACThreadRunning();

	/** @brief Passes an elevated paste to the UAC aware helper app, when that is possible.
	 *  @param pasteAsAdmin true if the paste must be done as administrator.
	 *  @return pasteAsAdmin, set to false if the helper app could not take the paste. */
	bool PassPasteToUacApp(bool pasteAsAdmin);

	/**
	 * @brief Sends a configured key string and shows an error when it is malformed.
	 * @param send The key sender.
	 * @param keys The key string (from the paste, copy or cut options).
	 * @param action What the keys do ("paste", "copy", "cut"), for the error.
	 */
	static void SendKeyString(CSendKeys& send, const CString& keys, LPCTSTR action);

	/** @brief Tells if a caret position was found.
	 *  @param pt Caret position, (-1, -1) when not found.
	 *  @return true if neither coordinate is -1. */
	static bool IsCaretFound(const CPoint& pt);

	/** @brief Reads the caret position with the IAccessible object of the active window.
	 *  @param pt Set to the caret position when it is found. */
	void CaretFromAccessible(CPoint& pt);

	/** @brief Reads the caret position with GetGUIThreadInfo.
	 *  @param threadId Thread of the active window.
	 *  @param pt Set to the caret position when it is found. */
	void CaretFromGuiThreadInfo(DWORD threadId, CPoint& pt);

	/** @brief Reads the caret position by attaching to the thread of the active window.
	 *  @param threadId Thread of the active window.
	 *  @param pt Set to the caret position, or to (-1, -1) when GetCaretPos gives none. */
	void CaretFromAttachedThread(DWORD threadId, CPoint& pt);
};
