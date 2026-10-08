#pragma once

class CAppState;
class CAppWindows;

/**
 * @brief Shows the main window's taskbar button while the object lives (for a dialog or a message
 *        box); the last of these objects to end puts the main window back into the tray.
 */
class CShowTaskBarIcon
{
public:
	/**
	 * @brief Shows the main frame's taskbar button and counts this user.
	 * @param windows The application's windows (the main frame must exist); must outlive this object.
	 * @param state The application state (the count of users); must outlive this object.
	 */
	CShowTaskBarIcon(CAppWindows& windows, CAppState& state);
	~CShowTaskBarIcon(void);

	CShowTaskBarIcon(const CShowTaskBarIcon&) = delete;
	CShowTaskBarIcon& operator=(const CShowTaskBarIcon&) = delete;

	/** @brief The main frame's window when the object was created. */
	HWND m_hWnd{};

private:
	/** @brief The application's windows (not owned). */
	CAppWindows& m_windows;
	/** @brief The application state (not owned). */
	CAppState& m_state;
};

