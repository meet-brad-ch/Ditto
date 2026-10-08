#pragma once

#include "Options.h"
#include "Misc.h"
#include "Clip.h"
#include "MultiLanguage.h"
#include "DittoDb.h"
#include "RegisteredClipboardFormats.h"
#include "AppState.h"
#include "AppWindows.h"
#include "ClipContext.h"
#include "HotKeys.h"
#include "ExternalWindowTracker.h"
#include "DittoCopyBuffer.h"
#include "DittoAddins.h"
#include "ClipEditThread.h"
#include "ICU_String.h"
#include "ClipboardMonitor.h"
#include "GroupNavigator.h"
#include "ClipCommands.h"

/**
 * @brief The composition root: owns the application's services and hands them out.
 *
 * CCP_MainApp owns the one instance (theApp.Services()); it is CCP_MainApp's first member, so it is
 * created before and destroyed after everything else the application holds. Classes that an owner
 * creates get the services they need from that owner (constructor or function parameters, also for
 * static classes such as CDatabaseManager); classes derived from CCmdTarget (windows, dialogs,
 * property pages, threads, OLE sources) take them from theApp.Services(). The documented exceptions:
 * CLogger and CErrorReport, called from every class and thread, also use theApp.Services().
 * The services are created in the order of the members below (each gets the ones before it)
 * and destroyed in the reverse order.
 */
class CAppServices
{
public:
	/** @brief Creates the services; the settings start over the registry until LoadSettings runs. */
	CAppServices();

	CAppServices(const CAppServices&) = delete;
	CAppServices& operator=(const CAppServices&) = delete;

	/**
	 * @brief The application's settings.
	 * @return The settings, valid as long as this object.
	 */
	CGetSetOptions& Settings();

	/**
	 * @brief The UI texts of the chosen language (loaded by CCP_MainApp::InitInstance).
	 * @return The texts, valid as long as this object.
	 */
	CMultiLanguage& Language();

	/**
	 * @brief The clip database connection (opened by CCP_MainApp::InitInstance through
	 *        DatabaseLocator::CheckDBExists, or later by the no-database window; reopened by the
	 *        main frame after a resume and by the options).
	 * @return The connection, valid as long as this object.
	 */
	CDittoDb& Database();

	/**
	 * @brief The clipboard formats Ditto registers by name.
	 * @return The formats, valid as long as this object.
	 */
	const CRegisteredClipboardFormats& ClipboardFormats();

	/**
	 * @brief The application-wide state (current group, window flags, the clip saved last, ...).
	 * @return The state, valid as long as this object.
	 */
	CAppState& State();

	/**
	 * @brief The user's idle time.
	 * @return The idle-time reader, valid as long as this object.
	 */
	CIdleTime& IdleTime();

	/**
	 * @brief The main frame, its handle and the quick paste window, and the updates sent to them.
	 * @return The windows, valid as long as this object.
	 */
	CAppWindows& Windows();

	/**
	 * @brief The services a clip works with (CClip, CClip_ImportExport, CClipIDs).
	 * @return The clip context, valid as long as this object.
	 */
	CClipContext& ClipContext();

	/**
	 * @brief The system-wide hot keys (the registry and the named hot keys).
	 * @return The registry, valid as long as this object.
	 */
	CHotKeys& HotKeys();

	/**
	 * @brief The tracker of the window Ditto pastes into (and the elevated paste helper).
	 * @return The tracker, valid as long as this object.
	 */
	ExternalWindowTracker& ActiveWindow();

	/**
	 * @brief The Ditto copy buffers.
	 * @return The copy buffers, valid as long as this object.
	 */
	CDittoCopyBuffer& CopyBuffer();

	/**
	 * @brief The loaded add-ins.
	 * @return The add-ins, valid as long as this object.
	 */
	CDittoAddins& Addins();

	/**
	 * @brief The watcher of clip files being edited in an external editor.
	 * @return The watcher thread, valid as long as this object.
	 */
	CClipEditThread& EditThread();

	/**
	 * @brief The ICU case mapping (loaded by CCP_MainApp::InitInstance).
	 * @return The case mapping, valid as long as this object.
	 */
	CICU_String& IcuString();

	/**
	 * @brief The clipboard monitor: the copy thread and the clipboard connection.
	 * @return The monitor, valid as long as this object.
	 */
	CClipboardMonitor& Clipboard();

	/**
	 * @brief The navigation between groups.
	 * @return The navigator, valid as long as this object.
	 */
	CGroupNavigator& Groups();

	/**
	 * @brief The commands on whole clips (edit in an editor, import).
	 * @return The commands, valid as long as this object.
	 */
	CClipCommands& ClipCommands();

private:
	/// The application's settings.
	CGetSetOptions m_settings{};
	/// The UI texts.
	CMultiLanguage m_language{};
	/// The clip database connection; its errors that cannot be thrown go to the log.
	CDittoDb m_database;
	/// The registered clipboard formats.
	CRegisteredClipboardFormats m_clipboardFormats{};
	/// The application-wide state.
	CAppState m_state;
	/// The user's idle time.
	CIdleTime m_idleTime;
	/// The application's windows.
	CAppWindows m_windows;
	/// The services a clip works with.
	CClipContext m_clipContext;
	/// The system-wide hot keys.
	CHotKeys m_hotKeys;
	/// The tracker of the target window.
	ExternalWindowTracker m_activeWindow;
	/// The Ditto copy buffers.
	CDittoCopyBuffer m_copyBuffer;
	/// The add-ins.
	CDittoAddins m_addins;
	/// The watcher of edited clip files.
	CClipEditThread m_editThread;
	/// The ICU case mapping.
	CICU_String m_icuString{};
	/// The clipboard monitor.
	CClipboardMonitor m_clipboard;
	/// The navigation between groups.
	CGroupNavigator m_groups;
	/// The commands on whole clips.
	CClipCommands m_clipCommands;
};
