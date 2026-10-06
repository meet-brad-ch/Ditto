#pragma once

// Reports an operation that failed to the user as an error balloon on the tray icon. Safe to call
// from any thread: the text is posted to the main window, which takes ownership and shows it.
class CErrorReport
{
public:
	static void Show(const CString& text);
};
