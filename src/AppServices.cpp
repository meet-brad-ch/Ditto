#include "stdafx.h"
#include "CP_Main.h" // the service types in the order they need
#include "AppServices.h"

CAppServices::CAppServices() :
	m_database([](const CString& text)
			   { CLogger::Log(text); }),
	m_state(m_settings),
	m_idleTime(m_settings),
	m_windows(m_state),
	m_clipContext(m_settings, m_state.LastAddedClip(), m_database, m_clipboardFormats, m_windows),
	m_hotKeys(m_settings),
	m_activeWindow(m_settings, m_idleTime, m_state, m_windows),
	m_copyBuffer(m_settings, m_database, m_activeWindow, m_windows, m_clipboardFormats),
	m_addins(m_settings, m_language, m_windows),
	m_editThread(m_settings, m_clipContext, m_windows),
	m_clipboard(m_settings, m_database, m_language, m_state, m_windows, m_copyBuffer),
	m_groups(m_state, m_database, m_windows),
	m_clipCommands(m_settings, m_language, m_clipboardFormats, m_state, m_windows, m_editThread, m_clipContext)
{
}

CGetSetOptions& CAppServices::Settings()
{
	return m_settings;
}

CMultiLanguage& CAppServices::Language()
{
	return m_language;
}

CDittoDb& CAppServices::Database()
{
	return m_database;
}

const CRegisteredClipboardFormats& CAppServices::ClipboardFormats()
{
	return m_clipboardFormats;
}

CAppState& CAppServices::State()
{
	return m_state;
}

CIdleTime& CAppServices::IdleTime()
{
	return m_idleTime;
}

CAppWindows& CAppServices::Windows()
{
	return m_windows;
}

CClipContext& CAppServices::ClipContext()
{
	return m_clipContext;
}

CHotKeys& CAppServices::HotKeys()
{
	return m_hotKeys;
}

ExternalWindowTracker& CAppServices::ActiveWindow()
{
	return m_activeWindow;
}

CDittoCopyBuffer& CAppServices::CopyBuffer()
{
	return m_copyBuffer;
}

CDittoAddins& CAppServices::Addins()
{
	return m_addins;
}

CClipEditThread& CAppServices::EditThread()
{
	return m_editThread;
}

CICU_String& CAppServices::IcuString()
{
	return m_icuString;
}

CClipboardMonitor& CAppServices::Clipboard()
{
	return m_clipboard;
}

CGroupNavigator& CAppServices::Groups()
{
	return m_groups;
}

CClipCommands& CAppServices::ClipCommands()
{
	return m_clipCommands;
}
