#pragma once

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

#include "../resource.h"       // main symbols
#include "Clip.h"
#include "DatabaseUtilities.h"
#include "Misc.h"
#include "Options.h"
#include "AppServices.h"
#include "..\Shared\ArrayEx.h"
#include "MainFrm.h"
#include "ProcessPaste.h"
#include "MultiLanguage.h"
#include "CopyThread.h"
#include "ClipboardSaveRestore.h"
#include "DittoCopyBuffer.h"
#include "sqlite\CppSQLite3.h"
#include "DittoDb.h"
#include "DittoAddins.h"
#include "externalwindowtracker.h"
#include "HotKeys.h"
#include "UAC_Thread.h"
#include "ICU_String.h"
#include "ClipEditThread.h"
#include <memory>


extern class CCP_MainApp theApp;

class DittoCommandLineInfo;

class CCP_MainApp : public CWinApp
{
public:
	CCP_MainApp();
	~CCP_MainApp();

	/**
	 * @brief The application's services (the composition root): the settings and, later, the
	 *        other services. For MFC windows, dialogs and threads the framework creates; other
	 *        classes get what they need from their owner.
	 * @return The services; they exist from the constructor until the destructor.
	 */
	CAppServices& Services();

private:
	/// The services; declared first so that they are created before and destroyed after every other member.
	std::unique_ptr<CAppServices> m_services{};

public:
	CDittoDb m_db;
	bool m_databaseOnNetworkShare;

	HANDLE	m_hMutex; // for singleton app
	HANDLE m_adminPasteMutex;
	// track stages of startup / shutdown
	bool	m_bAppRunning;
	bool	m_bAppExiting;
	int 	m_connectOnStartup;

	// MainFrame
	HWND m_MainhWnd;
	// non-owning: each frame window deletes itself (CFrameWnd::PostNcDestroy)
	CMainFrame* m_pMainFrame;
	CFrameWnd* m_pNoDbMainFrame;
	bool AfterMainCreate();  // called after main window creation; false: Ditto cannot run (reported)
	void BeforeMainClose();  // called before main window close

// System-wide HotKeys
	// g_HotKeys owns every hot key; the CHotKey* members below are non-owning pointers into it
	CHotKey* m_pDittoHotKey; // activate ditto's qpaste window
	CHotKey* m_pDittoHotKey2; // activate ditto's qpaste window
	CHotKey* m_pDittoHotKey3; // activate ditto's qpaste window

	CHotKey* m_pPosOne;
	CHotKey* m_pPosTwo;
	CHotKey* m_pPosThree;
	CHotKey* m_pPosFour;
	CHotKey* m_pPosFive;
	CHotKey* m_pPosSix;
	CHotKey* m_pPosSeven;
	CHotKey* m_pPosEight;
	CHotKey* m_pPosNine;
	CHotKey* m_pPosTen;

	CHotKey* m_pCopyBuffer1;
	CHotKey* m_pPasteBuffer1;
	CHotKey* m_pCutBuffer1;
	CHotKey* m_pCopyBuffer2;
	CHotKey* m_pPasteBuffer2;
	CHotKey* m_pCutBuffer2;
	CHotKey* m_pCopyBuffer3;
	CHotKey* m_pPasteBuffer3;
	CHotKey* m_pCutBuffer3;

	CHotKey* m_pCopyBuffer4;
	CHotKey* m_pPasteBuffer4;
	CHotKey* m_pCutBuffer4;

	CHotKey* m_pCopyBuffer5;
	CHotKey* m_pPasteBuffer5;
	CHotKey* m_pCutBuffer5;

	CHotKey* m_pTextOnlyPaste;
	CHotKey* m_pSaveClipboard;
	CHotKey* m_pCopyAndSaveClipboard;

	ExternalWindowTracker m_activeWnd;

	CClipEditThread m_editThread;

	// CopyThread and ClipViewer (Copy and Paste Management)
	CCopyThread	m_CopyThread;
	bool StartCopyThread();  // false: the copy thread could not start (reported)
	void StopCopyThread();
	// for posting messages
	HWND GetClipboardViewer() { return m_CopyThread.m_pClipboardViewer->m_hWnd; }
	bool EnableCbCopy(bool bState) { return m_CopyThread.SetCopyOnChange(bState); }
	bool IsClipboardViewerConnected() { return m_CopyThread.IsClipboardViewerConnected(); }
	bool GetConnectCV() { return m_CopyThread.GetConnectCV(); }
	void SetConnectCV(bool bConnect);
	bool ToggleConnectCV();
	void UpdateMenuConnectCV(CMenu* pMenu, UINT nMenuID);
	bool ImportClips(HWND hWnd);
	void LoadGlobalClips();

	void OnDeleteID(long lID);
	BOOL GetClipData(long lID, CClipFormat& Clip);
	bool EditItems(CClipIDs& Ids, bool bShowError, bool forceTextEdit);

	std::unique_ptr<CClipTypes> LoadTypesFromDB(); // a new types array; null when the database read failed
	void ReloadTypes();
	void RefreshView(CopyReasonEnum::CopyReason copyReason = CopyReasonEnum::COPY_TO_UNKOWN); // refreshes the view if it is visible
	void RefreshClipInUI(int clipId, int updateFlags);
	void OnCopyCompleted(long lLastID, int count = 1, CopyReasonEnum::CopyReason copyReason = CopyReasonEnum::COPY_TO_UNKOWN);
	void OnPasteCompleted();

	// Groups
	long		m_GroupDefaultID; // new clips are saved to this group
	long		m_GroupID;        // current group
	long		m_GroupParentID;  // current group's parent
	CString		m_GroupText;      // current group's description

	long		m_oldGroupID;
	long		m_oldGroupParentID;
	CString		m_oldGroupText;

	void SaveCurrentGroupState();
	void ClearOldGroupState();
	BOOL TryEnterOldGroupState();
	BOOL EnterGroupID(long lID, BOOL clearOldGroupState = TRUE, BOOL saveCurrentGroupState = FALSE);
	long GetValidGroupID(); // returns a valid id (not negative)
	void SetGroupDefaultID(long lID); // sets a valid id


// Window States
	// the ID given focus by CQPasteWnd::FillList
	long	m_FocusID;

	bool	m_bShowingQuickPaste;
	bool	m_bRefreshView;

	CString m_Status;
	CQPasteWnd* QPasteWnd();
	HWND QPastehWnd();
	void SetStatus(const TCHAR* status = NULL, bool bRepaintImmediately = false);

	void ShowPersistent(bool bVal);
	bool	m_bAsynchronousRefreshView;

	CLIPFORMAT m_cfIgnoreClipboard; // used by CClip::LoadFromClipboard
	CLIPFORMAT m_excludeClipboardContentFromMonitorProcessing;
	CLIPFORMAT m_canIncludeInClipboardHistory;
	CLIPFORMAT m_cfDelaySavingData;
	CLIPFORMAT m_PingFormat;
	CLIPFORMAT m_HTML_Format;
	CLIPFORMAT m_RTFFormat;
	CLIPFORMAT m_DittoFileData;
	CLIPFORMAT m_PNG_Format;


	COleDateTime m_oldtStartUp;

	CMultiLanguage m_Language;

	CDittoCopyBuffer m_CopyBuffer;
	void PumpMessageEx(HWND hWnd = NULL);

	CDittoAddins m_Addins;

	ULONG_PTR m_gdiplusToken;

	bool UACPaste();
	bool UACCopy();
	bool UACCut();
	bool UACThreadRunning();

	void RefreshShowInTaskBar();

	void SetActiveGroupId(int groupId);
	int GetActiveGroupId();

	void SetCopyReason(CopyReasonEnum::CopyReason copyReason);
	CopyReasonEnum::CopyReason GetCopyReason();

	void CreateMainWnd();
	void CloseNoDbWindow();

	CICU_String m_icuString;

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnIdle(LONG lCount);

protected:
	void ShowCommandLineError(CString csTitle, CString csMessage);
	// Puts an exported clip file given on the command line on the clipboard; reports any error
	void ImportFileFromCommandLine(const CString& fileName);
	std::unique_ptr<CUAC_Thread> m_pUacPasteThread{}; // owned; created on the first UAC paste/copy/cut

	int m_activeGroupId;
	ULONGLONG m_activeGroupStartTime{};

	CopyReasonEnum::CopyReason m_copyReason;
	ULONGLONG m_copyReasonStartTime{};

private:
	/**
	 * @brief Starts the application; InitInstance wraps it as the start-up error boundary.
	 * @return TRUE to run the message loop, FALSE to exit the process.
	 * @throws std::exception When a start-up step fails.
	 */
	BOOL InitInstanceBody();
	/**
	 * @brief EnterGroupID's database step: makes lID the current group when Main holds it as a group.
	 * @param lID The clip id of the group to enter.
	 * @return TRUE when the group was entered, FALSE when lID is missing or not a group.
	 * @throws CppSQLite3Exception When the query fails; EnterGroupID reports it.
	 */
	BOOL EnterStoredGroup(long lID);

	/** @brief A message this instance sends to the running Ditto for a command line switch. */
	struct RunningInstanceRequest
	{
		/** @brief True when the command line asks for this message. */
		bool requested{};
		/** @brief The message. */
		UINT message{};
		/** @brief The message's wParam. */
		WPARAM wParam{};
		/** @brief The message's lParam. */
		LPARAM lParam{};
	};

	/** @brief How EditItems edits one clip: the file kind, its extension and the editor. */
	struct ClipEditTarget
	{
		/** @brief The clip is written as a unicode text file. */
		bool unicodeFile{};
		/** @brief The clip is written as an ANSI text file. */
		bool asciFile{};
		/** @brief The clip is written as an RTF file. */
		bool rtfFile{};
		/** @brief The clip is written as an image file. */
		bool imageFile{};
		/** @brief The editor set in the options; empty for the system mapping (or the internal editor for text). */
		CString exePath{};
		/** @brief The file extension (txt, rtf, png or bmp). */
		CString extension{};
	};

	/**
	 * @brief InitInstanceBody's command line step (restart, import, connect, or a request to the running Ditto).
	 * @param cmdInfo The parsed command line.
	 * @return False when this instance must exit (InitInstance returns FALSE).
	 */
	bool HandleCommandLine(const DittoCommandLineInfo& cmdInfo);

	/**
	 * @brief Handles /Connect or /Disconnect: passes it to the running Ditto, else starts this instance (dis)connected.
	 * @param cmdInfo The parsed command line.
	 * @return False when the running Ditto handled it (this instance exits).
	 */
	bool HandleConnectSwitch(const DittoCommandLineInfo& cmdInfo);

	/**
	 * @brief Sends the first requested command line request (open/close, exit, plain text paste, paste or edit a clip) to the running Ditto.
	 * @param cmdInfo The parsed command line.
	 * @return True when the command line held such a request (this instance exits).
	 */
	bool ForwardToRunningInstance(const DittoCommandLineInfo& cmdInfo);

	/**
	 * @brief Registers with the restart manager and creates the single instance mutex.
	 * @return False when Ditto already runs (the running one is asked to show its tray icon).
	 */
	bool CreateSingleInstanceMutex();

	/**
	 * @brief EnterGroupID's step: makes lID (-1 for the history) the current group.
	 * @param lID The group's clip id, or -1.
	 * @param bResult Receives TRUE when the group was entered.
	 * @return False when reading the group failed (reported); EnterGroupID then returns FALSE.
	 */
	bool OpenGroup(long lID, BOOL& bResult);

	/**
	 * @brief EnterGroupID's last step: refreshes the view when the group was entered and logs a slow switch.
	 * @param bResult TRUE when the group was entered.
	 * @param startTick GetTickCount64 at the start of EnterGroupID.
	 */
	void FinishEnterGroup(BOOL bResult, ULONGLONG startTick);

	/**
	 * @brief EditItems' step for one clip: writes it to a file and opens the editor (or the internal editor).
	 * @param id The clip id; -1 for a new clip.
	 * @param forceTextEdit True: edit RTF clips as text.
	 * @param lastFileCheckId In/out: the next number tried for a new clip's file name.
	 * @return True when an external editor was launched.
	 */
	bool EditItem(int id, bool forceTextEdit, int& lastFileCheckId);

	/**
	 * @brief Chooses the file kind and editor of a clip from its formats.
	 * @param clip The clip, its formats loaded.
	 * @param id The clip id; -1 for a new clip.
	 * @param forceTextEdit True: edit RTF clips as text.
	 * @param target Receives the choice.
	 * @return False when the clip has no editable format.
	 */
	bool ChooseClipEditTarget(CClip& clip, int id, bool forceTextEdit, ClipEditTarget& target);

	/**
	 * @brief Opens a text or RTF clip in the internal editor when no external editor is set.
	 * @param target The clip's edit target.
	 * @param id The clip id.
	 * @return True when the internal editor took the clip.
	 */
	bool EditInInternalEditor(const ClipEditTarget& target, int id);

	/**
	 * @brief The file a clip is edited in; a new clip gets the first free NewClip_n name.
	 * @param id The clip id; negative for a new clip.
	 * @param extension The file extension.
	 * @param lastFileCheckId In/out: the next number tried for a new clip's file name.
	 * @return The file path (empty when no free name was found).
	 */
	CString MakeEditFilePath(int id, const CString& extension, int& lastFileCheckId);

	/**
	 * @brief Opens a clip's file in the editor (or with the system mapping).
	 * @param exePath The editor; empty for the system mapping.
	 * @param savePath The clip's file.
	 * @param id The clip id (for the log).
	 * @return False when ShellExecuteEx failed.
	 */
	bool LaunchClipEditor(const CString& exePath, const CString& savePath, int id);
};