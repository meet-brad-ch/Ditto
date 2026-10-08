#pragma once

#include "QListCtrl.h"
#include "SearchEditBox.h"
#include "WndEx.h"
#include "GroupStatic.h"
#include "GroupTree.h"
#include "AlphaBlend.h"
#include "Sqlite\CppSQLite3.h"
#include <vector>
#include <list>
#include <map>
#include <afxmt.h>
#include "ClipFormatQListCtrl.h"
#include "QPasteWndThread.h"
#include "editwithbutton.h"
#include "GdipButton.h"
#include "SpecialPasteOptions.h"
#include "ClipIds.h"
#include "SymbolEdit.h"
#include "Popup.h"
#include "ModernScrollBar.h"
#include "ActionEnums.h"
#include <array>
#include <span>

class CMainFrame;

class CMainTable
{
public:
	CMainTable() :
		m_lID(-1),
		m_bDontAutoDelete(false),
		m_bIsGroup(false),
		m_bHasShortCut(false),
		m_bHasParent(false),
		m_dateCopied(0),
		m_datePasted(0)
	{
	}

	~CMainTable()
	{
	}

	long m_lID;
	CString m_Desc;
	bool m_bDontAutoDelete;
	bool m_bIsGroup;
	bool m_bHasShortCut;
	bool m_bHasParent;
	CString m_QuickPaste;
	double m_clipOrder{};
	double m_clipGroupOrder{};
	double m_stickyClipOrder{};
	double m_stickyClipGroupOrder{};
	__int64 m_dateCopied;
	__int64 m_datePasted;

	static bool SortDesc(const CMainTable& d1, const CMainTable& d2)
	{
		double d1StickyOrder = d1.m_stickyClipOrder;
		double d2StickyOrder = d2.m_stickyClipOrder;

		if (d1StickyOrder != d2StickyOrder)
			return d1StickyOrder > d2StickyOrder;

		if (d1.m_bIsGroup != d2.m_bIsGroup)
			return d1.m_bIsGroup < d2.m_bIsGroup;

		return d1.m_clipOrder > d2.m_clipOrder;
	}

	static bool GroupSortDesc(const CMainTable& d1, const CMainTable& d2)
	{
		double d1StickyOrder = d1.m_stickyClipGroupOrder;

		double d2StickyOrder = d2.m_stickyClipGroupOrder;

		if (d1StickyOrder != d2StickyOrder)
			return d1StickyOrder > d2StickyOrder;

		if (d1.m_bIsGroup != d2.m_bIsGroup)
			return d1.m_bIsGroup < d2.m_bIsGroup;

		return d1.m_clipGroupOrder > d2.m_clipGroupOrder;
	}
};


typedef std::map<int, CMainTable> MainTypeMap;
typedef std::map<int, CClipFormatQListCtrl> CF_DibTypeMap;
typedef std::map<int, char> CF_NoDibTypeMap;


/////////////////////////////////////////////////////////////////////////////
// CQPasteWnd window

class CGetSetOptions;
class CAppServices;

class CQPasteWnd : public CWndEx
{
	// Construction
public:
	CQPasteWnd();

private:
	/** @brief The application settings (theApp's services; this window is created by the framework).
	@return the settings. */
	CGetSetOptions& Settings() const;
	/** @brief The application services (theApp's; this window is created by the framework). Static, so that
	the static helpers of this class reach them too.
	@return the services. */
	static CAppServices& Services();

	// Attributes
public:
	// Operations
public:
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQPasteWnd)
public:
	virtual BOOL Create(CRect rect, CWnd* pParentWnd);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	bool CheckActions(MSG* pMsg);

	//}}AFX_VIRTUAL

	// Implementation
public:
	bool Add(const CString& csHeader, const CString& csText, int nID);
	virtual ~CQPasteWnd();

	void UpdateFont();

	//protected:
	CQListCtrl m_lstHeader;

	CAlphaBlend m_Alpha;
	//CEditWithButton m_search;
	CSymbolEdit m_search;
	CFont m_SearchFont;
	bool m_bHideWnd;
	CString m_strSQLSearch;
	CString m_strSearch;
	CGroupStatic m_stGroup;
	CFont m_groupFont;
	CString m_Title;
	CGroupTree m_GroupTree;
	CGdipButton m_ShowGroupsFolderBottom;
	CGdipButton m_BackButton;
	CGroupStatic m_alwaysOnToWarningStatic;
	CGdipButton m_systemMenu;
	CGroupStatic m_noSearchResultsStatic;

	long m_lRecordCount{};
	bool m_bStopQuery{};
	bool m_bHandleSearchTextChange;
	bool m_bModifersMoveActive;

	CQPasteWndThread m_thread;
	CQPasteWndThread m_extraDataThread;
	std::vector<CMainTable> m_listItems;

	std::list<CPoint> m_loadItems;
	std::list<CClipFormatQListCtrl> m_ExtraDataLoadItems;
	CF_DibTypeMap m_cf_dibCache;
	CF_NoDibTypeMap m_cf_NO_dibCache;
	CF_DibTypeMap m_cf_rtfCache;
	CF_NoDibTypeMap m_cf_NO_rtfCache;
	CCriticalSection m_CritSection;
	CAccels m_actions;
	CAccels m_toolTipActions;
	CAccels m_modifierKeyActions;
	bool m_showScrollBars;
	CModernScrollBar m_modernScrollBar;     // Vertical scrollbar
	CModernScrollBar m_modernScrollBarHorz; // Horizontal scrollbar
	int m_leftSelectedCompareId;
	INT64 m_extraDataCounter;
	CPopup m_popupMsg;
	bool m_noSearchResults;
	bool m_bShowStarredClips;
	CAccel m_timerAction;
	__int64 m_lastDbWrite;
	bool m_pendingRefresh;
	ULONGLONG m_lastNonActiveMouseMove{};

	void RefreshNc();
	void UpdateStatus(bool bRepaintImmediately = false); // regenerates the status (caption) text
	BOOL FillList(CString csSQLSearch = "");
	BOOL HideQPasteWindow(bool releaseFocus, BOOL clearSearchData = -1);
	BOOL ShowQPasteWindow(BOOL bFillList = TRUE);
	void MoveControls();

	void DeleteSelectedRows();

	BOOL OpenID(int id, CSpecialPasteOptions pasteOptions);
	BOOL OpenSelection(CSpecialPasteOptions pasteOptions);
	BOOL OpenIndex(int item, bool plainTextOnly = false);
	BOOL NewGroup(bool bGroupSelection = true, int parentId = -1);

	CString LoadDescription(int nItem);
	bool SaveDescription(int nItem, CString text);

	//Menu Items
	void SetLinesPerRow(int lines, bool force, bool resetListCount);
	void SetTransparency(int percent);
	void OnUpdateLinesPerRow(CCmdUI* pCmdUI, int nValue);
	void OnUpdateTransparency(CCmdUI* pCmdUI, int nValue);
	void AddShowStarredClipsMenuItem(CMenu* pMenu);
	void SetMenuChecks(CMenu* pMenu);


	bool InsertNextNRecords(int nEnd);

	CString GetDisplayText(int lDontAutoDelete, int lShortCut, bool bIsGroup, int lParentID, CString csText);

	static void FillMainTable(CMainTable& table, CppSQLite3Query& q);
	void RunThread();
	void MoveSelection(bool down, bool requireModifersActive);
	void OnKeyStateUp();
	void SetKeyModiferState(bool bActive);
	void SaveWindowSize();
	void SelectFocusID();
	void SetSearchImages();
	/**
	 * @brief Clears a clip's sticky setting in the shown list and saves it.
	 * @param id The clip.
	 * @param sort Set to true when the list item changed.
	 * @return False when the clip could not be saved (the error is shown).
	 */
	bool RemoveStickyInternal(int id, bool& sort);

	DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	void OnDragLeave();
	COleDropTarget* m_pDropTarget{};

	bool DoAction(CAccel a);
	bool DoAction(DWORD cmd);
	bool DoActionShowDescription();
	bool DoActionNextDescription();
	bool DoActionPrevDescription();
	bool DoActionShowMenu();
	bool DoActionShowSystemMenu();
	bool DoActionNewGroup();
	bool DoActionNewGroupSelection();
	bool DoActionToggleFileLogging();
	bool DoActionToggleOutputDebugString();
	bool DoActionCloseWindow();
	bool DoActionForceCloseWindow();
	bool DoActionNextTabControl();
	bool DoActionPrevTabControl();
	bool DoActionShowGroups();
	bool DoActionNewClip();
	bool DoActionEditClip();
	bool DoActionMoveSelectionDown();
	bool DoActionToggleDescriptionWordWrap();
	bool DoActionApplyLastSearch();
	bool DoActionToggleSearchMethod();
	bool DoActionMoveSelectionUp();
	bool DoModifierActiveActionSelectionUp();
	bool DoModifierActiveActionSelectionDown();
	bool DoModifierActiveActionMoveFirst();
	bool DoModifierActiveActionMoveLast();
	bool DoActionCancelFilter();
	bool DoActionHomeList();
	bool DoActionBackGroup();
	bool DoActionToggleShowPersistant();
	bool DoActionDeleteSelected();
	bool DoActionPasteSelected();
	bool DoActionClipProperties();
	bool DoActionPasteSelectedPlainText();
	bool DoActionMoveClipToGroup();
	bool DoActionElevatePrivleges();
	bool DoShowInTaskBar();
	bool DoClipCompare();
	bool DoSelectLeftSideCompare();
	bool DoSelectRightSideAndDoCompare();
	bool DoExportToQRCode();
	// Shows the text as a QR code in a viewer window; reports text too long for a QR code
	bool ShowQRCode(const CString& clipText, const CString& description);
	bool DoExportToTextFile();
	bool DoActionGenerateGuid();
	bool DoPasteAsImage();
	bool DoExportToBitMapFile();
	bool DoSaveCurrentClipboard();
	bool DoMoveClipDown();
	bool DoMoveClipUp();
	bool DoMoveClipTOP();
	bool DoMoveClipLast();
	bool DoFilterOnSelectedClip();
	bool DoPasteUpperCase();
	bool DoPasteCamelCase();
	bool DoPasteImagesVert();
	bool DoPasteAsciiOnly();
	bool DoPasteImagesHorz();
	bool DoPasteLowerCase();
	bool DoPasteCapitalize();
	bool DoPasteSentenceCase();
	bool DoInvertCase();
	bool DoPasteRemoveLineFeeds();
	bool DoPastePlusAddLineFeed();
	bool DoPasteAddTwoLineFeeds();
	bool DoPasteTypoglycemia();
	bool DoPasteAddCurrentTime();
	bool OnShowFirstTenText();
	bool OnShowClipWasPasted();
	bool OnToggleLastGroupToggle();
	bool OnMakeTopSticky(bool forceSort);
	bool OnMakeLastSticky();
	bool OnRemoveStickySetting();
	bool DoActionReplaceTopStickyClip();
	bool DoActionSaveCF_HDROP_FileData();
	bool DoActionToggleClipboardConnection();
	bool DoActionPasteDontMoveClip();
	bool DoSetDragFileName();
	bool DoActionPasteTrimWhiteSpace();
	bool DoActionPastePosixifyPaths();
	bool DoActionToggleTransparency();
	bool DoActionIncreaseTransparency();
	bool DoActionDecreaseTransparency();

	// Refresh scrollbar colors from current theme
	void RefreshScrollBarColors();
	// Refresh all theme colors (caption, scrollbars, etc.)
	void RefreshThemeColors();
	bool DoActionSlugify();
	bool DoCopySelection();
	bool DoRefreshList();
	bool DoDeleteAllNonUsedClips();

	bool OnNewClip();
	bool OnImportClip();
	bool OnDeleteClipData();
	bool OnGlobalHotkyes();

	void UpdateMenuShortCut(CCmdUI* pCmdUI, DWORD action);

	bool ShowProperties(int id, int row);
	bool DeleteClips(CClipIDs& IDs, ARRAY& Indexs);
	void RemoveFromImageRtfCache(int row, int id = -1);
	bool SyncClipDataToArrayData(CClip& clip);
	bool SelectIds(ARRAY& ids);

	void LoadShortcuts();

	void ShowRightClickMenu();

	void SetCurrentTransparency();

	// Generated message map functions
protected:
	//{{AFX_MSG(CQPasteWnd)
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pOldWnd);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnMenuLinesperrow1();
	afx_msg void OnMenuLinesperrow2();
	afx_msg void OnMenuLinesperrow3();
	afx_msg void OnMenuLinesperrow4();
	afx_msg void OnMenuLinesperrow5();
	afx_msg void OnMenuTransparency10();
	afx_msg void OnMenuTransparency15();
	afx_msg void OnMenuTransparency20();
	afx_msg void OnMenuTransparency25();
	afx_msg void OnMenuTransparency30();
	afx_msg void OnMenuTransparency40();
	afx_msg void OnMenuTransparency5();
	afx_msg void OnMenuTransparencyNone();
	afx_msg void OnMenuDelete();
	afx_msg void OnMenuPositioningAtcaret();
	afx_msg void OnMenuPositioningAtcursor();
	afx_msg void OnMenuPositioningAtpreviousposition();
	afx_msg void OnMenuOptions();
	afx_msg LRESULT OnCancelFilter(WPARAM wParam, LPARAM lParam);
	afx_msg void OnMenuExitprogram();
	afx_msg void OnMenuToggleConnectCV();
	afx_msg void OnMenuProperties();
	afx_msg void OnClose();
	afx_msg void OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void GetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnFindItem(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMenuFirsttenhotkeysUsectrlnum();
	afx_msg void OnMenuFirsttenhotkeysShowhotkeytext();
	afx_msg void OnMenuQuickoptionsAllwaysshowdescription();
	afx_msg void OnMenuQuickoptionsDoubleclickingoncaptionTogglesalwaysontop();
	afx_msg void OnMenuQuickoptionsDoubleclickingoncaptionRollupwindow();
	afx_msg void OnMenuQuickoptionsDoubleclickingoncaptionTogglesshowdescription();
	afx_msg void OnMenuQuickoptionsPromptfornewgroupnames();
	afx_msg void OnShowGroupsBottom();
	afx_msg void OnShowGroupsTop();
	afx_msg void OnMenuViewgroups();
	afx_msg void OnMenuQuickpropertiesSettoneverautodelete();
	afx_msg void OnMenuQuickpropertiesAutodelete();
	afx_msg void OnMenuQuickpropertiesRemovehotkey();
	afx_msg void OnMenuGroupsMovetogroup();
	afx_msg void OnMenuPasteplaintextonly();
	afx_msg void OnMenuQuickoptionsFont();
	afx_msg void OnMenuQuickoptionsShowthumbnails();
	afx_msg void OnMenuQuickoptionsDrawrtftext();
	afx_msg void OnMenuQuickoptionsPasteclipafterselection();
	afx_msg void OnSearchEditChange();
	afx_msg void OnMenuQuickoptionsFindasyoutype();
	afx_msg void OnMenuQuickoptionsEnsureentirewindowisvisible();
	afx_msg void OnMenuQuickoptionsShowclipsthatareingroupsinmainlist();
	afx_msg void OnMenuPastehtmlasplaintext();
	afx_msg void OnPromptToDeleteClip();
	afx_msg void OnUpdateMenuNewgroup(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuNewgroupselection(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuAllwaysontop(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuViewfulldescription(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuViewgroups(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuPasteplaintextonly(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuDelete(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuProperties(CCmdUI* pCmdUI);
	afx_msg void OnDestroy();
	afx_msg LRESULT OnSearchEnterKeyPressed(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSearch(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDelete(WPARAM wParam, LPARAM lParam);
	afx_msg void OnGetToolTipText(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnListSelect_DB_ID(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnListMoveSelectionToGroup(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRefreshView(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnReloadClipInUI(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGroupTreeMessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFillRestOfList(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRefeshRow(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetListCount(WPARAM wParam, LPARAM lParam);
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg void OnNcLButtonDblClk(UINT nHitTest, CPoint point);
	afx_msg void OnViewcaptionbaronRight();
	afx_msg void OnViewcaptionbaronBottom();
	afx_msg void OnViewcaptionbaronLeft();
	afx_msg void OnViewcaptionbaronTop();
	afx_msg void OnMenuAutohide();
	afx_msg void OnMenuViewfulldescription();
	afx_msg void OnMenuAllwaysontop();
	afx_msg void OnMenuNewGroup();
	afx_msg void OnMenuNewGroupSelection();
	afx_msg void OnBackButton();
	afx_msg void OnSystemButton();
	afx_msg LRESULT OnUpDown(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnItemDeleted(WPARAM wParam, LPARAM lParam);
	LRESULT OnToolTipWndInactive(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnMenuExport();
	afx_msg void OnMenuImport();
	afx_msg void OnQuickpropertiesRemovequickpaste();
	afx_msg void OnMenuEdititem();
	afx_msg void OnMenuNewclip();
	afx_msg void OnUpdateMenuEdititem(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuNewclip(CCmdUI* pCmdUI);
	afx_msg void OnAddinSelect(UINT id);
	afx_msg LRESULT OnSelectAll(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnShowHideScrollBar(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateScrollBar(WPARAM wParam, LPARAM lParam);
	afx_msg void OnMenuSearchDescription();
	afx_msg void OnMenuSearchFullText();
	afx_msg void OnMenuSearchQuickPaste();
	afx_msg void OnMenuShowStarredClips();
	afx_msg void OnMenuSimpleTextSearch();
	afx_msg LRESULT OnPostOptions(WPARAM wParam, LPARAM lParam);
	afx_msg void OnMakeTopStickyClip();
	afx_msg void OnMakeLastStickyClip();
	afx_msg void OnRemoveSticky();
	afx_msg void OnElevateAppToPasteIntoElevatedApp();

public:
	afx_msg void OnQuickoptionsShowintaskbar();
	afx_msg void OnMenuViewasqrcode();
	afx_msg void OnExportExporttotextfile();
	afx_msg void OnCompareCompare();
	afx_msg void OnCompareSelectleftcompare();
	afx_msg void OnCompareCompareagainst();
	afx_msg void OnUpdateCompareCompare(CCmdUI* pCmdUI);
	afx_msg LRESULT OnShowProperties(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNewGroup(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDeleteId(WPARAM wParam, LPARAM lParam);
	afx_msg void OnMenuRegularexpressionsearch();
	afx_msg void OnImportExportclipBitmap();
	afx_msg void OnUpdateImportExportclipBitmap(CCmdUI* pCmdUI);
	afx_msg void OnMenuWildcardsearch();
	afx_msg void OnMenuSavecurrentclipboard();
	afx_msg void OnUpdateMenuSavecurrentclipboard(CCmdUI* pCmdUI);
	afx_msg void OnCliporderMoveup();
	afx_msg void OnUpdateCliporderMoveup(CCmdUI* pCmdUI);
	afx_msg void OnCliporderMovedown();
	afx_msg void OnUpdateCliporderMovedown(CCmdUI* pCmdUI);
	afx_msg void OnCliporderMovetotop();
	afx_msg void OnUpdateCliporderMovetotop(CCmdUI* pCmdUI);
	afx_msg void OnMenuFilteron();
	afx_msg void OnUpdateMenuFilteron(CCmdUI* pCmdUI);
	afx_msg void OnMenuGoToEntry();
	afx_msg void OnUpdateMenuGoToEntry(CCmdUI* pCmdUI);
	afx_msg void OnAlwaysOnTopClicked();
	//afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnSpecialpasteUppercase();
	afx_msg void OnUpdateSpecialpasteUppercase(CCmdUI* pCmdUI);
	afx_msg void OnSpecialpasteLowercase();
	afx_msg void OnUpdateSpecialpasteLowercase(CCmdUI* pCmdUI);
	afx_msg void OnSpecialpasteCapitalize();
	afx_msg void OnUpdateSpecialpasteCapitalize(CCmdUI* pCmdUI);
	afx_msg void OnSpecialpasteSentence();
	afx_msg void OnUpdateSpecialpasteSentence(CCmdUI* pCmdUI);
	afx_msg void OnSpecialpasteRemovelinefeeds();
	afx_msg void OnUpdateSpecialpasteRemovelinefeeds(CCmdUI* pCmdUI);
	afx_msg void OnSpecialpastePaste();
	afx_msg void OnUpdateSpecialpastePaste(CCmdUI* pCmdUI);
	afx_msg void OnSpecialpastePaste32919();
	afx_msg void OnUpdateSpecialpastePaste32919(CCmdUI* pCmdUI);
	afx_msg void OnSpecialpasteTypoglycemia();
	afx_msg void OnUpdateSpecialpasteTypoglycemia(CCmdUI* pCmdUI);
	afx_msg void OnNMClickList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMDblclkList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMRClickList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMRDblclkList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnQuickoptionsShowtextforfirsttencopyhotkeys();
	afx_msg void OnUpdateQuickoptionsShowtextforfirsttencopyhotkeys(CCmdUI* pCmdUI);
	afx_msg void OnQuickoptionsShowindicatoracliphasbeenpasted();
	afx_msg void OnUpdateQuickoptionsShowindicatoracliphasbeenpasted(CCmdUI* pCmdUI);
	afx_msg void OnGroupsTogglelastgroup();
	afx_msg void OnUpdateGroupsTogglelastgroup(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStickyclipsMaketopstickyclip(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStickyclipsMakelaststickyclip(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStickyclipsRemovestickysetting(CCmdUI* pCmdUI);
	afx_msg void OnSpecialpastePaste32927();
	afx_msg void OnUpdateSpecialpastePaste32927(CCmdUI* pCmdUI);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMenuGlobalhotkeys32933();
	afx_msg void OnMenuDeleteclipdata32934();
	afx_msg void OnMenuImportclip32935();
	afx_msg void OnMenuNewclip32937();
	afx_msg void OnUpdateMenuImportclip32935(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuNewclip32937(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuGlobalhotkeys32933(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMenuDeleteclipdata32934(CCmdUI* pCmdUI);
	afx_msg LRESULT OnSearchFocused(WPARAM wParam, LPARAM lParam);
	afx_msg void OnCliporderReplacetopstickyclip();
	afx_msg void OnUpdateCliporderReplacetopstickyclip(CCmdUI* pCmdUI);
	afx_msg void OnImportImportcopiedfile();
	afx_msg void OnUpdateImportImportcopiedfile(CCmdUI* pCmdUI);
	afx_msg void OnUpdate32775(CCmdUI* pCmdUI);
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
	afx_msg void OnCliporderMovetolast();
	afx_msg void OnUpdateCliporderMovetolast(CCmdUI* pCmdUI);
	afx_msg LRESULT OnCopyClip(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSpecialpastePasteDontUpdateOrder();
	afx_msg void OnUpdateOnSpecialPasteDontUpdateOrder(CCmdUI* pCmdUI);

	afx_msg void OnSpecialpasteTrim();
	afx_msg void OnSpecialpastePosixifyPaths();
	afx_msg void OnUpdateSpecialpasteTrim(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSpecialPosixifyPaths(CCmdUI* pCmdUI);
	afx_msg void OnTransparencyIncrease();
	afx_msg void OnUpdateTransparencyIncrease(CCmdUI* pCmdUI);
	afx_msg void OnTransparencyDecrease();
	afx_msg void OnUpdateTransparencyDecrease(CCmdUI* pCmdUI);
	afx_msg void OnTransparencyToggle();
	afx_msg void OnUpdateTransparencyToggle(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTransparencyNone(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTransparency5(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTransparency10(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTransparency15(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTransparency20(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTransparency25(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTransparency30(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTransparency35(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTransparency40(CCmdUI* pCmdUI);
	afx_msg void OnTransparency35();
	afx_msg void OnSpecialpasteSlugify();
	afx_msg void OnUpdateSpecialpasteSlugify(CCmdUI* pCmdUI);
	afx_msg void OnSpecialpasteTogglecase();
	afx_msg void OnUpdateSpecialpasteTogglecase(CCmdUI* pCmdUI);
	afx_msg void OnFirstShowstartupmessage();
	afx_msg void OnFirstBackupDb();
	afx_msg void OnFirstRestoreDb();
	afx_msg void OnMenuDeleteallnonusedclips();
	afx_msg void OnUpdateMenuDeleteallnonusedclips(CCmdUI* pCmdUI);
	afx_msg void OnImportSetdragfilename();
	afx_msg void OnUpdateImportSetdragfilename(CCmdUI* pCmdUI);
	afx_msg void OnSpecialpasteCamelcase();
	afx_msg void OnUpdateSpecialpasteCamelcase(CCmdUI* pCmdUI);
	afx_msg void OnSpecialpasteMultipleImagesHorz();
	afx_msg void OnUpdateSpecialpasteMultipleImagesHorz(CCmdUI* pCmdUI);
	afx_msg void OnSpecialpasteMultipleImagesVert();
	afx_msg void OnUpdateSpecialpasteMultipleImagesVert(CCmdUI* pCmdUI);
	afx_msg void OnSpecialpasteAsciitextonly();
	afx_msg void OnUpdateSpecialpasteAsciitextonly(CCmdUI* pCmdUI);
	afx_msg void OnSpecialpastePastenewguid();
	afx_msg void OnUpdateSpecialpastePastenewguid(CCmdUI* pCmdUI);
	afx_msg void OnSpecialpastePasteAsImage();
	afx_msg void OnUpdateSpecialpastePasteAsImage(CCmdUI* pCmdUI);

private:
	/** @brief The window title (and the prefix of the "always on top" title). */
	static constexpr const TCHAR* s_qpasteTitle{ _T("Ditto") };

	/** @brief The control ids of the window's child controls (Create and the message map). */
	enum : UINT
	{
		/** @brief The clip list. */
		IdListHeader = 0x201,
		/** @brief The search box. */
		IdEditSearch = 0x202,
		/** @brief The group name text. */
		IdGroupText = 0x204,
		/** @brief The "show groups" button at the bottom. */
		IdShowGroupsBottom = 0x205,
		/** @brief The "show groups" button at the top. */
		IdShowGroupsTop = 0x206,
		/** @brief The back (leave group) button. */
		IdBackButton = 0x207,
		/** @brief The "always on top" warning text. */
		IdOnTopWarning = 0x209,
		/** @brief The system menu button. */
		IdSystemButton = 0x210,
		/** @brief The "no search results" text. */
		IdNoSearchResults = 0x211,
	};

	/** @brief The timer ids of the window (SetTimer / OnTimer); CWndEx uses 5 and 6 (OnTimer passes every id on to it). */
	enum : UINT
	{
		/** @brief Fills the list cache. */
		TimerFillCache = 1,
		/** @brief Runs the search after typing paused. */
		TimerDoSearch = 2,
		/** @brief Pastes after the modifier keys were released. */
		TimerPasteFromModifier = 3,
		/** @brief Hides the error message. */
		TimerErrorMsg = 4,
		/** @brief Hides the window while a clip is dragged out of it. */
		TimerDragHideWindow = 8,
		/** @brief Ends the wait for the second key stroke of an action. */
		TimerDoAction = 7,
	};
	// OnTimer passes every timer on to CWndEx: an id shared with CWndEx would run both handlers
	// (UINT{} conversions: comparing two enumeration types is deprecated)
	static_assert(UINT{ TimerFillCache } != UINT{ TimerAutoMax } && UINT{ TimerFillCache } != UINT{ TimerButtonUp }, "CWndEx uses this timer id");
	static_assert(UINT{ TimerDoSearch } != UINT{ TimerAutoMax } && UINT{ TimerDoSearch } != UINT{ TimerButtonUp }, "CWndEx uses this timer id");
	static_assert(UINT{ TimerPasteFromModifier } != UINT{ TimerAutoMax } && UINT{ TimerPasteFromModifier } != UINT{ TimerButtonUp }, "CWndEx uses this timer id");
	static_assert(UINT{ TimerErrorMsg } != UINT{ TimerAutoMax } && UINT{ TimerErrorMsg } != UINT{ TimerButtonUp }, "CWndEx uses this timer id");
	static_assert(UINT{ TimerDragHideWindow } != UINT{ TimerAutoMax } && UINT{ TimerDragHideWindow } != UINT{ TimerButtonUp }, "CWndEx uses this timer id");
	static_assert(UINT{ TimerDoAction } != UINT{ TimerAutoMax } && UINT{ TimerDoAction } != UINT{ TimerButtonUp }, "CWndEx uses this timer id");

	// OnGetToolTipText's clip text: the clip's lines, each ended with "\r\n", up to the max tool tip lines
	CString ToolTipClipLines(const CString& clipText) const;

	/** @brief One action that DoAction runs through a member function without arguments. */
	struct ActionHandler
	{
		/** @brief The action this entry handles. */
		ActionEnums::ActionEnumValues action{};
		/** @brief The member function that runs the action. */
		bool (CQPasteWnd::*handler)() = nullptr;
		/** @brief true: DoAction returns the handler's result; false: DoAction ignores it and returns false. */
		bool returnsResult{};
	};

	/** @brief One paste-by-position action: DoAction runs OpenIndex(index, plainText). */
	struct PastePositionAction
	{
		/** @brief The action this entry handles. */
		ActionEnums::ActionEnumValues action{};
		/** @brief The list row to paste. */
		int index{};
		/** @brief true: paste the clip as plain text only. */
		bool plainText{};
	};

	/** @brief One fixed transparency action: DoAction runs SetTransparency(percent) and returns false. */
	struct TransparencyAction
	{
		/** @brief The action this entry handles. */
		ActionEnums::ActionEnumValues action{};
		/** @brief The transparency in percent. */
		int percent{};
	};

	/** @brief A menu item that SetMenuChecks checks when an option has the given value. */
	struct MenuValueCheck
	{
		/** @brief The option value. */
		long value{};
		/** @brief The menu command ID to check. */
		UINT menuId{};
	};

	/** @brief The order, paste date and description of a clip as OnReloadClipInUI reads them from the database. */
	struct ReloadedClip
	{
		/** @brief The clip order in the main list. */
		double order{};
		/** @brief The clip order in its group. */
		double orderGroup{};
		/** @brief The last paste date. */
		__int64 lastPasted{};
		/** @brief The clip description (mText). */
		CString description{};
	};

	/** @brief The parts of the list queries that FillList builds. */
	struct FillListQuery
	{
		/** @brief The WHERE condition. */
		CString filter{};
		/** @brief The group condition (Main.lParentID = n), empty outside a group. */
		CString parentFilter{};
		/** @brief The ORDER BY columns. */
		CString sort{};
		/** @brief The JOIN of the Data table for a full text search, else empty. */
		CString dataJoin{};
		/** @brief "DISTINCT" when the Data join can return a clip more than once, else empty. */
		CString isDistinct{};
	};

	/** @brief The chosen export file name in parts, and the next number to try for numbered file names. */
	struct ExportFileNames
	{
		/** @brief The full path the user chose. */
		CString startingFilePath{};
		/** @brief The folder of the chosen path. */
		CString path{};
		/** @brief The file name without extension. */
		CString fileName{};
		/** @brief The file extension. */
		CString ext{};
		/** @brief The next number to try for a numbered file name. */
		int lastFileCheckId{ 1 };
	};

	/** @brief The sort key of the clip that OnMenuGoToEntry goes to. */
	struct GoToEntryKey
	{
		/** @brief The clip's stickyClipOrder. */
		int sticky{ 0 };
		/** @brief The clip's bIsGroup. */
		int isGroup{ 0 };
		/** @brief The clip's clipOrder. */
		int clipOrder{ 0 };
		/** @brief The clip's parent group ID. */
		long parent{ -1 };
	};

	/** @brief The actions DoAction runs through a member function without arguments. */
	static const std::array<ActionHandler, 88> s_actionHandlers;
	/** @brief The paste-by-position actions of DoAction. */
	static const std::array<PastePositionAction, 20> s_pastePositionActions;
	/** @brief The fixed transparency actions of DoAction. */
	static const std::array<TransparencyAction, 9> s_transparencyActions;
	/** @brief Transparency percent -> menu item. */
	static const std::array<MenuValueCheck, 8> s_transparencyMenuChecks;
	/** @brief Lines per row -> menu item. */
	static const std::array<MenuValueCheck, 5> s_linesPerRowMenuChecks;
	/** @brief Quick paste position -> menu item. */
	static const std::array<MenuValueCheck, 3> s_positionMenuChecks;
	/** @brief Caption position -> menu item. */
	static const std::array<MenuValueCheck, 4> s_captionPosMenuChecks;
	/** @brief Double click on caption setting -> menu item. */
	static const std::array<MenuValueCheck, 3> s_doubleClickCaptionMenuChecks;

	/** @brief Runs one entry of s_actionHandlers.
	@param entry the table entry.
	@return the handler's result, or false when the entry ignores it. */
	bool RunActionHandler(const ActionHandler& entry);
	/** @brief The MAKE_TOP_STICKY action: OnMakeTopSticky(false).
	@return the result of OnMakeTopSticky. */
	bool DoActionMakeTopSticky();
	/** @brief The SHOW_STARRED_CLIPS action: toggles the starred clips view.
	@return true. */
	bool DoActionShowStarredClips();

	/** @brief Adds the configured shortcuts of one user configurable action.
	@param action the action. */
	void LoadActionShortcuts(ActionEnums::ActionEnumValues action);
	/** @brief Adds one configured shortcut of an action to the action and tool tip accelerators.
	@param action the action.
	@param a the first key.
	@param b the second key. */
	void AddActionShortcut(ActionEnums::ActionEnumValues action, int a, int b);
	/** @brief The modifier of the shift variation of a key: shift, or none when the key already has shift.
	@param a the key.
	@return HOTKEYF_SHIFT or 0. */
	static int ShiftVariationModifier(int a);

	/** @brief Shows and places, or hides, the group name and back button.
	@param cx the client width.
	@return the top of the list box. */
	int MoveGroupHeader(int cx);
	/** @brief Updates, shows or hides the modern scroll bars by the options. */
	void UpdateModernScrollBars();

	/** @brief Is a window the list's tool tip window?
	@param pWndOther the window.
	@return true for the tool tip window. */
	bool IsListToolTipWnd(CWnd* pWndOther);
	/** @brief OnActivate's work when the window becomes inactive. */
	void OnDeactivateWindow();
	/** @brief OnActivate's work when the window becomes active.
	@param bMinimized the window is minimized. */
	void OnActivateWindow(BOOL bMinimized);
	/** @brief Must the list be filled again on activation (empty list, or a newer database on a network share)?
	@return TRUE to fill the list. */
	BOOL NeedsFillListOnActivate();

	/** @brief HideQPasteWindow's default for clearSearchData.
	@return FALSE when a search view is kept, else TRUE. */
	BOOL DefaultClearSearchData();
	/** @brief Minimizes the window when it shows in the task bar, else hides it. */
	void HideOrMinimizeWindow();
	/** @brief Clears the search text and, when needed, the list, on hiding the window. */
	void ClearSearchOnHide();
	/** @brief Goes back to the top level group or to the old group state on hiding the window. */
	void RestoreGroupOnHide();

	/** @brief Applies the reloaded fields of a clip to its list item.
	@param item the list item of the clip.
	@param reloaded the fields read from the database.
	@param updateFlags the UPDATE_* flags.
	@param clipId the clip ID.
	@return TRUE when the item was updated. */
	BOOL ApplyReloadedClip(CMainTable& item, const ReloadedClip& reloaded, int updateFlags, int clipId);

	/** @brief Sets the filter and sort of the list for the starred view, the main list or a group.
	@param query the query parts to set.
	@param strStarredFilter the starred clips condition. */
	void SetGroupFilter(FillListQuery& query, const CString& strStarredFilter);
	/** @brief The main list condition by the options.
	@return the condition. */
	CString MainListFilter() const;
	/** @brief Sets the filter of the list for a search text.
	@param csSQLSearch the search text; a /q or /f prefix is removed.
	@param query the query parts to set.
	@param strStarredFilter the starred clips condition. */
	void SetSearchFilter(CString& csSQLSearch, FillListQuery& query, const CString& strStarredFilter);
	/** @brief The description search condition.
	@param csSQLSearch the search text.
	@return the condition, or empty when the description is not searched. */
	CString SearchDescriptionSql(const CString& csSQLSearch) const;
	/** @brief The quick paste text search condition.
	@param csSQLSearch the search text; a /q prefix is removed.
	@return the condition, or empty when the quick paste text is not searched. */
	CString SearchQuickPasteSql(CString& csSQLSearch) const;
	/** @brief The full text search condition; sets the Data join and DISTINCT.
	@param csSQLSearch the search text; a /f prefix is removed.
	@param descriptionSql the description condition.
	@param quickPasteSql the quick paste text condition.
	@param query the query parts to set.
	@return the condition, or empty when the full text is not searched. */
	CString SearchFullTextSql(CString& csSQLSearch, const CString& descriptionSql, const CString& quickPasteSql, FillListQuery& query) const;
	/** @brief Joins the search conditions with OR, in parentheses.
	@param descriptionSql the description condition.
	@param quickPasteSql the quick paste text condition.
	@param fullTextSql the full text condition.
	@return the joined condition. */
	static CString JoinSearchSql(const CString& descriptionSql, const CString& quickPasteSql, const CString& fullTextSql);

	/** @brief Checks a menu item when a condition holds.
	@param pMenu the menu.
	@param condition the condition.
	@param menuId the menu command ID. */
	static void CheckMenuItemIf(CMenu* pMenu, BOOL condition, UINT menuId);
	/** @brief Checks the menu item of the table entry with the given value, if there is one.
	@param pMenu the menu.
	@param checks the value -> menu item table.
	@param value the option value. */
	static void CheckMenuItemForValue(CMenu* pMenu, std::span<const MenuValueCheck> checks, long value);

	/** @brief Runs the action of the middle mouse button. */
	void CheckMiddleClickActions();
	/** @brief The main frame, which runs the commands this window passes on (options, save, backup).
	@return the frame, or null (reported to the user) when it does not exist. */
	CMainFrame* MainFrameOrReport();
	/** @brief Tracks the active window on mouse moves over the inactive always-on-top window. */
	void TrackNonActiveMouseMove();
	/** @brief Runs the action of a key message, or starts a search with a character typed in the list.
	@param pMsg the message.
	@return true when the message was handled. */
	bool PreTranslateActionOrChar(MSG* pMsg);
	/** @brief Moves the focus to the search box and adds a character to the search.
	@param ch the character. */
	void StartSearchWithChar(TCHAR ch);

	/** @brief The list row of a clip.
	@param id the clip ID.
	@param notFoundRow the result when the clip is not in the list.
	@return the row, or notFoundRow. */
	int FindListRow(int id, int notFoundRow);
	/** @brief Removes the list item of a clip.
	@param id the clip ID. */
	void EraseListItem(int id);
	/**
	 * @brief Gives a clip the newest or the oldest order of the shown list (the open group's, or
	 *        the main list's) and saves it.
	 * @param clip The loaded clip.
	 * @param latest True for the newest order, false for the oldest.
	 * @return False when the order could not be read or the clip not saved (the error is shown).
	 */
	bool SaveClipAtListEdge(CClip& clip, bool latest);
	/** @brief Saves the file data of a clip with CF_HDROP data and reloads its list item.
	@param row the list row.
	@param id the clip ID.
	@param errorMessage the errors, added to.
	@return False when reloading the list item failed with a database error (shown to the user). */
	bool SaveClipFileData(int row, int id, CString& errorMessage);

	/** @brief The file path for the next exported clip: the chosen path, or the next free numbered path.
	@param names the chosen file name; its next number is advanced.
	@param clipCount the number of clips to export.
	@return the path, or empty when no free path was found. */
	static CString NextExportFilePath(ExportFileNames& names, INT_PTR clipCount);
	/** @brief Has a clip a bitmap or PNG format to export?
	@param toSave the clip, with its formats loaded.
	@return true when it has one. */
	static bool HasExportImage(CClip& toSave);

	/** @brief GetDispInfo's text of a row, or queues the row to load.
	@param pItem the list item. */
	void GetDispInfoText(LV_ITEM* pItem);
	/** @brief The list text of a clip: its symbol tags, "|" and its display text.
	@param item the list item.
	@return the text. */
	CString ListItemDisplayText(const CMainTable& item) const;
	/** @brief Is a clip sticky in the current view (group or main list)?
	@param item the list item.
	@return true when sticky. */
	static bool IsListItemSticky(const CMainTable& item);
	/** @brief Queues a list row to load, unless a queued range has it, and starts the load.
	@param item the list row. */
	void QueueListItemLoad(int item);
	/** @brief GetDispInfo's item data (clip ID) of a row.
	@param pItem the list item. */
	void GetDispInfoParam(LV_ITEM* pItem);
	/** @brief GetDispInfo's cached image or rich text of a row, or queues it to load.
	@param pItem the list item.
	@param cfType the clip format.
	@param noFormatCache the rows known not to have the format.
	@param formatCache the loaded formats. */
	void GetDispInfoExtraFormat(LV_ITEM* pItem, CLIPFORMAT cfType, CF_NoDibTypeMap& noFormatCache, CF_DibTypeMap& formatCache);
	/** @brief Queues a format of a list row to load, unless it is queued, and starts the load.
	@param row the list row.
	@param cfType the clip format. */
	void QueueExtraDataLoad(int row, CLIPFORMAT cfType);

	/** @brief Adds the clip details (ID, dates, flags, shortcut, sticky, group) to the tool tip.
	@param q the clip's query row.
	@param clipData the tool tip details, added to. */
	static void AppendToolTipClipDetails(CppSQLite3Query& q, CString& clipData);
	/** @brief Adds the clip's shortcut to the tool tip.
	@param q the clip's query row.
	@param clipData the tool tip details, added to. */
	static void AppendToolTipShortCut(CppSQLite3Query& q, CString& clipData);
	/** @brief Adds the clip's sticky state to the tool tip.
	@param q the clip's query row.
	@param clipData the tool tip details, added to. */
	static void AppendToolTipSticky(CppSQLite3Query& q, CString& clipData);

	/** @brief The search timer: fills the list with the search text. */
	void OnDoSearchTimer();
	/** @brief The paste-from-modifier timer: pastes the selection while the modifiers are active. */
	void OnPasteFromModifierTimer();
	/** @brief The drag timer: hides the window when the mouse left it. */
	void OnDragHideWindowTimer();

	/** @brief Reads the sort key of a clip.
	@param targetID the clip ID.
	@param key the sort key to set.
	@return true when the clip was found; false when it was not found or on a database error
	(the error is reported). */
	static bool LoadGoToEntryKey(long targetID, GoToEntryKey& key);
	/** @brief The row of a clip in the main list.
	@param filter the main list condition.
	@param key the clip's sort key.
	@return the row, or -1 on a database error (the error is reported). */
	static int GoToEntryRank(const CString& filter, const GoToEntryKey& key);
	/** @brief Waits up to 5 s for the list load, pumping messages. */
	void WaitForListLoad();
};
