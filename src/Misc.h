
#if !defined(AFX_CP_GUI_GLOBALS__FBCDED09_A6F2_47EB_873F_50A746EBC86B__INCLUDED_)
#define AFX_CP_GUI_GLOBALS__FBCDED09_A6F2_47EB_873F_50A746EBC86B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\Shared/ArrayEx.h"
#include <atomic>
#include <source_location>
#include <vector>

/**
 * @brief The mouse "keys" of Ditto's shortcuts: key codes below the real virtual keys for the
 * mouse actions in the quick paste list. They are stored in the settings as shortcut keys.
 */
class CMouseKey
{
public:
	/** @brief The mouse key codes. */
	enum : int
	{
		Click = 0x01,       ///< a left click
		DoubleClick = 0x02, ///< a left double click
		RightClick = 0x03,  ///< a right click
		MiddleClick = 0x04  ///< a middle click
	};
};

/** @brief The flags of CAppWindows::RefreshClipInUI: what to do after a clip was reloaded. */
class CClipRefreshFlags
{
public:
	/** @brief The flag bits, combined with |. */
	enum : int
	{
		AfterPasteSelectClip = 0x1, ///< the clip was pasted: select it
		ClipDescription = 0x2       ///< the clip's description changed
	};
};

/** @brief Keyboard state queries. */
class CKeyboard
{
public:
	/**
	 * @brief Whether a control key is down. Foreign keyboards send right alt (ALT_GR) as left
	 * control + right alt; that is not a control press.
	 * http://compgroups.net/comp.os.programmer.win32/alt-gr-key-and-left-ctrl/2840252
	 * @return true when control is down and it is not ALT_GR.
	 */
	static bool IsControlPressed()
	{
		return ((::GetKeyState(VK_CONTROL) & 0x8000) && (((GetKeyState(VK_RMENU) < 0) && (GetKeyState(VK_LCONTROL) < 0)) == FALSE));
	}
};

/** @brief Durations in milliseconds (timer periods and waits). */
class CMilliseconds
{
public:
	/** @brief One minute in milliseconds. */
	static constexpr int OneMinute = 60000;
	/** @brief One hour in milliseconds. */
	static constexpr int OneHour = 3600000;
};

class CopyReasonEnum
{
public:
	enum CopyReason
	{ 
		COPY_TO_UNKOWN,
		COPY_TO_GROUP,
		COPY_TO_BUFFER,
		COPY_FROM_TOOLTIP
	};
};

/** @brief Folder path text helpers. */
class CFolderPath
{
public:
	/**
	 * @brief Appends a backslash to a folder path that does not end with a backslash or a slash;
	 * an empty path stays empty.
	 * @param csPath The folder path; changed in place.
	 */
	static void AddTrailingSlash(CString& csPath)
	{
		if(csPath.IsEmpty() == FALSE && csPath.GetAt(csPath.GetLength()-1) != '\\' && csPath.GetAt(csPath.GetLength()-1) != '/')
			csPath += "\\";
	}
};

#include "DatabaseUtilities.h"

#include "Logger.h"
#include "StringUtil.h"
#include "GlobalMemory.h"
#include "ClipboardFormats.h"
#include "FileSystem.h"
#include "TempFileCleaner.h"
#include "MonitorGeometry.h"
#include "WindowInspector.h"
#include "SystemTheme.h"
#include "AppVersion.h"
#include "ClipDatabase.h"
#include "DatabaseBackupPrompt.h"

class CGetSetOptions;

/**
 * @brief How long the user has been idle (no keyboard or mouse input). Owned by CAppServices
 * (IdleTime()); called from several threads.
 */
class CIdleTime
{
public:
	/**
	 * @brief Creates the idle-time reader; the tick-count check runs on the first IdleSeconds call.
	 * @param settings The application's settings (the tick-count adjustment); must outlive this object.
	 */
	explicit CIdleTime(CGetSetOptions& settings);

	CIdleTime(const CIdleTime&) = delete;
	CIdleTime& operator=(const CIdleTime&) = delete;

	/**
	 * @brief The time since the last input (GetLastInputInfo). When the tick count was found below
	 * the last input time on the first call, the settings' GetFunnyTickCountAdjustment() is added
	 * to the tick count (logged once).
	 * @return The idle time in seconds.
	 */
	double IdleSeconds();

private:
	/** @brief The states of the tick-count check (m_adjustment). */
	enum AdjustmentState : int
	{
		NotChecked = -1,      ///< no IdleSeconds call yet
		NoAdjustment = 0,     ///< the tick count was not below the last input time
		AdjustAndLog = 1,     ///< adjust; the first adjusting call logs it
		Adjust = 2            ///< adjust (already logged)
	};

	/**
	 * @brief Decides on the first call whether the tick count needs the adjustment.
	 * @param currentTick The tick count (32-bit, as LASTINPUTINFO::dwTime).
	 * @param lastInputTick LASTINPUTINFO::dwTime.
	 */
	void CheckTickCount(DWORD currentTick, DWORD lastInputTick);

	/** @brief The application's settings (not owned). */
	CGetSetOptions& m_settings;
	/** @brief The AdjustmentState of the tick-count check; atomic: IdleSeconds runs on several threads. */
	std::atomic<int> m_adjustment{NotChecked};
};


/**
 * @brief Ditto's own window messages (WM_USER + n). Some are sent from a second Ditto process to
 * the running one (command line switches): the values never change. WM_USER + 202, 204, 209 and
 * 211 had names without a sender or a handler and are left out; WM_USER + 215 and 216 are the
 * tray icon's messages.
 */
class CDittoMessage
{
public:
	/** @brief The message ids. */
	enum : UINT
	{
		ShowTrayIcon = WM_USER + 200,      ///< to the main window: show the tray icon or not
		SetConnect = WM_USER + 201,        ///< to the clipboard viewer: connect to the clipboard chain or not
		RefreshView = WM_USER + 205,       ///< to the quick paste window: reload the list
		ClipboardCopied = WM_USER + 206,   ///< wParam: CClip* owned by the receiver (CMainFrame::OnClipboardCopied)
		ShowOwnedErrorMsg = WM_USER + 207, ///< wParam: CString* owned by the receiver (CErrorReport)
		EditWndClosing = WM_USER + 212,    ///< the edit window is closing
		SetConnected = WM_USER + 213,      ///< to the main window: connect or disconnect from the clipboard
		ReloadClipInUi = WM_USER + 217,    ///< to the quick paste window: reload one clip (CClipRefreshFlags)
		GlobalClipsClosed = WM_USER + 218, ///< the global clips window closed
		OptionsClosed = WM_USER + 219,     ///< the options sheet closed
		ShowOptions = WM_USER + 220,       ///< to the main window: show the options
		DeleteClipsClosed = WM_USER + 221, ///< the delete clips window closed
		OpenCloseWindow = WM_USER + 222,   ///< to the main window: open or close the quick paste window
		SaveClipboard = WM_USER + 223,     ///< to the main window: save the clipboard
		ReaddTaskbarIcon = WM_USER + 224,  ///< to the main window: add the tray icon again
		ReopenDatabase = WM_USER + 225,    ///< to the main window: open the database again
		ShowMsgWindow = WM_USER + 226,     ///< wParam: CString* owned by the receiver (CMainFrame::OnShowMsgWindow)
		ShowDittoGroup = WM_USER + 227,    ///< to the main window: show a group
		PlainTextPaste = WM_USER + 228,    ///< to the main window: paste the clipboard as plain text
		RestoreDb = WM_USER + 230,         ///< to the main window: restore the database
		BackupDb = WM_USER + 231,          ///< to the main window: back up the database
		RefreshFooter = WM_USER + 232,     ///< to the tool tip: refresh its footer
		PasteClip = WM_USER + 233,         ///< to the main window: paste a clip by id
		EditClip = WM_USER + 234           ///< to the main window: edit a clip by id
	};
};


#if !defined(_BITSET_)
#	include <bitset>
#endif // !defined(_BITSET_)

class CICU_String;

/**
 * @brief Marks the case-insensitive matches of a search text in a list row's text (search highlighting).
 */
class CMarkerInserter
{
public:
	/**
	 * @brief Puts markers around each match (at most 101), drops the leading lines before the
	 * first match when it is past the row's lines, and turns the line breaks into unprintable markers.
	 * @param icuString The ICU case mapping (the case-insensitive comparison).
	 * @param mainStr The text; changed in place.
	 * @param findStr The search text; nothing is done when it is empty.
	 * @param preInsert The marker inserted before each match.
	 * @param postInsert The marker inserted after each match.
	 * @param linesPerRow The number of lines a list row shows.
	 * @return The number of matches marked.
	 */
	static int Insert(CICU_String& icuString, CString& mainStr, CString& findStr, CString preInsert, CString postInsert, int linesPerRow);

private:
	/** @brief What the insert step did. */
	struct InsertResult
	{
		/** @brief The number of matches marked. */
		int replaceCount{};
		/** @brief The position of the first match's text in the marked text; 0 without a match. */
		int firstFindPos{};
	};

	/**
	 * @brief Insert's first step: inserts the markers around each match (at most 101).
	 * @param icuString The ICU case mapping (the case-insensitive comparison).
	 * @param mainStr The text; changed in place.
	 * @param findStr The search text, not empty.
	 * @param preInsert The marker inserted before each match.
	 * @param postInsert The marker inserted after each match.
	 * @return The number of matches and where the first one now is.
	 */
	static InsertResult InsertMarkers(CICU_String& icuString, CString& mainStr,CString& findStr, const CString& preInsert, const CString& postInsert);

	/**
	 * @brief Insert's second step: when the first match is past the row's lines, drops the lines
	 * before it (keeping one line before it when a row shows more than one) and prefixes "... ".
	 * @param mainStr The marked text; changed in place.
	 * @param firstFindPos The position of the first match.
	 * @param linesPerRow The number of lines a list row shows.
	 */
	static void TrimLeadingLines(CString& mainStr, int firstFindPos, int linesPerRow);
};

/**
 * @brief Updates the enabled and checked state of a popup menu's items through the command UI
 * handlers of a window (MFC's CFrameWnd::OnInitMenuPopup for windows that are not frames).
 */
class CMenuPopupUpdater
{
public:
	/**
	 * @brief Runs the update handlers of pWnd for each item of the popup.
	 * @param pPopupMenu The popup menu that is about to open.
	 * @param pWnd The window whose handlers update the items.
	 */
	static void Update(CMenu *pPopupMenu, CWnd *pWnd);

private:
	/**
	 * @brief Update's first step: sets state.m_pParentMenu to the menu that holds the popup, when found.
	 * @param state The command UI state.
	 * @param pPopupMenu The popup menu.
	 * @param pWnd The window.
	 */
	static void FindParentMenu(CCmdUI& state, CMenu *pPopupMenu, CWnd *pWnd);

	/**
	 * @brief Update's item step: runs the handler of the item at state.m_nIndex (a sub-popup is
	 * routed to its first item); separators and items that cannot be routed are skipped.
	 * @param state The command UI state, on the item.
	 * @param pPopupMenu The popup menu.
	 * @param pWnd The window.
	 */
	static void UpdateItem(CCmdUI& state, CMenu *pPopupMenu, CWnd *pWnd);

	/**
	 * @brief Adjusts state.m_nIndex and state.m_nIndexMax when the handler deleted or added items.
	 * @param state The command UI state, after the item's handler ran.
	 * @param pPopupMenu The popup menu.
	 */
	static void AdjustForMenuChanges(CCmdUI& state, CMenu *pPopupMenu);
};

#endif // !defined(AFX_CP_GUI_GLOBALS__FBCDED09_A6F2_47EB_873F_50A746EBC86B__INCLUDED_)
