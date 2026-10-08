#include "stdafx.h"
#include "RegisteredClipboardFormats.h"
#include "ClipboardFormats.h"

// registered in the order CCP_MainApp's constructor registered them
CRegisteredClipboardFormats::CRegisteredClipboardFormats() :
	m_rtf(Register(_T("Rich Text Format"))),
	m_html(Register(_T("HTML Format"))),
	m_ping(Register(_T("Ditto Ping Format"))),
	m_ignoreClipboard(Register(_T("Clipboard Viewer Ignore"))),
	m_delaySavingData(Register(_T("Ditto Delay Saving Data"))),
	m_dittoFileData(Register(_T("Ditto File Data"))),
	m_png(CClipboardFormats::GetFormatID(_T("PNG"))),
	//https://learn.microsoft.com/en-us/windows/win32/dataxchg/clipboard-formats
	m_excludeFromMonitorProcessing(Register(_T("ExcludeClipboardContentFromMonitorProcessing"))),
	m_canIncludeInClipboardHistory(Register(_T("CanIncludeInClipboardHistory")))
{
}

CLIPFORMAT CRegisteredClipboardFormats::Register(const TCHAR* name)
{
	return static_cast<CLIPFORMAT>(::RegisterClipboardFormat(name));
}

CLIPFORMAT CRegisteredClipboardFormats::Rtf() const
{
	return m_rtf;
}

CLIPFORMAT CRegisteredClipboardFormats::Html() const
{
	return m_html;
}

CLIPFORMAT CRegisteredClipboardFormats::Png() const
{
	return m_png;
}

CLIPFORMAT CRegisteredClipboardFormats::Ping() const
{
	return m_ping;
}

CLIPFORMAT CRegisteredClipboardFormats::IgnoreClipboard() const
{
	return m_ignoreClipboard;
}

CLIPFORMAT CRegisteredClipboardFormats::DelaySavingData() const
{
	return m_delaySavingData;
}

CLIPFORMAT CRegisteredClipboardFormats::DittoFileData() const
{
	return m_dittoFileData;
}

CLIPFORMAT CRegisteredClipboardFormats::ExcludeClipboardContentFromMonitorProcessing() const
{
	return m_excludeFromMonitorProcessing;
}

CLIPFORMAT CRegisteredClipboardFormats::CanIncludeInClipboardHistory() const
{
	return m_canIncludeInClipboardHistory;
}
