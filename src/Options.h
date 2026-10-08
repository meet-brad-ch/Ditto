#pragma once

#include "Theme.h"
#include "RegExFilterHelper.h"
#include "ClipSavePolicy.h"
#include "ISettingsStore.h"
#include <array>
#include <memory>
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

	/**
	 * @brief Creates the settings over the registry (HKCU\\Software\\Ditto); LoadSettings
	 *        switches to the ini file when it finds one.
	 */
	CGetSetOptions();

	/**
	 * @brief Creates the settings over a given store (tests); LoadSettings still switches to the
	 *        ini file when it finds one.
	 * @param store The store; not null.
	 * @throws std::invalid_argument When @p store is null.
	 */
	explicit CGetSetOptions(std::unique_ptr<DittoCore::ISettingsStore> store);

	/** @brief Destroys the settings. */
	virtual ~CGetSetOptions();

	CGetSetOptions(const CGetSetOptions&) = delete;
	CGetSetOptions& operator=(const CGetSetOptions&) = delete;

	bool m_bFromIni{};
	CString m_csIniFileName{};
	CTheme m_Theme{};
	bool m_portable{};
	bool m_windowsApp{};
	bool m_chocolateyApp{};

	void LoadSettings();
	CString GetIniFileName(bool bLocalIniFile);
	CString GetAppDataPath();
	CString GetTempFilePath();
	/**
	 * @brief Creates the ini file as UTF-16 (with a byte order mark) when it does not exist, so
	 *        Unicode settings can be written to it.
	 * @param path The ini file.
	 * @throws std::runtime_error When the file cannot be created or written (the start-up boundary
	 *         shows it).
	 */
	void CreateIniFile(CString path);

	CString GetExeFileName();
	CString GetAppName();

	BOOL GetShowIconInSysTray();
	BOOL SetShowIconInSysTray(BOOL bShow);

	BOOL GetRunOnStartUp();
	void SetRunOnStartUp(BOOL bRun);

	BOOL SetProfileFont(CString csSection, LOGFONT &font);
	BOOL GetProfileFont(CString csSection, LOGFONT &font);

	long GetResolutionProfileLong(CString csName, long lDefaultValue, CString csNewPath = _T(""));
	BOOL SetResolutionProfileLong(CString csName, long lValue);

	BOOL SetProfileLong(CString csName, long lValue);
	long GetProfileLong(CString csName, long lDefaultValue = -1, CString csNewPath = _T(""));

	CString GetProfileString(CString csName, CString csDefault, CString csNewPath = _T(""), int maxSize = -1);
	BOOL	SetProfileString(CString csName, CString csValue);

	// The registry value's bytes; empty when the value does not exist or cannot be read
	std::vector<BYTE> GetProfileData(CString csName);
	BOOL	SetProfileData(CString csName, LPVOID lpData, DWORD dwLength);

	BOOL SetQuickPasteSize(CSize size);
	void GetQuickPasteSize(CSize &size);

	BOOL SetQuickPastePoint(CPoint point);
	void GetQuickPastePoint(CPoint &point);

	BOOL SetEnableTransparency(BOOL bCheck);
	BOOL GetEnableTransparency();

	BOOL SetTransparencyPercent(long lPercent);
	long GetTransparencyPercent();

	long m_nLinesPerRow{};
	BOOL SetLinesPerRow(long lLines);
	long GetLinesPerRow();

	BOOL SetQuickPastePosition(long lPosition);
	long GetQuickPastePosition();

	long GetCopyGap();
	void SetCopyGap(long lGap);

	BOOL SetDBPath(CString csPath);
	CString GetDBPath(bool resolvePath = true);
	// Folder of the default database: the app data folder, or empty (the exe folder) when portable
	CString GetDefaultDBDirectory();
	CString ResolvePath(CString path);


	void SetCheckForMaxEntries(BOOL bVal);
	BOOL GetCheckForMaxEntries();

	void SetCheckForExpiredEntries(BOOL bVal);
	BOOL GetCheckForExpiredEntries();

	void SetMaxEntries(long lVal);
	long GetMaxEntries();

	void SetExpiredEntries(long lVal);
	long GetExpiredEntries();

	void SetTripCopyCount(long lVal);
	long GetTripCopyCount();

	void SetTripPasteCount(long lVal);
	long GetTripPasteCount();

	void SetTripDate(long lDate);
	long GetTripDate();

	void SetTotalCopyCount(long lVal);
	long GetTotalCopyCount();

	void SetTotalPasteCount(long lVal);
	long GetTotalPasteCount();

	void SetTotalDate(long lDate);
	long GetTotalDate();

	CString	GetUpdateFilePath();
	BOOL		SetUpdateFilePath(CString cs);

	CString	GetUpdateInstallPath();
	BOOL		SetUpdateInstallPath(CString cs);

	long		GetLastUpdate();
	long		SetLastUpdate(long lValue);

	BOOL		GetCheckForUpdates();
	BOOL		SetCheckForUpdates(BOOL bCheck);

	BOOL		m_bUseCtrlNumAccel{};
	void		SetUseCtrlNumForFirstTenHotKeys(BOOL bVal);
	BOOL		GetUseCtrlNumForFirstTenHotKeys();

	BOOL		m_bAllowDuplicates{};
	void		SetAllowDuplicates(BOOL bVal);
	BOOL		GetAllowDuplicates();

	BOOL		m_bUpdateTimeOnPaste{};
	void		SetUpdateTimeOnPaste(BOOL bVal);
	BOOL		GetUpdateTimeOnPaste();

	BOOL		m_bSaveMultiPaste{};
	void		SetSaveMultiPaste(BOOL bVal);
	BOOL		GetSaveMultiPaste();

	BOOL		m_bShowPersistent{};
	void		SetShowPersistent(BOOL bVal);
	BOOL		GetShowPersistent();

	BOOL		m_bHideDittoOnPaste{};
	void		SetHideDittoOnPaste(BOOL bVal);
	BOOL		GetHideDittoOnPaste();

	void		SetShowTextForFirstTenHotKeys(BOOL bVal);
	BOOL		GetShowTextForFirstTenHotKeys();

	void		SetMainHWND(long lhWnd);
	long		GetMainHWND();

	void		SetCaptionPos(long lPos);
	long		GetCaptionPos();

	void		SetAutoHide(BOOL bAutoHide);
	BOOL		GetAutoHide();

	long		m_bDescTextSize{};
	void		SetDescTextSize(long lSize);
	long		GetDescTextSize();

	BOOL		m_bDescShowLeadingWhiteSpace{};
	void		SetDescShowLeadingWhiteSpace(BOOL bVal);
	BOOL		GetDescShowLeadingWhiteSpace();

	BOOL		m_bAllwaysShowDescription{};
	void		SetAllwaysShowDescription(long bShow);
	BOOL		GetAllwaysShowDescription();

	long		m_bDoubleClickingOnCaptionDoes{};
	void		SetDoubleClickingOnCaptionDoes(long lOption);
	long		GetDoubleClickingOnCaptionDoes();

	BOOL		m_bPrompForNewGroupName{};
	void		SetPrompForNewGroupName(BOOL bOption);
	BOOL		GetPrompForNewGroupName();

	BOOL		m_bSendPasteOnFirstTenHotKeys{};
	void		SetSendPasteOnFirstTenHotKeys(BOOL bOption);
	BOOL		GetSendPasteOnFirstTenHotKeys();


	BOOL		m_HideDittoOnHotKeyIfAlreadyShown{};
	BOOL		GetHideDittoOnHotKeyIfAlreadyShown();
	void		SetHideDittoOnHotKeyIfAlreadyShown(BOOL bVal);

	BOOL		GetFont(LOGFONT &font);
	void		SetFont(LOGFONT &font);

	BOOL		m_bDrawThumbnail{};
	void		SetDrawThumbnail(long bDraw);
	BOOL		GetDrawThumbnail();

	BOOL		m_bFastThumbnailMode{};
	void		SetFastThumbnailMode(BOOL bval);
	BOOL		GetFastThumbnailMode();

	BOOL		m_bDrawRTF{};
	void		SetDrawRTF(long bDraw);
	BOOL		GetDrawRTF();

	BOOL		m_bMultiPasteReverse{};
	void		SetMultiPasteReverse(BOOL bVal);
	BOOL		GetMultiPasteReverse();

	CString	m_csPlaySoundOnCopy{};
	void		SetPlaySoundOnCopy(CString cs);
	CString	GetPlaySoundOnCopy();

	BOOL		m_bSendPasteMessageAfterSelection{};
	void		SetSendPasteAfterSelection(BOOL bVal);
	BOOL		GetSendPasteAfterSelection();

	BOOL		m_bFindAsYouType{};
	void		SetFindAsYouType(BOOL bVal);
	BOOL		GetFindAsYouType();

	BOOL		m_bEnsureEntireWindowCanBeSeen{};
	void		SetEnsureEntireWindowCanBeSeen(BOOL bVal);
	BOOL		GetEnsureEntireWindowCanBeSeen();

	BOOL		m_bShowAllClipsInMainList{};
	void		SetShowAllClipsInMainList(BOOL bVal);
	BOOL		GetShowAllClipsInMainList();


	long		m_lMaxClipSizeInBytes{};
	long		GetMaxClipSizeInBytes();
	void		SetMaxClipSizeInBytes(long lSize);

	CString	GetLanguageFile();
	void		SetLanguageFile(CString csLanguage);

	DWORD	m_dwSaveClipDelay{};
	ULONG	GetSaveClipDelay();
	void		SetSaveClipDelay(DWORD dwDelay);

	long		m_lProcessDrawClipboardDelay{};
	long		GetProcessDrawClipboardDelay();
	void		SetProcessDrawClipboardDelay(long lDelay);

	BOOL		m_bEnableDebugLogging{};
	BOOL		GetEnableDebugLogging();
	void		SetEnableDebugLogging(BOOL bEnable);

	BOOL		m_bEnsureConnectToClipboard{};
	BOOL		GetEnsureConnectToClipboard();
	void		SetEnsureConnectToClipboard(BOOL bSet);

	BOOL		GetPromptWhenDeletingClips();
	void		SetPromptWhenDeletingClips(BOOL bSet);

	CString	GetLastImportDir();
	void		SetLastImportDir(CString csDir);

	CString	GetLastExportDir();
	void		SetLastExportDir(CString csDir);

	BOOL		GetUpdateDescWhenSavingClip();
	void		SetUpdateDescWhenSavingClip(BOOL bSet);

	BOOL		m_outputDebugStringLogging{};
	BOOL		GetEnableOutputDebugStringLogging();
	void		SetEnableOutputDebugStringLogging(BOOL bSet);


	CString  GetPath(long lPathID);

	__int64	nLastDbWriteTime{};

	long		GetDittoRestoreClipboardDelay();
	void		SetDittoRestoreClipboardDelay(long lDelay);

	void		GetCopyBufferItem(int nPos, CCopyBufferItem &Item);
	void		SetCopyBufferItem(int nPos, CCopyBufferItem &Item);

	CString  GetMultiPasteSeparator(bool bConvertToLineFeeds = true);
	void		SetMultiPasteSeparator(CString csSep);

	BOOL		GetSetCurrentDirectory();

	CString GetPasteString(CString csAppName);

	CString GetDefaultPasteString();
	void SetDefaultPasteString(CString val);

	CString GetCopyString(CString csAppName);
	CString GetDefaultCopyString();
	void SetDefaultCopyString(CString val);

	CString GetCutString(CString csAppName);
	CString GetDefaultCutString();
	void SetDefaultCutString(CString val);

	BOOL	GetEditWordWrap();
	void	SetEditWordWrap(BOOL bSet);


	bool		GetIsPortableDitto();
	bool		GetIsWindowsApp();
	bool		GetIsChocolateyApp();

	long		GetAutoMaxDelay();
	void		SetAutoMaxDelay(long lDelay);

	void SetTheme(CString csTheme);
	CString GetTheme();

	long		GetKeyStateWaitTimerCount();
	long		GetKeyStatePasteDelay();

	DWORD	GetDittoHotKey();
	
	DWORD	SendKeysDelay();
	void		SetSendKeysDelay(DWORD val);

	DWORD	RealSendKeysDelay();
	void		SetRealSendKeysDelay(DWORD val);

	DWORD	WaitForActiveWndTimeout();
	DWORD	FocusChangedDelay();
	DWORD	FocusWndTimerTimeout();

	BOOL		GetConnectedToClipboard();
	void		SetConnectedToClipboard(BOOL val);

	DWORD	GetTextOnlyRestoreDelay();
	DWORD 	GetTextOnlyPasteDelay();

	BOOL		GetSetFocusToApp(CString csAppName);

	DWORD	SelectedIndex();
	void		SetSelectedIndex(int val);

	void		SetCopyAppInclude(CString csAppName);
	CString  GetCopyAppInclude();

	void		SetCopyAppExclude(CString csAppName);
	CString  GetCopyAppExclude();

	CString  GetCopyAppSeparator();

	DWORD	GetNoFormatsRetryDelay();

	DWORD	GetMainDeletesDeleteCount();

	DWORD	GetIdleSecondsBeforeDelete();

	DWORD	GetDbTimeout();

	DWORD	GetFunnyTickCountAdjustment();

	DWORD	GetMinIdleTimeBeforeTrackFocus();

	DWORD	GetTimeBeforeExpandWindow();

	DWORD	GetUseGuiThreadInfoForFocus();

	void		SetSearchDescription(BOOL val);
	BOOL		GetSearchDescription();

	void		SetSearchFullText(BOOL val);
	BOOL		GetSearchFullText();

	void		SetSearchQuickPaste(BOOL val);
	BOOL		GetSearchQuickPaste();

	void		SetSimpleTextSearch(BOOL val);
	BOOL		GetSimpleTextSearch();

	void		SetMoveClipsOnGlobal10(BOOL val);
	BOOL		GetMoveClipsOnGlobal10();

	void		SetShowScrollBar(BOOL val);
	BOOL		GetShowScrollBar();
	BOOL		m_showScrollBar{};

	void		SetUseModernScrollBar(BOOL val);
	BOOL		GetUseModernScrollBar();
	BOOL		m_useModernScrollBar{ TRUE };

	void		SetPasteAsAdmin(BOOL val);
	BOOL		GetPasteAsAdmin();

	void		SetRememberDescPos(BOOL val);
	BOOL		GetRememberDescPos();

	void		SetSizeDescWindowToContent(BOOL val);
	BOOL		GetSizeDescWindowToContent();

	void		SetScaleImagesToDescWindow(BOOL val);
	BOOL		GetScaleImagesToDescWindow();

	void		SetDescWndPoint(CPoint point);
	void		GetDescWndPoint(CPoint &point);

	void		SetDescWndSize(CSize size);
	void		GetDescWndSize(CSize &size);

	void		SetShowInTaskBar(BOOL val);
	BOOL		GetShowInTaskBar();

	void		SetHideTaskbarIconOnClose(BOOL val);
	BOOL		GetHideTaskbarIconOnClose();

	void		SetDiffApp(CString val);
	CString	GetDiffApp();

	void		SetQRCodeBorderPixels(int val);
	int	GetQRCodeBorderPixels();

	BOOL GetRegExTextSearch();
	void SetRegExTextSearch(BOOL val);




	int ReadRandomFileInterval();
	int ReadRandomFileIdleMin();

	BOOL GetShowGroupsInMainList();
	void SetShowGroupsInMainList(BOOL val);

	void SetGroupDoubleClickTimeMS(int val);
	int GetGroupDoubleClickTimeMS();

	void SetSaveToGroupTimeoutMS(int val);
	int GetSaveToGroupTimeoutMS();

	void SetCopyReasonTimeoutMS(int val);
	int GetCopyReasonTimeoutMS();

	void SetWindowsResumeDelayReOpenDbMS(int val);
	int GetWindowsResumeDelayReOpenDbMS();

	BOOL GetShowMsgWndOnCopyToGroup();
	void SetShowMsgWndOnCopyToGroup(BOOL val);

	int GetActionShortCutA(DWORD action, int pos);
	void SetActionShortCutA(int action, DWORD shortcut, int pos);

	int GetActionShortCutB(DWORD action, int pos);
	void SetActionShortCutB(int action, DWORD shortcut, int pos);

	BOOL	m_bShowAlwaysOnTopWarning{ TRUE };
	BOOL GetShowAlwaysOnTopWarning();
	void SetShowAlwaysOnTopWarning(BOOL show);
	
	BOOL GetUseIPFromAccept();
	void SetUseIPFromAccept(BOOL useAccept);

	int GetDragId();
	void SetDragId(int id);

	BOOL GetShowIfClipWasPasted();
	void SetShowIfClipWasPasted(BOOL val);

	int GetLastGroupToggle();
	void SetLastGroupToggle(int val);

	BOOL GetMouseClickHidesDescription();
	void SetMouseClickHidesDescription(int val);

	BOOL GetWrapDescriptionText();
	void SetWrapDescriptionText(int val);

	BOOL GetUseUISelectedGroupForLastTenCopies();
	void SetUseUISelectedGroupForLastTenCopies(int val);


	BOOL GetAdjustClipsForCRC();
	void SetAdjustClipsForCRC(int val);


	int GetBalloonTimeout();
	void SetBalloonTimeout(int val);


	int GetMaxFileContentsSize();
	void SetMaxFileContentsSize(int val);

	int GetErrorMsgPopupTimeout();
	void SetErrorMsgPopupTimeout(int val);

	CRegExFilterHelper m_regexHelper{};
	void		SetRegexFilter(CString val, int pos);
	CString	GetRegexFilter(int pos);

	void SetRegexFilterByProcessName(CString val, int pos);
	CString GetRegexFilterByProcessName(int pos);

	BOOL GetOpenToGroupByActiveExe();
	void SetOpenToGroupByActiveExe(int val);

	BOOL GetShowStartupMessage();
	void SetShowStartupMessage(int val);

	long m_tooltipTimeout{};
	long GetToolTipTimeout();
	void SetToolTipTimeout(int long);

	CString GetPastSearchXml();
	void SetPastSearchXml(CString val);


	BOOL m_cleanRTFBeforeDrawing{ TRUE };
	BOOL GetCleanRTFBeforeDrawing();
	void SetCleanRTFBeforeDrawing(BOOL val);

	BOOL GetDisableExpireClipsConfig();
	void SetDisableExpireClipsConfig(BOOL val);

	BOOL GetRevertToTopLevelGroup();
	void SetRevertToTopLevelGroup(BOOL val);

	BOOL GetUpdateClipOrderOnCtrlC();
	void SetUpdateClipOrderOnCtrlC(BOOL val);

	int GetMaxToolTipLines();
	void SetMaxToolTipLines(int val);

	int GetMaxToolTipCharacters();
	void SetMaxToolTipCharacters(int val);

	int m_doubleKeyStrokeTimeout{ 350 };
	int GetDoubleKeyStrokeTimeout();
	void SetDoubleKeyStrokeTimeout(int val);

	int m_firstTenHotKeysStart{ 1 };
	int GetFirstTenHotKeysStart();
	void SetFirstTenHotKeysStart(int val);

	int m_firstTenHotKeysFontSize{ 5 };
	int GetFirstTenHotKeysFontSize();
	void SetFirstTenHotKeysFontSize(int val);

	BOOL GetAddCFHDROP_OnDrag();
	void SetAddCFHDROP_OnDrag(BOOL val);

	int GetCopyAndSveDelay();
	void SetCopyAndSveDelay(int val);

	int GetEditorDefaultFontSize();
	void SetEditorDefaultFontSize(int val);

	BOOL m_moveSelectionOnOpenHotkey{ TRUE };
	BOOL GetMoveSelectionOnOpenHotkey();
	void SetMoveSelectionOnOpenHotkey(BOOL val);

	BOOL m_allowBackToBackDuplicates{};
	BOOL GetAllowBackToBackDuplicates();
	void SetAllowBackToBackDuplicates(BOOL val);

	BOOL m_maintainSearchView{};
	BOOL GetMaintainSearchView();
	void SetMaintainSearchView(BOOL val);


	CString m_tempDragFileName{};
	CTime m_tempDragFileNameSetTime{};
	CString GetTempDragFileName();
	void SetTempDragFileName(CString val);

	BOOL m_refreshViewAfterPasting{ TRUE };
	BOOL GetRefreshViewAfterPasting();
	void SetRefreshViewAfterPasting(BOOL val);

	CString GetSlugifySeparator();
	void SetSlugifySeparator(CString val);

	BOOL m_supportAllTypes{};
	BOOL GetSupportAllTypes();
	void SetSupportAllTypes(BOOL val);

	CString GetIgnoreAnnoyingCFDIB(BOOL useCache = FALSE);
	CString m_ignoreAnnoyingCFDIB{};
	void SetIgnoreAnnoyingCFDIB(CString val);
	std::set<CString> GetIgnoreAnnoyingCFDIBSet(BOOL useCache = FALSE);
	// The options that decide how a copied clip is saved, for injection into CClip
	DittoCore::ClipSaveSettings GetClipSaveSettings();

	BOOL GetRegexCaseInsensitive();
	void SetRegexCaseInsensitive(BOOL val);

	BOOL		m_bDrawCopiedColorCode{};
	void		SetDrawCopiedColorCode(long bDraw);
	BOOL		GetDrawCopiedColorCode();


	BOOL m_centerWindowBelowCursorOrCaret{};
	void SetCenterWindowBelowCursorOrCaret(BOOL center);
	BOOL GetCenterWindowBelowCursorOrCaret();

	BOOL SetTextEditorPath(CString path);
	CString GetTextEditorPath();

	BOOL SetImageEditorPath(CString path);
	CString GetImageEditorPath();

	BOOL SetRTFEditorPath(CString path);
	CString GetRTFEditorPath();



	void SetPreferUtf8ForCompare(BOOL val);
	BOOL GetPreferUtf8ForCompare();

	int	m_clipEditSaveDelayAfterLoadSeconds{ 3 };
	void SetClipEditSaveDelayAfterLoadSeconds(int val);
	BOOL GetClipEditSaveDelayAfterLoadSeconds();

	int	m_clipEditSaveDelayAfterSaveSeconds{ 3 };
	void SetClipEditSaveDelayAfterSaveSeconds(int val);
	BOOL GetClipEditSaveDelayAfterSaveSeconds();

	BOOL m_bDoNotHideOnDeactivate{};
	void SetDoNotHideOnDeactivate(BOOL val);
	BOOL GetDoNotHideOnDeactivate();

	BOOL SetEditWndSize(CSize size);
	void GetEditWndSize(CSize& size);

	BOOL SetEditWndPoint(CPoint point);
	void GetEditWndPoint(CPoint& point);

	BOOL m_enforceClipboardIgnoreFormats{ TRUE };
	void SetEnforceClipboardIgnoreFormats(BOOL val);
	BOOL GetEnforceClipboardIgnoreFormats();

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
	void ApplyPathRule(const PathRule& rule, CString& csDir);

	/**
	 * @brief LoadSettings' step: finds the ini file (Windows Store, Chocolatey, portable or app data) and sets m_csIniFileName, m_bFromIni and the app kind flags.
	 * @param exeDir The exe's folder, with a trailing backslash.
	 */
	void LocateIniFile(const CString& exeDir);

	/** @brief LocateIniFile's step for a plain install: the ini file next to the exe (portable) or in app data. */
	void LocatePortableOrAppDataIniFile();

	/** @brief Where the settings are stored: the registry, or the ini file once LoadSettings found one. */
	std::unique_ptr<DittoCore::ISettingsStore> m_store{};
};
