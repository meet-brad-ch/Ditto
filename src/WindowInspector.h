#pragma once

/** @brief Questions about other windows: which process owns them and what it is called. */
class CWindowInspector
{
public:
	/**
	 * @brief Whether a window is owned by this process.
	 * @param hWnd The window.
	 * @return true when the window's process is this process.
	 */
	static bool IsAppWnd(HWND hWnd);

	/**
	 * @brief The executable name of the process that owns a window. A UWP app's frame
	 * (ApplicationFrameHost.exe) gives the name of the hosted app's process.
	 * @param hWnd The window.
	 * @param processId The process id; 0 to take it from the window.
	 * @return The executable name (with extension); empty when it is not found.
	 */
	static CString GetProcessName(HWND hWnd, DWORD processId = 0);

private:
	/** @brief The EnumChildWindows context of UwpAppName. */
	struct WindowInfo
	{
		/** @brief The process of the frame window. */
		DWORD ownerpid{};
		/** @brief The process of a child window from another process; ownerpid when there is none. */
		DWORD childpid{};
	};

	/**
	 * @brief The executable name of the process of a UWP frame window's hosted app (the last child
	 * window owned by another process).
	 * @param active_window The frame window.
	 * @param ownerpid The frame window's process.
	 * @return The executable name; empty when the process cannot be opened.
	 */
	static CString UwpAppName(HWND active_window, DWORD ownerpid);

	/**
	 * @brief GetProcessName's fallback: the executable name of a process from a snapshot of all processes.
	 * @param processId The process id.
	 * @return The executable name; empty when the process is not found or the snapshot fails (logged).
	 */
	static CString ProcessNameFromSnapshot(DWORD processId);

	/**
	 * @brief The EnumChildWindows callback of UwpAppName: records a child window's process when it
	 * is not the owner's.
	 * @param hWnd The child window.
	 * @param lp The WindowInfo.
	 * @return TRUE to go on enumerating.
	 */
	static BOOL CALLBACK EnumChildWindowsCallback(HWND hWnd, LPARAM lp);
};
