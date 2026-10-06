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
	Log(StrF(_T("Error reported to the user: %s"), text.GetString()));

	auto message = std::make_unique<CString>(text);
	if (::PostMessage(theApp.m_MainhWnd, WM_SHOW_OWNED_ERROR_MSG, reinterpret_cast<WPARAM>(message.get()), 0))
	{
		message.release();  // CMainFrame::OnOwnedErrorMsg owns it now
	}
	else
	{
		Log(StrF(_T("Could not post the error to the main window, GetLastError %d"), ::GetLastError()));
	}
}
