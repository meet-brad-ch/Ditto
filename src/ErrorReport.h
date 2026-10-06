/**
 * @file ErrorReport.h
 * @brief Declares CErrorReport.
 */
#pragma once

/**
 * @brief Reports a failed operation to the user as an error balloon on the tray icon.
 *
 * Safe to call from any thread: the text is posted to the main window, which takes
 * ownership and shows it (CMainFrame::OnOwnedErrorMsg).
 */
class CErrorReport
{
public:
	/**
	 * @brief Logs the text and shows it to the user.
	 * @param text What failed and why, as the user should read it.
	 */
	static void Show(const CString& text);
};
