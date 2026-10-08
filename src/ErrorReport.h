/**
 * @file ErrorReport.h
 * @brief Declares CErrorReport.
 */
#pragma once

/**
 * @brief Reports a failed operation to the user as an error balloon on the tray icon.
 *
 * Safe to call from any thread: the text is posted to the main window, which takes
 * ownership and shows it (CMainFrame::OnOwnedErrorMsg). While Ditto is not fully running
 * (start-up, no database, closing), or when posting fails, a message box shows it instead (that
 * blocks the calling thread until closed). Reads the main window and the running state through
 * theApp.Services() (the documented exception to the access rule: it is called from every class
 * and thread).
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
