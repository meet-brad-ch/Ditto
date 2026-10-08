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
	CLogger::Log(CStringUtil::Format(_T("Error reported to the user: %s"), text.GetString()));

	// Only a running main window shows a posted error. During start-up (a failed start destroys
	// the window before it reads its messages), without a database, or while closing, a message
	// box shows it instead
	// the documented exception to the access rule: CErrorReport is called from every class and thread
	CAppServices& services{ theApp.Services() };
	const HWND mainWindow = services.Windows().MainHwnd();
	if (mainWindow != NULL && services.State().m_bAppRunning)
	{
		auto message = std::make_unique<CString>(text);
		if (::PostMessage(mainWindow, CDittoMessage::ShowOwnedErrorMsg, reinterpret_cast<WPARAM>(message.get()), 0))
		{
			message.release(); // CMainFrame::OnOwnedErrorMsg owns it now
			return;
		}
		CLogger::Log(CStringUtil::Format(_T("Could not post the error to the main window, GetLastError %d; showing a message box"), ::GetLastError()));
	}

	::MessageBox(NULL, text, _T("Ditto"), MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
}
