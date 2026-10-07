/**
 * @file ErrorReport.cpp
 * @brief Implements CErrorReport.
 */
#include "stdafx.h"
#include "ErrorReport.h"
#include "CP_Main.h"
#include "Misc.h"

#include <memory>

void CErrorReport::Show(const CString& text)
{
	CLogger::Log(StrF(_T("Error reported to the user: %s"), text.GetString()));

	// Only a running main window shows a posted error. During start-up (a failed start destroys
	// the window before it reads its messages), without a database, or while closing, a message
	// box shows it instead
	const HWND mainWindow = theApp.m_MainhWnd;
	if (mainWindow != NULL && theApp.m_bAppRunning)
	{
		auto message = std::make_unique<CString>(text);
		if (::PostMessage(mainWindow, CDittoMessage::ShowOwnedErrorMsg, reinterpret_cast<WPARAM>(message.get()), 0))
		{
			message.release();  // CMainFrame::OnOwnedErrorMsg owns it now
			return;
		}
		CLogger::Log(StrF(_T("Could not post the error to the main window, GetLastError %d; showing a message box"), ::GetLastError()));
	}

	::MessageBox(NULL, text, _T("Ditto"), MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
}
