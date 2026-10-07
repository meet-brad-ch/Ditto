#pragma once

#include "Theme.h"
#include "RegExFilterHelper.h"
#include "ClipSavePolicy.h"
#include <array>
#include <set>
#include <vector>

class CCopyBufferItem
{
public:
	CCopyBufferItem()
	{
		m_lCopyHotKey = -1;
		m_lPasteHotKey = -1;
		m_lCutHotKey = -1;
		m_bPlaySoundOnCopy = FALSE;
	}
	long m_lCopyHotKey;
	long m_lPasteHotKey;
	long m_lCutHotKey;
	BOOL m_bPlaySoundOnCopy;
};

class CGetSetOptions
{
public:
	/** @brief The registry key of the settings under HKEY_CURRENT_USER. */
	static constexpr const TCHAR* RegPath = _T("Software\\Ditto");

	/** @brief Where the quick paste window opens (the stored ShowQuickPastePosition value). */
	enum : int
	{
		PosAtCaret = 1,    ///< at the caret of the focused window
		PosAtCursor = 2,   ///< at the mouse cursor
		PosAtPrevious = 3  ///< where it was last
	};

	/** @brief The side of a Ditto window its caption bar is on (the stored CaptionPos value). */
	enum : int
	{
		CaptionOnRight = 1,  ///< on the right
		CaptionOnBottom = 2, ///< at the bottom
		CaptionOnLeft = 3,   ///< on the left
		CaptionOnTop = 4     ///< at the top
	};

	/** @brief What double-clicking the caption does (the stored DoubleClickingOnCaptionDoes value). */
	enum : int
	{
		TogglesAlwaysOnTop = 1,           ///< toggles always on top
		TogglesAlwaysShowDescription = 2, ///< toggles always showing the description
		RollsUpWindow = 3                 ///< rolls the window up
	};

	/** @brief The folder ids of GetPath. */
	enum : int
	{
		PathHelp = 0,          ///< the help files
		PathRemoteFiles = 1,   ///< files received from other computers
		PathLogFile = 2,       ///< the log file's folder
		PathLanguage = 3,      ///< the language files
		PathThemes = 8,        ///< the theme files
		PathAddins = 9,        ///< the add-in DLLs
		PathDragFiles = 10,    ///< files written for drag and drop
		PathClipDiff = 11,     ///< files written for clip compare
		PathRestoreTemp = 12,  ///< the temporary copy of a database to restore
		PathEditClips = 13     ///< files written for editing clips
	};

	CGetSetOptions();
	virtual ~CGetSetOptions();

	static bool m_bFromIni;
	static CString m_csIniFileName;
	static bool m_bInConversion;
	static CTheme m_Theme;
	static bool m_portable;
	static bool m_windowsApp;
	static bool m_chocolateyApp;

	static void LoadSettings();
	static CString GetIniFileName(bool bLocalIniFile);
	static void ConverSettingsToIni();
	static CString GetAppDataPath();
	static CString GetTempFilePath();
	static void CreateIniFile(CString path);

	static CString GetExeFileName();
	static CString GetAppName();

	static BOOL GetShowIconInSysTray();
	static BOOL SetShowIconInSysTray(BOOL bShow);

	static BOOL GetRunOnStartUp();
	static void SetRunOnStartUp(BOOL bRun);

	static BOOL SetProfileFont(CString csSection, LOGFONT &font);
	static BOOL GetProfileFont(CString csSection, LOGFONT &font);

	static long GetResolutionProfileLong(CString csName, long lDefaultValue, CString csNewPath = _T(""));
	static BOOL SetResolutionProfileLong(CString csName, long lValue);

	static BOOL SetProfileLong(CString csName, long lValue);
	static long GetProfileLong(CString csName, long lDefaultValue = -1, CString csNewPath = _T(""));

	static CString GetProfileString(CString csName, CString csDefault, CString csNewPath = _T(""), int maxSize = -1);
	static BOOL	SetProfileString(CString csName, CString csValue);

	// The registry value's bytes; empty when the value does not exist or cannot be read
	static std::vector<BYTE> GetProfileData(CString csName);
	static BOOL	SetProfileData(CString csName, LPVOID lpData, DWORD dwLength);

	static BOOL SetQuickPasteSize(CSize size);
	static void GetQuickPasteSize(CSize &size);

	static BOOL SetQuickPastePoint(CPoint point);
	static void GetQuickPastePoint(CPoint &point);

	static BOOL SetEnableTransparency(BOOL bCheck);
	static BOOL GetEnableTransparency();

	static BOOL SetTransparencyPercent(long lPercent);
	static long GetTransparencyPercent();

	static long m_nLinesPerRow;
	static BOOL SetLinesPerRow(long lLines);
	static long GetLinesPerRow();

	static BOOL SetQuickPastePosition(long lPosition);
	static long GetQuickPastePosition();

	static long GetCopyGap();
	static void SetCopyGap(long lGap);

	static BOOL SetDBPath(CString csPath);
	static CString GetDBPath(bool resolvePath = true);
	// Folder of the default database: the app data folder, or empty (the exe folder) when portable
	static CString GetDefaultDBDirectory();
	static CString ResolvePath(CString path);


	static void SetCheckForMaxEntries(BOOL bVal);
	static BOOL GetCheckForMaxEntries();

	static void SetCheckForExpiredEntries(BOOL bVal);
	static BOOL GetCheckForExpiredEntries();

	static void SetMaxEntries(long lVal);
	static long GetMaxEntries();

	static void SetExpiredEntries(long lVal);
	static long GetExpiredEntries();

	static void SetTripCopyCount(long lVal);
	static long GetTripCopyCount();

	static void SetTripPasteCount(long lVal);
	static long GetTripPasteCount();

	static void SetTripDate(long lDate);
	static long GetTripDate();

	static void SetTotalCopyCount(long lVal);
	static long GetTotalCopyCount();

	static void SetTotalPasteCount(long lVal);
	static long GetTotalPasteCount();

	static void SetTotalDate(long lDate);
	static long GetTotalDate();

	static CString	GetUpdateFilePath();
	static BOOL		SetUpdateFilePath(CString cs);

	static CString	GetUpdateInstallPath();
	static BOOL		SetUpdateInstallPath(CString cs);

	static long		GetLastUpdate();
	static long		SetLastUpdate(long lValue);

	static BOOL		GetCheckForUpdates();
	static BOOL		SetCheckForUpdates(BOOL bCheck);

	static BOOL		m_bUseCtrlNumAccel;
	static void		SetUseCtrlNumForFirstTenHotKeys(BOOL bVal);
	static BOOL		GetUseCtrlNumForFirstTenHotKeys();

	static BOOL		m_bAllowDuplicates;
	static void		SetAllowDuplicates(BOOL bVal);
	static BOOL		GetAllowDuplicates();

	static BOOL		m_bUpdateTimeOnPaste;
	static void		SetUpdateTimeOnPaste(BOOL bVal);
	static BOOL		GetUpdateTimeOnPaste();

	static BOOL		m_bSaveMultiPaste;
	static void		SetSaveMultiPaste(BOOL bVal);
	static BOOL		GetSaveMultiPaste();

	static BOOL		m_bShowPersistent;
	static void		SetShowPersistent(BOOL bVal);
	static BOOL		GetShowPersistent();

	static BOOL		m_bHideDittoOnPaste;
	static void		SetHideDittoOnPaste(BOOL bVal);
	static BOOL		GetHideDittoOnPaste();

	static void		SetShowTextForFirstTenHotKeys(BOOL bVal);
	static BOOL		GetShowTextForFirstTenHotKeys();

	static void		SetMainHWND(long lhWnd);
	static long		GetMainHWND();

	static void		SetCaptionPos(long lPos);
	static long		GetCaptionPos();

	static void		SetAutoHide(BOOL bAutoHide);
	static BOOL		GetAutoHide();

	static long		m_bDescTextSize;
	static void		SetDescTextSize(long lSize);
	static long		GetDescTextSize();

	static BOOL		m_bDescShowLeadingWhiteSpace;
	static void		SetDescShowLeadingWhiteSpace(BOOL bVal);
	static BOOL		GetDescShowLeadingWhiteSpace();

	static BOOL		m_bAllwaysShowDescription;
	static void		SetAllwaysShowDescription(long bShow);
	static BOOL		GetAllwaysShowDescription();

	static long		m_bDoubleClickingOnCaptionDoes;
	static void		SetDoubleClickingOnCaptionDoes(long lOption);
	static long		GetDoubleClickingOnCaptionDoes();

	static BOOL		m_bPrompForNewGroupName;
	static void		SetPrompForNewGroupName(BOOL bOption);
	static BOOL		GetPrompForNewGroupName();

	static BOOL		m_bSendPasteOnFirstTenHotKeys;
	static void		SetSendPasteOnFirstTenHotKeys(BOOL bOption);
	static BOOL		GetSendPasteOnFirstTenHotKeys();


	static BOOL		m_HideDittoOnHotKeyIfAlreadyShown;
	static BOOL		GetHideDittoOnHotKeyIfAlreadyShown();
	static void		SetHideDittoOnHotKeyIfAlreadyShown(BOOL bVal);

	static BOOL		GetFont(LOGFONT &font);
	static void		SetFont(LOGFONT &font);

	static BOOL		m_bDrawThumbnail;
	static void		SetDrawThumbnail(long bDraw);
	static BOOL		GetDrawThumbnail();

	static BOOL		m_bFastThumbnailMode;
	static void		SetFastThumbnailMode(BOOL bval);
	static BOOL		GetFastThumbnailMode();

	static BOOL		m_bDrawRTF;
	static void		SetDrawRTF(long bDraw);
	static BOOL		GetDrawRTF();

	static BOOL		m_bMultiPasteReverse;
	static void		SetMultiPasteReverse(BOOL bVal);
	static BOOL		GetMultiPasteReverse();

	static CString	m_csPlaySoundOnCopy;
	static void		SetPlaySoundOnCopy(CString cs);
	static CString	GetPlaySoundOnCopy();

	static BOOL		m_bSendPasteMessageAfterSelection;
	static void		SetSendPasteAfterSelection(BOOL bVal);
	static BOOL		GetSendPasteAfterSelection();

	static BOOL		m_bFindAsYouType;
	static void		SetFindAsYouType(BOOL bVal);
	static BOOL		GetFindAsYouType();

	static BOOL		m_bEnsureEntireWindowCanBeSeen;
	static void		SetEnsureEntireWindowCanBeSeen(BOOL bVal);
	static BOOL		GetEnsureEntireWindowCanBeSeen();

	static BOOL		m_bShowAllClipsInMainList;
	static void		SetShowAllClipsInMainList(BOOL bVal);
	static BOOL		GetShowAllClipsInMainList();


	static long		m_lMaxClipSizeInBytes;
	static long		GetMaxClipSizeInBytes();
	static void		SetMaxClipSizeInBytes(long lSize);

	static CString	GetLanguageFile();
	static void		SetLanguageFile(CString csLanguage);

	static DWORD	m_dwSaveClipDelay;
	static ULONG	GetSaveClipDelay();
	static void		SetSaveClipDelay(DWORD dwDelay);

	static long		m_lProcessDrawClipboardDelay;
	static long		GetProcessDrawClipboardDelay();
	static void		SetProcessDrawClipboardDelay(long lDelay);

	static BOOL		m_bEnableDebugLogging;
	static BOOL		GetEnableDebugLogging();
	static void		SetEnableDebugLogging(BOOL bEnable);

	static BOOL		m_bEnsureConnectToClipboard;
	static BOOL		GetEnsureConnectToClipboard();
	static void		SetEnsureConnectToClipboard(BOOL bSet);

	static BOOL		GetPromptWhenDeletingClips();
	static void		SetPromptWhenDeletingClips(BOOL bSet);

	static CString	GetLastImportDir();
	static void		SetLastImportDir(CString csDir);

	static CString	GetLastExportDir();
	static void		SetLastExportDir(CString csDir);

	static BOOL		GetUpdateDescWhenSavingClip();
	static void		SetUpdateDescWhenSavingClip(BOOL bSet);

	static BOOL		m_outputDebugStringLogging;
	static BOOL		GetEnableOutputDebugStringLogging();
	static void		SetEnableOutputDebugStringLogging(BOOL bSet);


	static CString  GetPath(long lPathID);

	static __int64	nLastDbWriteTime;

	static long		GetDittoRestoreClipboardDelay();
	static void		SetDittoRestoreClipboardDelay(long lDelay);

	static void		GetCopyBufferItem(int nPos, CCopyBufferItem &Item);
	static void		SetCopyBufferItem(int nPos, CCopyBufferItem &Item);

	static CString  GetMultiPasteSeparator(bool bConvertToLineFeeds = true);
	static void		SetMultiPasteSeparator(CString csSep);

	static BOOL		GetSetCurrentDirectory();

	static CString GetPasteString(CString csAppName);

	static CString GetDefaultPasteString();
	static void SetDefaultPasteString(CString val);

	static CString GetCopyString(CString csAppName);
	static CString GetDefaultCopyString();
	static void SetDefaultCopyString(CString val);

	static CString GetCutString(CString csAppName);
	static CString GetDefaultCutString();
	static void SetDefaultCutString(CString val);

	static BOOL	GetEditWordWrap();
	static void	SetEditWordWrap(BOOL bSet);


	static bool		GetIsPortableDitto();
	static bool		GetIsWindowsApp();
	static bool		GetIsChocolateyApp();

	static long		GetAutoMaxDelay();
	static void		SetAutoMaxDelay(long lDelay);

	static void SetTheme(CString csTheme);
	static CString GetTheme();

	static long		GetKeyStateWaitTimerCount();
	static long		GetKeyStatePasteDelay();

	static DWORD	GetDittoHotKey();
	
	static DWORD	SendKeysDelay();
	static void		SetSendKeysDelay(DWORD val);

	static DWORD	RealSendKeysDelay();
	static void		SetRealSendKeysDelay(DWORD val);

	static DWORD	WaitForActiveWndTimeout();
	static DWORD	FocusChangedDelay();
	static DWORD	FocusWndTimerTimeout();

	static BOOL		GetConnectedToClipboard();
	static void		SetConnectedToClipboard(BOOL val);

	static DWORD	GetTextOnlyRestoreDelay();
	static DWORD 	GetTextOnlyPasteDelay();

	static BOOL		GetSetFocusToApp(CString csAppName);

	static DWORD	SelectedIndex();
	static void		SetSelectedIndex(int val);

	static void		SetCopyAppInclude(CString csAppName);
	static CString  GetCopyAppInclude();

	static void		SetCopyAppExclude(CString csAppName);
	static CString  GetCopyAppExclude();

	static CString  GetCopyAppSeparator();

	static DWORD	GetNoFormatsRetryDelay();

	static DWORD	GetMainDeletesDeleteCount();

	static DWORD	GetIdleSecondsBeforeDelete();

	static DWORD	GetDbTimeout();

	static DWORD	GetFunnyTickCountAdjustment();

	static DWORD	GetMinIdleTimeBeforeTrackFocus();

	static DWORD	GetTimeBeforeExpandWindow();

	static DWORD	GetUseGuiThreadInfoForFocus();

	static void		SetSearchDescription(BOOL val);
	static BOOL		GetSearchDescription();

	static void		SetSearchFullText(BOOL val);
	static BOOL		GetSearchFullText();

	static void		SetSearchQuickPaste(BOOL val);
	static BOOL		GetSearchQuickPaste();

	static void		SetSimpleTextSearch(BOOL val);
	static BOOL		GetSimpleTextSearch();

	static void		SetMoveClipsOnGlobal10(BOOL val);
	static BOOL		GetMoveClipsOnGlobal10();

	static void		SetShowScrollBar(BOOL val);
	static BOOL		GetShowScrollBar();
	static BOOL		m_showScrollBar;

	static void		SetUseModernScrollBar(BOOL val);
	static BOOL		GetUseModernScrollBar();
	static BOOL		m_useModernScrollBar;

	static void		SetPasteAsAdmin(BOOL val);
	static BOOL		GetPasteAsAdmin();

	static void		SetRememberDescPos(BOOL val);
	static BOOL		GetRememberDescPos();

	static void		SetSizeDescWindowToContent(BOOL val);
	static BOOL		GetSizeDescWindowToContent();

	static void		SetScaleImagesToDescWindow(BOOL val);
	static BOOL		GetScaleImagesToDescWindow();

	static void		SetDescWndPoint(CPoint point);
	static void		GetDescWndPoint(CPoint &point);

	static void		SetDescWndSize(CSize size);
	static void		GetDescWndSize(CSize &size);

	static void		SetShowInTaskBar(BOOL val);
	static BOOL		GetShowInTaskBar();

	static void		SetHideTaskbarIconOnClose(BOOL val);
	static BOOL		GetHideTaskbarIconOnClose();

	static void		SetDiffApp(CString val);
	static CString	GetDiffApp();

	static void		SetQRCodeBorderPixels(int val);
	static int	GetQRCodeBorderPixels();

	static BOOL GetRegExTextSearch();
	static void SetRegExTextSearch(BOOL val);




	static int ReadRandomFileInterval();
	static int ReadRandomFileIdleMin();

	static BOOL GetShowGroupsInMainList();
	static void SetShowGroupsInMainList(BOOL val);

	static void SetGroupDoubleClickTimeMS(int val);
	static int GetGroupDoubleClickTimeMS();

	static void SetSaveToGroupTimeoutMS(int val);
	static int GetSaveToGroupTimeoutMS();

	static void SetCopyReasonTimeoutMS(int val);
	static int GetCopyReasonTimeoutMS();

	static void SetWindowsResumeDelayReOpenDbMS(int val);
	static int GetWindowsResumeDelayReOpenDbMS();

	static BOOL GetShowMsgWndOnCopyToGroup();
	static void SetShowMsgWndOnCopyToGroup(BOOL val);

	static int GetActionShortCutA(DWORD action, int pos);
	static void SetActionShortCutA(int action, DWORD shortcut, int pos);

	static int GetActionShortCutB(DWORD action, int pos);
	static void SetActionShortCutB(int action, DWORD shortcut, int pos);

	static BOOL	m_bShowAlwaysOnTopWarning;
	static BOOL GetShowAlwaysOnTopWarning();
	static void SetShowAlwaysOnTopWarning(BOOL show);
	
	static BOOL GetUseIPFromAccept();
	static void SetUseIPFromAccept(BOOL useAccept);

	static int GetDragId();
	static void SetDragId(int id);

	static BOOL GetShowIfClipWasPasted();
	static void SetShowIfClipWasPasted(BOOL val);

	static int GetLastGroupToggle();
	static void SetLastGroupToggle(int val);

	static BOOL GetMouseClickHidesDescription();
	static void SetMouseClickHidesDescription(int val);

	static BOOL GetWrapDescriptionText();
	static void SetWrapDescriptionText(int val);

	static BOOL GetUseUISelectedGroupForLastTenCopies();
	static void SetUseUISelectedGroupForLastTenCopies(int val);

	static int GetDelayRenderLockout();
	static void SetDelayRenderLockout(int val);

	static BOOL GetAdjustClipsForCRC();
	static void SetAdjustClipsForCRC(int val);


	static int GetBalloonTimeout();
	static void SetBalloonTimeout(int val);


	static int GetMaxFileContentsSize();
	static void SetMaxFileContentsSize(int val);

	static int GetErrorMsgPopupTimeout();
	static void SetErrorMsgPopupTimeout(int val);

	static CRegExFilterHelper m_regexHelper;
	static void		SetRegexFilter(CString val, int pos);
	static CString	GetRegexFilter(int pos);

	static void SetRegexFilterByProcessName(CString val, int pos);
	static CString GetRegexFilterByProcessName(int pos);

	static BOOL GetOpenToGroupByActiveExe();
	static void SetOpenToGroupByActiveExe(int val);

	static BOOL GetShowStartupMessage();
	static void SetShowStartupMessage(int val);

	static long m_tooltipTimeout;
	static long GetToolTipTimeout();
	static void SetToolTipTimeout(int long);

	static CString GetPastSearchXml();
	static void SetPastSearchXml(CString val);


	static BOOL m_cleanRTFBeforeDrawing;
	static BOOL GetCleanRTFBeforeDrawing();
	static void SetCleanRTFBeforeDrawing(BOOL val);

	static BOOL GetDisableExpireClipsConfig();
	static void SetDisableExpireClipsConfig(BOOL val);

	static BOOL GetRevertToTopLevelGroup();
	static void SetRevertToTopLevelGroup(BOOL val);

	static BOOL GetUpdateClipOrderOnCtrlC();
	static void SetUpdateClipOrderOnCtrlC(BOOL val);

	static int GetMaxToolTipLines();
	static void SetMaxToolTipLines(int val);

	static int GetMaxToolTipCharacters();
	static void SetMaxToolTipCharacters(int val);

	static int m_doubleKeyStrokeTimeout;
	static int GetDoubleKeyStrokeTimeout();
	static void SetDoubleKeyStrokeTimeout(int val);

	static int m_firstTenHotKeysStart;
	static int GetFirstTenHotKeysStart();
	static void SetFirstTenHotKeysStart(int val);

	static int m_firstTenHotKeysFontSize;
	static int GetFirstTenHotKeysFontSize();
	static void SetFirstTenHotKeysFontSize(int val);

	static BOOL GetAddCFHDROP_OnDrag();
	static void SetAddCFHDROP_OnDrag(BOOL val);

	static int GetCopyAndSveDelay();
	static void SetCopyAndSveDelay(int val);

	static int GetEditorDefaultFontSize();
	static void SetEditorDefaultFontSize(int val);

	static BOOL m_moveSelectionOnOpenHotkey;
	static BOOL GetMoveSelectionOnOpenHotkey();
	static void SetMoveSelectionOnOpenHotkey(BOOL val);

	static BOOL m_allowBackToBackDuplicates;
	static BOOL GetAllowBackToBackDuplicates();
	static void SetAllowBackToBackDuplicates(BOOL val);

	static BOOL m_maintainSearchView;
	static BOOL GetMaintainSearchView();
	static void SetMaintainSearchView(BOOL val);


	static CString m_tempDragFileName;
	static CTime m_tempDragFileNameSetTime;
	static CString GetTempDragFileName();
	static void SetTempDragFileName(CString val);

	static BOOL m_refreshViewAfterPasting;
	static BOOL GetRefreshViewAfterPasting();
	static void SetRefreshViewAfterPasting(BOOL val);

	static CString GetSlugifySeparator();
	static void SetSlugifySeparator(CString val);

	static BOOL m_supportAllTypes;
	static BOOL GetSupportAllTypes();
	static void SetSupportAllTypes(BOOL val);

	static CString GetIgnoreAnnoyingCFDIB(BOOL useCache = FALSE);
	static CString m_ignoreAnnoyingCFDIB;
	static void SetIgnoreAnnoyingCFDIB(CString val);
	static std::set<CString> GetIgnoreAnnoyingCFDIBSet(BOOL useCache = FALSE);
	// The options that decide how a copied clip is saved, for injection into CClip
	static DittoCore::ClipSaveSettings GetClipSaveSettings();

	static BOOL GetRegexCaseInsensitive();
	static void SetRegexCaseInsensitive(BOOL val);

	static BOOL		m_bDrawCopiedColorCode;
	static void		SetDrawCopiedColorCode(long bDraw);
	static BOOL		GetDrawCopiedColorCode();


	static BOOL m_centerWindowBelowCursorOrCaret;
	static void SetCenterWindowBelowCursorOrCaret(BOOL center);
	static BOOL GetCenterWindowBelowCursorOrCaret();

	static BOOL SetTextEditorPath(CString path);
	static CString GetTextEditorPath();

	static BOOL SetImageEditorPath(CString path);
	static CString GetImageEditorPath();

	static BOOL SetRTFEditorPath(CString path);
	static CString GetRTFEditorPath();



	static void SetPreferUtf8ForCompare(BOOL val);
	static BOOL GetPreferUtf8ForCompare();

	static int	m_clipEditSaveDelayAfterLoadSeconds;
	static void SetClipEditSaveDelayAfterLoadSeconds(int val);
	static BOOL GetClipEditSaveDelayAfterLoadSeconds();

	static int	m_clipEditSaveDelayAfterSaveSeconds;
	static void SetClipEditSaveDelayAfterSaveSeconds(int val);
	static BOOL GetClipEditSaveDelayAfterSaveSeconds();

	static BOOL m_bDoNotHideOnDeactivate;
	static void SetDoNotHideOnDeactivate(BOOL val);
	static BOOL GetDoNotHideOnDeactivate();

	static BOOL SetEditWndSize(CSize size);
	static void GetEditWndSize(CSize& size);

	static BOOL SetEditWndPoint(CPoint point);
	static void GetEditWndPoint(CPoint& point);

	static BOOL m_enforceClipboardIgnoreFormats;
	static void SetEnforceClipboardIgnoreFormats(BOOL val);
	static BOOL GetEnforceClipboardIgnoreFormats();

private:
	/** @brief The folder a GetPath folder starts from. */
	enum class PathRoot
	{
		ExeDir,                 ///< the exe's folder
		AppDataUnlessPortable,  ///< the app data folder, the exe's folder for portable Ditto
		TempUnlessPortable      ///< the temp folder, the exe's folder for portable Ditto
	};

	/** @brief How GetPath builds the folder of one Path* id. */
	struct PathRule
	{
		/** @brief The Path* id. */
		long pathId{};
		/** @brief The folder the path starts from. */
		PathRoot root{};
		/** @brief The sub folder appended to the root (empty for none). */
		const TCHAR* subDir{};
	};

	/** @brief GetPath's folders by Path* id; an id without a rule is the exe's folder. */
	static constexpr std::array<PathRule, 10> s_pathRules{ {
		{ PathHelp, PathRoot::ExeDir, _T("Help\\") },
		{ PathLanguage, PathRoot::ExeDir, _T("language\\") },
		{ PathThemes, PathRoot::ExeDir, _T("Themes\\") },
		{ PathLogFile, PathRoot::AppDataUnlessPortable, _T("") },
		{ PathAddins, PathRoot::ExeDir, _T("Addins\\") },
		{ PathRemoteFiles, PathRoot::TempUnlessPortable, _T("ReceivedFiles\\") },
		{ PathDragFiles, PathRoot::TempUnlessPortable, _T("DragFiles\\") },
		{ PathClipDiff, PathRoot::TempUnlessPortable, _T("ClipCompare\\") },
		{ PathRestoreTemp, PathRoot::TempUnlessPortable, _T("RestoreDb\\") },
		{ PathEditClips, PathRoot::TempUnlessPortable, _T("EditClips\\") },
	} };

	/**
	 * @brief GetPath's step for a known id: moves to the rule's root folder and appends its sub folder.
	 * @param rule The id's rule.
	 * @param csDir In: the exe's folder; out: the folder of the id.
	 */
	static void ApplyPathRule(const PathRule& rule, CString& csDir);

	/**
	 * @brief LoadSettings' step: finds the ini file (Windows Store, Chocolatey, portable or app data) and sets m_csIniFileName, m_bFromIni and the app kind flags.
	 * @param exeDir The exe's folder, with a trailing backslash.
	 */
	static void LocateIniFile(const CString& exeDir);

	/** @brief LocateIniFile's step for a plain install: the ini file next to the exe (portable) or in app data. */
	static void LocatePortableOrAppDataIniFile();

	/**
	 * @brief GetProfileString's ini file read; the buffer grows until the value fits (or maxSize is reached).
	 * @param csName The value name.
	 * @param csDefault The value when the ini file has none.
	 * @param csNewPath The section; empty for "Ditto".
	 * @param maxSize The most characters to read, -1 for no limit.
	 * @return The value.
	 */
	static CString GetIniProfileString(const CString& csName, const CString& csDefault, const CString& csNewPath, int maxSize);

	/**
	 * @brief GetProfileString's registry read under HKCU\\Software\\Ditto.
	 * @param csName The value name.
	 * @param csDefault The value when the registry has none or the read fails.
	 * @param csNewPath The sub key; empty for none.
	 * @param maxSize The most characters to read, -1 for no limit.
	 * @return The value.
	 */
	static CString GetRegistryProfileString(const CString& csName, const CString& csDefault, const CString& csNewPath, int maxSize);
};

// global for easy access and for initialization of fast access variables
extern CGetSetOptions g_Opt; 
