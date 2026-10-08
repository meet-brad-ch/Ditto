// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "stdafx.h"
#include "FileDialogPath.h"
#include "..\Shared\Tokenizer.h"
#include ".\qpastewnd.h"
#include "ClipboardFormatError.h"
#include "ErrorReport.h"
#include "ActionEnums.h"
#include "CF_TextAggregator.h"
#include "CF_UnicodeTextAggregator.h"
#include "..\Shared\TextConvert.h"
#include "ClipCompare.h"
#include "ControlTextBuffer.h"
#include "CopyProperties.h"
#include "CP_Main.h"
#include "DimWnd.h"
#include "FormatSQL.h"
#include "GroupName.h"
#include "htmlformataggregator.h"
#include "MainTableFunctions.h"
#include "Misc.h"
#include "MoveToGroupDlg.h"
#include "Path.h"
#include "ProcessPaste.h"
#include "QPasteWnd.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "QrBitmap.h"
#include "QRCodeViewer.h"

/////////////////////////////////////////////////////////////////////////////
// CQPasteWnd

CQPasteWnd::CQPasteWnd() :
	m_thread(theApp.Services().Settings()),
	m_extraDataThread(theApp.Services().Settings())
{
	m_Title = s_qpasteTitle;
	m_bHideWnd = true;
	m_strSQLSearch = "";
	m_strSearch = "";
	m_bHandleSearchTextChange = true;
	m_bModifersMoveActive = false;
	m_showScrollBars = false;
	m_leftSelectedCompareId = 0;
	m_extraDataCounter = 0;
	m_noSearchResults = false;
	m_bShowStarredClips = false;
	m_lastDbWrite = 0;
	m_pendingRefresh = false;
	m_lastNonActiveMouseMove = 0;
}

CQPasteWnd::~CQPasteWnd()
{
}

CGetSetOptions& CQPasteWnd::Settings() const
{
	return theApp.Services().Settings();
}

BEGIN_MESSAGE_MAP(CQPasteWnd, CWndEx)
	//{{AFX_MSG_MAP(CQPasteWnd)
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_ACTIVATE()
	ON_COMMAND(ID_MENU_LINESPERROW_1, OnMenuLinesperrow1)
	ON_COMMAND(ID_MENU_LINESPERROW_2, OnMenuLinesperrow2)
	ON_COMMAND(ID_MENU_LINESPERROW_3, OnMenuLinesperrow3)
	ON_COMMAND(ID_MENU_LINESPERROW_4, OnMenuLinesperrow4)
	ON_COMMAND(ID_MENU_LINESPERROW_5, OnMenuLinesperrow5)
	ON_COMMAND(ID_MENU_TRANSPARENCY_10, OnMenuTransparency10)
	ON_COMMAND(ID_MENU_TRANSPARENCY_15, OnMenuTransparency15)
	ON_COMMAND(ID_MENU_TRANSPARENCY_20, OnMenuTransparency20)
	ON_COMMAND(ID_MENU_TRANSPARENCY_25, OnMenuTransparency25)
	ON_COMMAND(ID_MENU_TRANSPARENCY_30, OnMenuTransparency30)
	ON_COMMAND(ID_MENU_TRANSPARENCY_40, OnMenuTransparency40)
	ON_COMMAND(ID_MENU_TRANSPARENCY_5, OnMenuTransparency5)
	ON_COMMAND(ID_MENU_TRANSPARENCY_NONE, OnMenuTransparencyNone)
	ON_COMMAND(ID_MENU_DELETE, OnMenuDelete)
	ON_COMMAND(ID_MENU_POSITIONING_ATCARET, OnMenuPositioningAtcaret)
	ON_COMMAND(ID_MENU_POSITIONING_ATCURSOR, OnMenuPositioningAtcursor)
	ON_COMMAND(ID_MENU_POSITIONING_ATPREVIOUSPOSITION, OnMenuPositioningAtpreviousposition)
	ON_COMMAND(ID_MENU_OPTIONS, OnMenuOptions)
	ON_COMMAND(ID_MENU_EXITPROGRAM, OnMenuExitprogram)
	ON_COMMAND(ID_MENU_TOGGLECONNECTCV, OnMenuToggleConnectCV)
	ON_COMMAND(ID_MENU_PROPERTIES, OnMenuProperties)
	ON_WM_CLOSE()
	ON_NOTIFY(LVN_BEGINDRAG, IdListHeader, OnBegindrag)
	ON_WM_SYSKEYDOWN()
	ON_NOTIFY(LVN_GETDISPINFO, IdListHeader, GetDispInfo)
	ON_NOTIFY(LVN_ODFINDITEM, IdListHeader, OnFindItem)
	ON_COMMAND(ID_MENU_FIRSTTENHOTKEYS_USECTRLNUM, OnMenuFirsttenhotkeysUsectrlnum)
	ON_COMMAND(ID_MENU_FIRSTTENHOTKEYS_SHOWHOTKEYTEXT, OnMenuFirsttenhotkeysShowhotkeytext)
	ON_COMMAND(ID_MENU_QUICKOPTIONS_ALLWAYSSHOWDESCRIPTION, OnMenuQuickoptionsAllwaysshowdescription)
	ON_COMMAND(ID_MENU_QUICKOPTIONS_DOUBLECLICKINGONCAPTION_TOGGLESALWAYSONTOP, OnMenuQuickoptionsDoubleclickingoncaptionTogglesalwaysontop)
	ON_COMMAND(ID_MENU_QUICKOPTIONS_DOUBLECLICKINGONCAPTION_ROLLUPWINDOW, OnMenuQuickoptionsDoubleclickingoncaptionRollupwindow)
	ON_COMMAND(ID_MENU_QUICKOPTIONS_DOUBLECLICKINGONCAPTION_TOGGLESALWAYSSHOWDESCRIPTION, OnMenuQuickoptionsDoubleclickingoncaptionTogglesshowdescription)
	ON_COMMAND(ID_MENU_QUICKOPTIONS_PROMPTFORNEWGROUPNAMES, OnMenuQuickoptionsPromptfornewgroupnames)
	ON_BN_CLICKED(IdShowGroupsBottom, OnShowGroupsBottom)
	ON_BN_CLICKED(IdShowGroupsTop, OnShowGroupsTop)
	ON_COMMAND(ID_MENU_VIEWGROUPS, OnMenuViewgroups)
	ON_COMMAND(ID_MENU_QUICKPROPERTIES_SETTONEVERAUTODELETE, OnMenuQuickpropertiesSettoneverautodelete)
	ON_COMMAND(ID_MENU_QUICKPROPERTIES_AUTODELETE, OnMenuQuickpropertiesAutodelete)
	ON_COMMAND(ID_MENU_QUICKPROPERTIES_REMOVEHOTKEY, OnMenuQuickpropertiesRemovehotkey)
	ON_COMMAND(ID_MENU_GROUPS_MOVETOGROUP, OnMenuGroupsMovetogroup)
	ON_COMMAND(ID_MENU_PASTEPLAINTEXTONLY, OnMenuPasteplaintextonly)
	ON_COMMAND(ID_MENU_QUICKOPTIONS_FONT, OnMenuQuickoptionsFont)
	ON_COMMAND(ID_MENU_QUICKOPTIONS_SHOWTHUMBNAILS, OnMenuQuickoptionsShowthumbnails)
	ON_COMMAND(ID_MENU_QUICKOPTIONS_DRAWRTFTEXT, OnMenuQuickoptionsDrawrtftext)
	ON_COMMAND(ID_MENU_QUICKOPTIONS_PASTECLIPAFTERSELECTION, OnMenuQuickoptionsPasteclipafterselection)
	ON_EN_CHANGE(IdEditSearch, OnSearchEditChange)
	ON_COMMAND(ID_MENU_QUICKOPTIONS_FINDASYOUTYPE, OnMenuQuickoptionsFindasyoutype)
	ON_COMMAND(ID_MENU_QUICKOPTIONS_ENSUREENTIREWINDOWISVISIBLE, OnMenuQuickoptionsEnsureentirewindowisvisible)
	ON_COMMAND(ID_MENU_QUICKOPTIONS_SHOWCLIPSTHATAREINGROUPSINMAINLIST, OnMenuQuickoptionsShowclipsthatareingroupsinmainlist)
	ON_UPDATE_COMMAND_UI(ID_MENU_NEWGROUP, OnUpdateMenuNewgroup)
	ON_UPDATE_COMMAND_UI(ID_MENU_NEWGROUPSELECTION, OnUpdateMenuNewgroupselection)
	ON_UPDATE_COMMAND_UI(ID_MENU_ALLWAYSONTOP, OnUpdateMenuAllwaysontop)
	ON_UPDATE_COMMAND_UI(ID_MENU_VIEWFULLDESCRIPTION, OnUpdateMenuViewfulldescription)
	ON_UPDATE_COMMAND_UI(ID_MENU_VIEWGROUPS, OnUpdateMenuViewgroups)
	ON_UPDATE_COMMAND_UI(ID_MENU_PASTEPLAINTEXTONLY, OnUpdateMenuPasteplaintextonly)
	ON_UPDATE_COMMAND_UI(ID_MENU_DELETE, OnUpdateMenuDelete)
	ON_UPDATE_COMMAND_UI(ID_MENU_PROPERTIES, OnUpdateMenuProperties)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_POSIXIFY_PATHS, &CQPasteWnd::OnUpdateSpecialPosixifyPaths)
	ON_COMMAND(ID_QUICKOPTIONS_PROMPTTODELETECLIP, OnPromptToDeleteClip)
	ON_COMMAND(ID_STICKYCLIPS_MAKETOPSTICKYCLIP, OnMakeTopStickyClip)
	ON_COMMAND(ID_STICKYCLIPS_MAKELASTSTICKYCLIP, OnMakeLastStickyClip)
	ON_COMMAND(ID_STICKYCLIPS_REMOVESTICKYSETTING, OnRemoveSticky)
	ON_COMMAND(ID_QUICKOPTIONS_ELEVATEPREVILEGESTOPASTEINTOELEVATEDAPPS, OnElevateAppToPasteIntoElevatedApp)

	ON_WM_DESTROY()

	//}}AFX_MSG_MAP
	ON_MESSAGE(CQListCtrl::NmSearchEnterPressed, OnSearchEnterKeyPressed)
	ON_MESSAGE(CQListCtrl::NmCopyClip, OnCopyClip)
	ON_MESSAGE(CQListCtrl::NmEnd, OnListEnd)
	ON_MESSAGE(CQListCtrl::CbSearch, OnSearch)
	ON_MESSAGE(CQListCtrl::NmDelete, OnDelete)
	ON_NOTIFY(CQListCtrl::NmGetToolTipText, IdListHeader, OnGetToolTipText)
	ON_MESSAGE(CQListCtrl::NmSelectDbId, OnListSelect_DB_ID)
	ON_MESSAGE(CDittoMessage::RefreshView, OnRefreshView)
	ON_MESSAGE(CDittoMessage::ReloadClipInUi, OnReloadClipInUI)
	ON_WM_NCLBUTTONDBLCLK()
	ON_WM_WINDOWPOSCHANGING()
	ON_COMMAND(ID_VIEWCAPTIONBARON_RIGHT, OnViewcaptionbaronRight)
	ON_COMMAND(ID_VIEWCAPTIONBARON_BOTTOM, OnViewcaptionbaronBottom)
	ON_COMMAND(ID_VIEWCAPTIONBARON_LEFT, OnViewcaptionbaronLeft)
	ON_COMMAND(ID_VIEWCAPTIONBARON_TOP, OnViewcaptionbaronTop)
	ON_COMMAND(ID_MENU_AUTOHIDE, OnMenuAutohide)
	ON_COMMAND(ID_MENU_VIEWFULLDESCRIPTION, OnMenuViewfulldescription)
	ON_COMMAND(ID_MENU_ALLWAYSONTOP, OnMenuAllwaysontop)
	ON_COMMAND(ID_MENU_NEWGROUP, OnMenuNewGroup)
	ON_COMMAND(ID_MENU_NEWGROUPSELECTION, OnMenuNewGroupSelection)
	ON_MESSAGE(CQListCtrl::NmGroupTreeMessage, OnGroupTreeMessage)
	ON_COMMAND(IdBackButton, OnBackButton)
	ON_COMMAND(IdSystemButton, OnSystemButton)
	ON_MESSAGE(CQListCtrl::CbUpDown, OnUpDown)
	ON_MESSAGE(CQListCtrl::NmInactiveToolTipWnd, OnToolTipWndInactive)
	ON_MESSAGE(CQListCtrl::NmSetListCount, OnSetListCount)
	ON_MESSAGE(CQListCtrl::NmRefreshRow, OnRefeshRow)
	ON_MESSAGE(CQListCtrl::NmItemDeleted, OnItemDeleted)
	ON_WM_TIMER()
	ON_COMMAND(ID_MENU_EXPORT, OnMenuExport)
	ON_COMMAND(ID_MENU_IMPORT, OnMenuImport)
	ON_COMMAND(ID_QUICKPROPERTIES_REMOVEQUICKPASTE, OnQuickpropertiesRemovequickpaste)
	ON_COMMAND(ID_MENU_EDITITEM, OnMenuEdititem)
	ON_COMMAND(ID_MENU_NEWCLIP, OnMenuNewclip)
	ON_UPDATE_COMMAND_UI(ID_MENU_EDITITEM, OnUpdateMenuEdititem)
	ON_UPDATE_COMMAND_UI(ID_MENU_NEWCLIP, OnUpdateMenuNewclip)
	ON_WM_CTLCOLOR_REFLECT()
	ON_COMMAND_RANGE(3000, 4000, OnAddinSelect)
	ON_MESSAGE(CQListCtrl::NmAllSelected, OnSelectAll)
	ON_MESSAGE(CQListCtrl::NmShowHideScrollBars, OnShowHideScrollBar)
	ON_MESSAGE(CQListCtrl::NmUpdateScrollBar, OnUpdateScrollBar)
	ON_MESSAGE(CQListCtrl::NmCancelSearch, OnCancelFilter)
	ON_MESSAGE(CQListCtrl::NmPostOptionsWindow, OnPostOptions)
	ON_COMMAND(ID_MENU_SEARCHDESCRIPTION, OnMenuSearchDescription)
	ON_COMMAND(ID_MENU_SEARCHFULLTEXT, OnMenuSearchFullText)
	ON_COMMAND(ID_MENU_SEARCHQUICKPASTE, OnMenuSearchQuickPaste)
	ON_COMMAND(ID_MENU_SHOWSTARREDCLIPS, OnMenuShowStarredClips)
	ON_COMMAND(ID_MENU_CONTAINSTEXTSEARCHONLY, OnMenuSimpleTextSearch)
	//ON_WM_CTLCOLOR()
	//ON_WM_ERASEBKGND()
	//ON_WM_PAINT()
	ON_COMMAND(ID_QUICKOPTIONS_SHOWINTASKBAR, &CQPasteWnd::OnQuickoptionsShowintaskbar)
	ON_COMMAND(ID_MENU_VIEWASQRCODE, &CQPasteWnd::OnMenuViewasqrcode)
	ON_COMMAND(ID_EXPORT_EXPORTTOTEXTFILE, &CQPasteWnd::OnExportExporttotextfile)
	ON_COMMAND(ID_COMPARE_COMPARE, &CQPasteWnd::OnCompareCompare)
	ON_COMMAND(ID_COMPARE_SELECTLEFTCOMPARE, &CQPasteWnd::OnCompareSelectleftcompare)
	ON_COMMAND(ID_COMPARE_COMPAREAGAINST, &CQPasteWnd::OnCompareCompareagainst)
	ON_UPDATE_COMMAND_UI(ID_COMPARE_COMPARE, &CQPasteWnd::OnUpdateCompareCompare)
	ON_MESSAGE(CQListCtrl::NmShowProperties, OnShowProperties)
	ON_MESSAGE(CQListCtrl::NmNewGroup, OnNewGroup)
	ON_MESSAGE(CQListCtrl::NmDeleteId, OnDeleteId)
	ON_COMMAND(ID_MENU_REGULAREXPRESSIONSEARCH, &CQPasteWnd::OnMenuRegularexpressionsearch)

	ON_COMMAND(ID_IMPORT_EXPORTCLIP_BITMAP, &CQPasteWnd::OnImportExportclipBitmap)
	ON_UPDATE_COMMAND_UI(ID_IMPORT_EXPORTCLIP_BITMAP, &CQPasteWnd::OnUpdateImportExportclipBitmap)

	ON_COMMAND(ID_MENU_WILDCARDSEARCH, &CQPasteWnd::OnMenuWildcardsearch)

	ON_COMMAND(ID_MENU_SAVECURRENTCLIPBOARD, &CQPasteWnd::OnMenuSavecurrentclipboard)
	ON_UPDATE_COMMAND_UI(ID_MENU_SAVECURRENTCLIPBOARD, &CQPasteWnd::OnUpdateMenuSavecurrentclipboard)
	ON_MESSAGE(CQListCtrl::NmMoveToGroup, OnListMoveSelectionToGroup)
	ON_COMMAND(ID_CLIPORDER_MOVEUP, &CQPasteWnd::OnCliporderMoveup)
	ON_UPDATE_COMMAND_UI(ID_CLIPORDER_MOVEUP, &CQPasteWnd::OnUpdateCliporderMoveup)
	ON_COMMAND(ID_CLIPORDER_MOVEDOWN, &CQPasteWnd::OnCliporderMovedown)
	ON_UPDATE_COMMAND_UI(ID_CLIPORDER_MOVEDOWN, &CQPasteWnd::OnUpdateCliporderMovedown)
	ON_COMMAND(ID_CLIPORDER_MOVETOTOP, &CQPasteWnd::OnCliporderMovetotop)
	ON_UPDATE_COMMAND_UI(ID_CLIPORDER_MOVETOTOP, &CQPasteWnd::OnUpdateCliporderMovetotop)
	ON_COMMAND(ID_MENU_FILTERON, &CQPasteWnd::OnMenuFilteron)
	ON_UPDATE_COMMAND_UI(ID_MENU_FILTERON, &CQPasteWnd::OnUpdateMenuFilteron)
	ON_COMMAND(ID_MENU_GOTOENTRY, &CQPasteWnd::OnMenuGoToEntry)
	ON_UPDATE_COMMAND_UI(ID_MENU_GOTOENTRY, &CQPasteWnd::OnUpdateMenuGoToEntry)
	ON_BN_CLICKED(IdOnTopWarning, OnAlwaysOnTopClicked)
	//ON_WM_CTLCOLOR()
	ON_COMMAND(ID_SPECIALPASTE_UPPERCASE, &CQPasteWnd::OnSpecialpasteUppercase)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_UPPERCASE, &CQPasteWnd::OnUpdateSpecialpasteUppercase)
	ON_COMMAND(ID_SPECIALPASTE_LOWERCASE, &CQPasteWnd::OnSpecialpasteLowercase)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_LOWERCASE, &CQPasteWnd::OnUpdateSpecialpasteLowercase)
	ON_COMMAND(ID_SPECIALPASTE_CAPITALIZE, &CQPasteWnd::OnSpecialpasteCapitalize)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_CAPITALIZE, &CQPasteWnd::OnUpdateSpecialpasteCapitalize)
	ON_COMMAND(ID_SPECIALPASTE_SENTENCE, &CQPasteWnd::OnSpecialpasteSentence)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_SENTENCE, &CQPasteWnd::OnUpdateSpecialpasteSentence)
	ON_COMMAND(ID_SPECIALPASTE_REMOVELINEFEEDS, &CQPasteWnd::OnSpecialpasteRemovelinefeeds)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_REMOVELINEFEEDS, &CQPasteWnd::OnUpdateSpecialpasteRemovelinefeeds)
	ON_COMMAND(ID_SPECIALPASTE_PASTE, &CQPasteWnd::OnSpecialpastePaste)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_PASTE, &CQPasteWnd::OnUpdateSpecialpastePaste)
	ON_COMMAND(ID_SPECIALPASTE_PASTE32919, &CQPasteWnd::OnSpecialpastePaste32919)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_PASTE32919, &CQPasteWnd::OnUpdateSpecialpastePaste32919)
	ON_COMMAND(ID_SPECIALPASTE_TYPOGLYCEMIA, &CQPasteWnd::OnSpecialpasteTypoglycemia)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_TYPOGLYCEMIA, &CQPasteWnd::OnUpdateSpecialpasteTypoglycemia)
	ON_NOTIFY(NM_CLICK, IdListHeader, &CQPasteWnd::OnNMClickList1)
	ON_NOTIFY(NM_DBLCLK, IdListHeader, &CQPasteWnd::OnNMDblclkList1)
	ON_NOTIFY(NM_RCLICK, IdListHeader, &CQPasteWnd::OnNMRClickList1)
	ON_NOTIFY(NM_RDBLCLK, IdListHeader, &CQPasteWnd::OnNMRDblclkList1)
	ON_COMMAND(ID_QUICKOPTIONS_SHOWTEXTFORFIRSTTENCOPYHOTKEYS, &CQPasteWnd::OnQuickoptionsShowtextforfirsttencopyhotkeys)
	ON_UPDATE_COMMAND_UI(ID_QUICKOPTIONS_SHOWTEXTFORFIRSTTENCOPYHOTKEYS, &CQPasteWnd::OnUpdateQuickoptionsShowtextforfirsttencopyhotkeys)
	ON_COMMAND(ID_QUICKOPTIONS_SHOWINDICATORACLIPHASBEENPASTED, &CQPasteWnd::OnQuickoptionsShowindicatoracliphasbeenpasted)
	ON_UPDATE_COMMAND_UI(ID_QUICKOPTIONS_SHOWINDICATORACLIPHASBEENPASTED, &CQPasteWnd::OnUpdateQuickoptionsShowindicatoracliphasbeenpasted)
	ON_COMMAND(ID_GROUPS_TOGGLELASTGROUP, &CQPasteWnd::OnGroupsTogglelastgroup)
	ON_UPDATE_COMMAND_UI(ID_GROUPS_TOGGLELASTGROUP, &CQPasteWnd::OnUpdateGroupsTogglelastgroup)
	ON_UPDATE_COMMAND_UI(ID_STICKYCLIPS_MAKETOPSTICKYCLIP, &CQPasteWnd::OnUpdateStickyclipsMaketopstickyclip)
	ON_UPDATE_COMMAND_UI(ID_STICKYCLIPS_MAKELASTSTICKYCLIP, &CQPasteWnd::OnUpdateStickyclipsMakelaststickyclip)
	ON_UPDATE_COMMAND_UI(ID_STICKYCLIPS_REMOVESTICKYSETTING, &CQPasteWnd::OnUpdateStickyclipsRemovestickysetting)
	ON_COMMAND(ID_SPECIALPASTE_PASTE32927, &CQPasteWnd::OnSpecialpastePaste32927)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_PASTE32927, &CQPasteWnd::OnUpdateSpecialpastePaste32927)
	ON_COMMAND(ID_MENU_GLOBALHOTKEYS32933, &CQPasteWnd::OnMenuGlobalhotkeys32933)
	ON_COMMAND(ID_MENU_DELETECLIPDATA32934, &CQPasteWnd::OnMenuDeleteclipdata32934)
	ON_COMMAND(ID_MENU_IMPORTCLIP32935, &CQPasteWnd::OnMenuImportclip32935)
	ON_COMMAND(ID_MENU_NEWCLIP32937, &CQPasteWnd::OnMenuNewclip32937)
	ON_UPDATE_COMMAND_UI(ID_MENU_IMPORTCLIP32935, &CQPasteWnd::OnUpdateMenuImportclip32935)
	ON_UPDATE_COMMAND_UI(ID_MENU_NEWCLIP32937, &CQPasteWnd::OnUpdateMenuNewclip32937)
	ON_UPDATE_COMMAND_UI(ID_MENU_GLOBALHOTKEYS32933, &CQPasteWnd::OnUpdateMenuGlobalhotkeys32933)
	ON_UPDATE_COMMAND_UI(ID_MENU_DELETECLIPDATA32934, &CQPasteWnd::OnUpdateMenuDeleteclipdata32934)
	ON_MESSAGE(CQListCtrl::NmFocusOnSearch, OnSearchFocused)
	ON_COMMAND(ID_CLIPORDER_REPLACETOPSTICKYCLIP, &CQPasteWnd::OnCliporderReplacetopstickyclip)
	ON_UPDATE_COMMAND_UI(ID_CLIPORDER_REPLACETOPSTICKYCLIP, &CQPasteWnd::OnUpdateCliporderReplacetopstickyclip)
	ON_COMMAND(ID_IMPORT_IMPORTCOPIEDFILE, &CQPasteWnd::OnImportImportcopiedfile)
	ON_UPDATE_COMMAND_UI(ID_IMPORT_IMPORTCOPIEDFILE, &CQPasteWnd::OnUpdateImportImportcopiedfile)
	ON_UPDATE_COMMAND_UI(32775, &CQPasteWnd::OnUpdate32775)
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
	ON_COMMAND(ID_CLIPORDER_MOVETOLAST, &CQPasteWnd::OnCliporderMovetolast)
	ON_UPDATE_COMMAND_UI(ID_CLIPORDER_MOVETOLAST, &CQPasteWnd::OnUpdateCliporderMovetolast)
	ON_COMMAND(ID_SPECIALPASTE_PASTE32945, &CQPasteWnd::OnSpecialpastePasteDontUpdateOrder)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_PASTE32945, &CQPasteWnd::OnUpdateOnSpecialPasteDontUpdateOrder)
	ON_COMMAND(ID_SPECIALPASTE_TRIM, &CQPasteWnd::OnSpecialpasteTrim)
	ON_COMMAND(ID_SPECIALPASTE_POSIXIFY_PATHS , &CQPasteWnd::OnSpecialpastePosixifyPaths)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_TRIM, &CQPasteWnd::OnUpdateSpecialpasteTrim)
	ON_COMMAND(ID_TRANSPARENCY_INCREASE, &CQPasteWnd::OnTransparencyIncrease)
	ON_UPDATE_COMMAND_UI(ID_TRANSPARENCY_INCREASE, &CQPasteWnd::OnUpdateTransparencyIncrease)
	ON_COMMAND(ID_TRANSPARENCY_DECREASE, &CQPasteWnd::OnTransparencyDecrease)
	ON_UPDATE_COMMAND_UI(ID_TRANSPARENCY_DECREASE, &CQPasteWnd::OnUpdateTransparencyDecrease)
	ON_COMMAND(ID_TRANSPARENCY_TOGGLE, &CQPasteWnd::OnTransparencyToggle)
	ON_UPDATE_COMMAND_UI(ID_TRANSPARENCY_TOGGLE, &CQPasteWnd::OnUpdateTransparencyToggle)
	ON_UPDATE_COMMAND_UI(ID_MENU_TRANSPARENCY_NONE, &CQPasteWnd::OnUpdateTransparencyNone)
	ON_UPDATE_COMMAND_UI(ID_MENU_TRANSPARENCY_5, &CQPasteWnd::OnUpdateTransparency5)
	ON_UPDATE_COMMAND_UI(ID_MENU_TRANSPARENCY_10, &CQPasteWnd::OnUpdateTransparency10)
	ON_UPDATE_COMMAND_UI(ID_MENU_TRANSPARENCY_15, &CQPasteWnd::OnUpdateTransparency15)
	ON_UPDATE_COMMAND_UI(ID_MENU_TRANSPARENCY_20, &CQPasteWnd::OnUpdateTransparency20)
	ON_UPDATE_COMMAND_UI(ID_MENU_TRANSPARENCY_25, &CQPasteWnd::OnUpdateTransparency25)
	ON_UPDATE_COMMAND_UI(ID_MENU_TRANSPARENCY_30, &CQPasteWnd::OnUpdateTransparency30)
	ON_UPDATE_COMMAND_UI(ID_TRANSPARENCY_35, &CQPasteWnd::OnUpdateTransparency35)
	ON_UPDATE_COMMAND_UI(ID_MENU_TRANSPARENCY_40, &CQPasteWnd::OnUpdateTransparency40)
	ON_COMMAND(ID_TRANSPARENCY_35, &CQPasteWnd::OnTransparency35)
	ON_COMMAND(ID_SPECIALPASTE_SLUGIFY, &CQPasteWnd::OnSpecialpasteSlugify)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_SLUGIFY, &CQPasteWnd::OnUpdateSpecialpasteSlugify)
	ON_COMMAND(ID_SPECIALPASTE_TOGGLECASE, &CQPasteWnd::OnSpecialpasteTogglecase)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_TOGGLECASE, &CQPasteWnd::OnUpdateSpecialpasteTogglecase)
	ON_COMMAND(ID_FIRST_SHOWSTARTUPMESSAGE, &CQPasteWnd::OnFirstShowstartupmessage)

	ON_COMMAND(ID_MENU_RESTOREDATABSAE, &CQPasteWnd::OnFirstRestoreDb)
	ON_COMMAND(ID_MENU_BACKUPDATABASE, &CQPasteWnd::OnFirstBackupDb)
	ON_COMMAND(ID_MENU_DELETEALLNONUSEDCLIPS, &CQPasteWnd::OnMenuDeleteallnonusedclips)
	ON_UPDATE_COMMAND_UI(ID_MENU_DELETEALLNONUSEDCLIPS, &CQPasteWnd::OnUpdateMenuDeleteallnonusedclips)
	ON_COMMAND(ID_IMPORT_SETDRAGFILENAME, &CQPasteWnd::OnImportSetdragfilename)
	ON_UPDATE_COMMAND_UI(ID_IMPORT_SETDRAGFILENAME, &CQPasteWnd::OnUpdateImportSetdragfilename)
	ON_COMMAND(ID_SPECIALPASTE_CAMELCASE, &CQPasteWnd::OnSpecialpasteCamelcase)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_CAMELCASE, &CQPasteWnd::OnUpdateSpecialpasteCamelcase)

	ON_COMMAND(ID_SPECIALPASTE_MULTIPLEIMAGESHORIZONTALLY, &CQPasteWnd::OnSpecialpasteMultipleImagesHorz)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_MULTIPLEIMAGESHORIZONTALLY, &CQPasteWnd::OnUpdateSpecialpasteMultipleImagesHorz)

	ON_COMMAND(ID_SPECIALPASTE_MULTIPLEIMAGESVERTICALLY, &CQPasteWnd::OnSpecialpasteMultipleImagesVert)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_MULTIPLEIMAGESVERTICALLY, &CQPasteWnd::OnUpdateSpecialpasteMultipleImagesVert)

	ON_COMMAND(ID_SPECIALPASTE_ASCIITEXTONLY, &CQPasteWnd::OnSpecialpasteAsciitextonly)
	ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_ASCIITEXTONLY, &CQPasteWnd::OnUpdateSpecialpasteAsciitextonly)
		ON_COMMAND(ID_SPECIALPASTE_PASTENEWGUID, &CQPasteWnd::OnSpecialpastePastenewguid)
		ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_PASTENEWGUID, &CQPasteWnd::OnUpdateSpecialpastePastenewguid)
		ON_COMMAND(ID_SPECIALPASTE_PASTEASIMAGE, &CQPasteWnd::OnSpecialpastePasteAsImage)
		ON_UPDATE_COMMAND_UI(ID_SPECIALPASTE_PASTEASIMAGE, &CQPasteWnd::OnUpdateSpecialpastePasteAsImage)
		END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CQPasteWnd message handlers

HBRUSH CQPasteWnd::CtlColor(CDC* pDC, UINT /*nCtlColor*/)
{
	pDC->SetBkMode(TRANSPARENT);
	pDC->SetBkColor(RGB(255, 0, 0));

	return (HBRUSH)GetStockObject(NULL_BRUSH);
}

BOOL CQPasteWnd::Create(CRect rect, CWnd* pParentWnd)
{
	return CWndEx::Create(rect, pParentWnd);
}

int CQPasteWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWndEx::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	HICON b = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 64, 64, LR_SHARED);
	SetIcon(b, TRUE);

	//BOOL b = this->Register(this);

	SetWindowText(s_qpasteTitle);

	m_search.Create(WS_TABSTOP | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, CRect(0, 0, 0, 0), this, IdEditSearch);
	m_search.SetDpiInfo(&m_DittoWindow.m_dpi);
	m_search.SetPromptText(theApp.m_Language.GetString(_T("Search"), _T("Search")));
	::SHAutoComplete(m_search.m_hWnd, SHACF_AUTOSUGGEST_FORCE_OFF);
	SetSearchImages();
	m_search.LoadPastSearches(Settings().GetPastSearchXml());

	CRect rcEditArea(m_DittoWindow.m_dpi.Scale(4), m_DittoWindow.m_dpi.Scale(2), m_DittoWindow.m_dpi.Scale(20), m_DittoWindow.m_dpi.Scale(2));
	//m_search.SetBorder(rcEditArea);

	CRect rcCloseArea(m_DittoWindow.m_dpi.Scale(85), m_DittoWindow.m_dpi.Scale(3), m_DittoWindow.m_dpi.Scale(99), m_DittoWindow.m_dpi.Scale(15));
	//m_search.SetButtonArea(rcCloseArea);

	// Create the header control
	if (!m_lstHeader.Create(WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | LVS_NOCOLUMNHEADER | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA | LVS_OWNERDRAWFIXED, CRect(0, 0, 0, 0), this, IdListHeader))
	{
		ASSERT(FALSE);
		return -1;
	}
	m_lstHeader.SetDpiInfo(&m_DittoWindow.m_dpi);
	m_lstHeader.ShowWindow(SW_SHOW);

	// Create modern scrollbar overlay (vertical)
	m_modernScrollBar.Create(this, &m_lstHeader, ScrollBarOrientation::Vertical);
	m_modernScrollBar.SetDPI(&m_DittoWindow.m_dpi);
	m_modernScrollBar.SetColors(
		Settings().m_Theme.ScrollBarTrack(),
		Settings().m_Theme.ScrollBarThumb(),
		Settings().m_Theme.ScrollBarThumbHover()
	);

	// Create modern scrollbar overlay (horizontal)
	m_modernScrollBarHorz.Create(this, &m_lstHeader, ScrollBarOrientation::Horizontal);
	m_modernScrollBarHorz.SetDPI(&m_DittoWindow.m_dpi);
	m_modernScrollBarHorz.SetColors(
		Settings().m_Theme.ScrollBarTrack(),
		Settings().m_Theme.ScrollBarThumb(),
		Settings().m_Theme.ScrollBarThumbHover()
	);

	((CWnd*)&m_GroupTree)->CreateEx(NULL, _T("SysTreeView32"), NULL, TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, CRect(0, 0, 100, 100), this, 0);
	m_GroupTree.ModifyStyle(WS_CAPTION | WS_TABSTOP, 0);

	m_GroupTree.SetNotificationWndEx(m_hWnd);
	m_GroupTree.ShowWindow(SW_HIDE);
	m_GroupTree.m_showRightClickMenu = true;

	m_ShowGroupsFolderBottom.Create(NULL, WS_CHILD | BS_OWNERDRAW | WS_TABSTOP, CRect(0, 0, 0, 0), this, IdShowGroupsBottom);
	//m_ShowGroupsFolderBottom.LoadBitmaps(IDB_CLOSED_FOLDER, IDB_CLOSED_FOLDER_PRESSED, IDB_CLOSED_FOLDER_FOCUSED);
	m_ShowGroupsFolderBottom.LoadStdImageDPI(m_DittoWindow.m_dpi.GetDPI(), open_folder_24, open_folder_30, open_folder_36, open_folder_42, open_folder_48, _T("PNG"), open_folder_54, open_folder_60, open_folder_66, open_folder_72, open_folder_78, open_folder_84);
	m_ShowGroupsFolderBottom.ShowWindow(SW_SHOW);
	m_ShowGroupsFolderBottom.SetToolTipText(theApp.m_Language.GetString(_T("GroupsTooltip"), _T("Groups")));
	m_ShowGroupsFolderBottom.ModifyStyle(WS_TABSTOP, 0);

	m_BackButton.Create(NULL, WS_CHILD | BS_OWNERDRAW | WS_TABSTOP, CRect(0, 0, 0, 0), this, IdBackButton);
	m_BackButton.LoadStdImageDPI(m_DittoWindow.m_dpi.GetDPI(), return_16, return_20, return_24, return_28, return_32, _T("PNG"));
	m_BackButton.ModifyStyle(WS_TABSTOP, 0);
	m_BackButton.ShowWindow(SW_SHOW);

	m_systemMenu.Create(NULL, WS_CHILD | BS_OWNERDRAW | WS_TABSTOP, CRect(0, 0, 0, 0), this, IdSystemButton);
	m_systemMenu.LoadStdImageDPI(m_DittoWindow.m_dpi.GetDPI(), system_menu_2_24, system_menu_2_30, system_menu_2_36, system_menu_2_42, system_menu_2_48, _T("PNG"), system_menu_54, system_menu_60, system_menu_66, system_menu_72, system_menu_78, system_menu_84);
	m_systemMenu.ModifyStyle(WS_TABSTOP, 0);
	m_systemMenu.ShowWindow(SW_SHOW);

	m_stGroup.Create(_T(""), WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IdGroupText);

	//Set the z-order
	m_lstHeader.SetWindowPos(this, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	m_search.SetWindowPos(&m_lstHeader, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	m_ShowGroupsFolderBottom.SetWindowPos(&m_search, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

	//LVS_EX_FLATSB
	m_lstHeader.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);

	// Create the columns
	if (m_lstHeader.InsertColumn(0, _T(""), LVCFMT_LEFT, 2500, 0) != 0)
	{
		ASSERT(FALSE);
		return -1;
	}

	m_Alpha.SetWindowHandle(m_hWnd);

	CString onTopMsg = theApp.m_Language.GetString(_T("TurnOfAlwaysOntop"), _T("Always on Top Enabled"));
	CString shortcutText = m_actions.GetCmdKeyText(ActionEnums::TOGGLESHOWPERSISTANT);
	if (shortcutText != _T("") &&
		shortcutText.Find("\t" + shortcutText) < 0)
	{
		onTopMsg += "\t(";
		onTopMsg += shortcutText;
		onTopMsg += ")";
	}

	m_alwaysOnToWarningStatic.Create(onTopMsg, WS_CHILD | SS_CENTERIMAGE | SS_NOTIFY, CRect(0, 0, 0, 0), this, IdOnTopWarning);
	m_alwaysOnToWarningStatic.SetBkColor(COLORREF(RGB(255, 255, 0)));
	m_alwaysOnToWarningStatic.SetTextColor(COLORREF(RGB(0, 0, 255)));
	m_alwaysOnToWarningStatic.SetToggleCursor(true);
	m_alwaysOnToWarningStatic.SetFont(&m_groupFont);

	m_noSearchResultsStatic.Create(onTopMsg, WS_CHILD, CRect(0, 0, 0, 0), this, IdNoSearchResults);

	m_popupMsg.m_hWndPosRelativeTo = m_hWnd;

	UpdateFont();

	m_thread.Start(this);
	m_extraDataThread.Start(this);

	LoadShortcuts();

	InvalidateNc();

	return 0;
}

void CQPasteWnd::LoadShortcuts()
{
	m_modifierKeyActions.RemoveAll();

	m_modifierKeyActions.m_checkModifierKeys = false;
	m_modifierKeyActions.AddAccel(ActionEnums::MODIFIER_ACTVE_SELECTIONUP, VK_UP);
	m_modifierKeyActions.AddAccel(ActionEnums::MODIFIER_ACTVE_SELECTIONDOWN, VK_DOWN);
	m_modifierKeyActions.AddAccel(ActionEnums::MODIFIER_ACTVE_MOVEFIRST, VK_HOME);
	m_modifierKeyActions.AddAccel(ActionEnums::MODIFIER_ACTVE_MOVELAST, VK_END);

	m_actions.RemoveAll();
	m_toolTipActions.RemoveAll();

	m_actions.AddAccel(ActionEnums::NEXTTABCONTROL, VK_TAB);
	m_actions.AddAccel(ActionEnums::PREVTABCONTROL, CAccels::MakeKey(VK_TAB, HOTKEYF_CONTROL));
	m_actions.AddAccel(ActionEnums::BACKGRROUP, VK_BACK);
	m_actions.AddAccel(ActionEnums::DELETE_SELECTED, VK_DELETE);

	m_actions.AddAccel(ActionEnums::HOMELIST, VK_HOME);
	m_actions.AddAccel(ActionEnums::SHOWMENU, VK_APPS);
	m_actions.AddAccel(ActionEnums::SYSTEM_MENU, CAccels::MakeKey(VK_APPS, HOTKEYF_CONTROL));

	m_search.SetLastSearchAccel(CAccel(0, ActionEnums::APPLY_LAST_SEARCH, 0));

	for (DWORD i = ActionEnums::FIRST_ACTION + 1; i < ActionEnums::LAST_ACTION; i++)
	{
		ActionEnums::ActionEnumValues action = (ActionEnums::ActionEnumValues)i;

		if (ActionEnums::UserConfigurable(action))
		{
			LoadActionShortcuts(action);
		}
	}

	m_actions.AddAccel(ActionEnums::TOGGLEFILELOGGING, CAccels::MakeKey('D', HOTKEYF_CONTROL), CAccels::MakeKey('F', HOTKEYF_CONTROL));
	m_actions.AddAccel(ActionEnums::TOGGLEOUTPUTDEBUGSTRING, CAccels::MakeKey('D', HOTKEYF_CONTROL), CAccels::MakeKey('O', HOTKEYF_CONTROL));

	m_lstHeader.SetTooltipActions(&m_toolTipActions);
}

void CQPasteWnd::LoadActionShortcuts(ActionEnums::ActionEnumValues action)
{
	for (int shortCutIndex = 0; shortCutIndex < 10; shortCutIndex++)
	{
		int a = Settings().GetActionShortCutA(action, shortCutIndex);
		if (a > 0)
		{
			int b = Settings().GetActionShortCutB(action, shortCutIndex);
			AddActionShortcut(action, a, b);
		}
	}
}

void CQPasteWnd::AddActionShortcut(ActionEnums::ActionEnumValues action, int a, int b)
{
	m_actions.AddAccel(action, a, b);

	//always add a shift variation to show description F3 so it will search backwards in the text search
	if (action == ActionEnums::SHOWDESCRIPTION)
	{
		m_actions.AddAccel(action, CAccels::MakeKey(LOBYTE(a), ShiftVariationModifier(a)), b);
	}
	else if (action == ActionEnums::APPLY_LAST_SEARCH)
	{
		m_search.SetLastSearchAccel(CAccel(a, action, b));
	}

	if (ActionEnums::ToolTipAction(action))
	{
		m_toolTipActions.AddAccel(action, a, b);

		if (action == ActionEnums::SHOWDESCRIPTION)
		{
			m_toolTipActions.AddAccel(action, CAccels::MakeKey(LOBYTE(a), ShiftVariationModifier(a)), b);
		}
	}
}

int CQPasteWnd::ShiftVariationModifier(int a)
{
	int shift = HOTKEYF_SHIFT;
	if ((HIBYTE(a) & HOTKEYF_SHIFT))
	{
		shift = 0;
	}

	return shift;
}

void CQPasteWnd::SetSearchImages()
{
	//int iSourceImageDPIToUse = 96; // We will assume 96 by default.

	//if (m_DittoWindow.m_dpi.GetDPI() > 144) 
	//	iSourceImageDPIToUse = 192;
	//else if (m_DittoWindow.m_dpi.GetDPI() > 120) 
	//	iSourceImageDPIToUse = 144;
	//else if (m_DittoWindow.m_dpi.GetDPI() > 96) 
	//	iSourceImageDPIToUse = 120;

	//// Now select the right resource to load.
	//switch(iSourceImageDPIToUse)
	//{
	//case 120: 
	//	m_search.SetBitmaps(IDB_BITMAP_SEARCH_NORMAL_125, IDB_BITMAP_SEARCH_CLOSE_125);
	//	break;
	//case 144: 
	//	m_search.SetBitmaps(IDB_BITMAP_SEARCH_NORMAL_150, IDB_BITMAP_SEARCH_CLOSE_150);
	//	break;
	//case 192: 
	//	m_search.SetBitmaps(IDB_BITMAP_SEARCH_NORMAL_200, IDB_BITMAP_SEARCH_CLOSE_200);
	//	break;						
	//default: // default to 96 DPI
	//	m_search.SetBitmaps(IDB_BITMAP_SEARCH_NORMAL, IDB_BITMAP_SEARCH_CLOSE);
	//	break;
	//}
}

void CQPasteWnd::OnSize(UINT nType, int cx, int cy)
{
	CWndEx::OnSize(nType, cx, cy);

	if (!IsWindow(m_lstHeader.m_hWnd))
	{
		return;
	}

	m_popupMsg.Hide();

	MoveControls();
}

void CQPasteWnd::MoveControls()
{
	CRect crRect;
	GetClientRect(crRect);
	int cx = crRect.Width();
	int cy = crRect.Height();

	//Hide the two pixels of space at the top, not sure where this is coming from
	int topOfListBox = MoveGroupHeader(cx);

	int searchRowStart = 33;

	/*if(Settings().m_bShowPersistent)
	{
		searchRowStart = 41;
	}*/

	int listBoxBottomOffset = m_DittoWindow.m_dpi.Scale(searchRowStart);

	int extraSize = 0;

	// Hide native scrollbar if using modern scrollbar OR if scrollbar is set to not always show
	bool hideNativeScrollbar = Settings().m_useModernScrollBar || 
		(m_showScrollBars == false && Settings().m_showScrollBar == false);

	if (hideNativeScrollbar)
	{
		extraSize = m_DittoWindow.m_dpi.Scale(::GetSystemMetrics(SM_CXVSCROLL));

		CRgn rgnRect;
		CRect r;
		m_lstHeader.GetWindowRect(&r);

		rgnRect.CreateRectRgn(0, 0, cx, (cy - listBoxBottomOffset - topOfListBox) );

		m_lstHeader.SetWindowRgn(rgnRect, TRUE);
	}
	else
	{
		// Clear region to show native scrollbar
		m_lstHeader.SetWindowRgn(NULL, TRUE);
	}

	if (m_noSearchResults &&
		(m_strSearch != _T("") || m_bShowStarredClips))
	{
		m_lstHeader.ShowWindow(SW_HIDE);
		m_noSearchResultsStatic.ShowWindow(SW_SHOW);
		m_modernScrollBar.ShowWindow(SW_HIDE);
		m_modernScrollBarHorz.ShowWindow(SW_HIDE);

		auto border = m_DittoWindow.m_dpi.Scale(10);
		m_noSearchResultsStatic.MoveWindow(border, topOfListBox + border, cx - border, cy - listBoxBottomOffset - topOfListBox + 1 - border);
	}
	else
	{
		m_lstHeader.ShowWindow(SW_SHOW);
		m_noSearchResultsStatic.ShowWindow(SW_HIDE);

		m_lstHeader.MoveWindow(0, topOfListBox, cx + extraSize, cy - listBoxBottomOffset - topOfListBox + extraSize + 1);

		UpdateModernScrollBars();
	}
	m_search.MoveWindow(m_DittoWindow.m_dpi.Scale(34), cy - m_DittoWindow.m_dpi.Scale(searchRowStart - 5), cx - m_DittoWindow.m_dpi.Scale(70), m_DittoWindow.m_dpi.Scale(25));

	m_systemMenu.MoveWindow(cx - m_DittoWindow.m_dpi.Scale(30), cy - m_DittoWindow.m_dpi.Scale(28), m_DittoWindow.m_dpi.Scale(24), m_DittoWindow.m_dpi.Scale(24));

	m_ShowGroupsFolderBottom.MoveWindow(m_DittoWindow.m_dpi.Scale(4), cy - m_DittoWindow.m_dpi.Scale(28), m_DittoWindow.m_dpi.Scale(24), m_DittoWindow.m_dpi.Scale(24));

	/*if (Settings().m_bShowPersistent &&
		Settings().m_bShowAlwaysOnTopWarning)
	{
		m_alwaysOnToWarningStatic.ShowWindow(SW_SHOW);
		m_alwaysOnToWarningStatic.MoveWindow(m_DittoWindow.m_dpi.Scale(2), cy - m_DittoWindow.m_dpi.Scale(18), cx - m_DittoWindow.m_dpi.Scale(4), m_DittoWindow.m_dpi.Scale(17));
	}
	else*/
	{
		m_alwaysOnToWarningStatic.ShowWindow(SW_HIDE);
	}
}

int CQPasteWnd::MoveGroupHeader(int cx)
{
	int topOfListBox = 0;

	if (theApp.m_GroupID > 0 && m_bShowStarredClips == false)
	{
		m_stGroup.ShowWindow(SW_SHOW);
		m_BackButton.ShowWindow(SW_SHOW);

		m_BackButton.MoveWindow(m_DittoWindow.m_dpi.Scale(2), m_DittoWindow.m_dpi.Scale(2), m_DittoWindow.m_dpi.Scale(16), m_DittoWindow.m_dpi.Scale(16));
		m_stGroup.MoveWindow(m_DittoWindow.m_dpi.Scale(24), m_DittoWindow.m_dpi.Scale(2), cx - m_DittoWindow.m_dpi.Scale(20), m_DittoWindow.m_dpi.Scale(16));

		topOfListBox = m_DittoWindow.m_dpi.Scale(20);
	}
	else
	{
		m_BackButton.ShowWindow(SW_HIDE);
		m_stGroup.ShowWindow(SW_HIDE);
	}

	return topOfListBox;
}

void CQPasteWnd::UpdateModernScrollBars()
{
	// Update modern scrollbar position and visibility (only if enabled)
	if (Settings().m_useModernScrollBar)
	{
		if (Settings().m_showScrollBar)
		{
			m_modernScrollBar.UpdateScrollBar();
			m_modernScrollBar.Show(false);
			m_modernScrollBarHorz.UpdateScrollBar();
			m_modernScrollBarHorz.Show(false);
		}
		else
		{
			m_modernScrollBar.UpdateScrollBar();
			m_modernScrollBar.Hide(false);
			m_modernScrollBarHorz.UpdateScrollBar();
			m_modernScrollBarHorz.Hide(false);
		}
	}
	else
	{
		m_modernScrollBar.Hide(false);
		m_modernScrollBarHorz.Hide(false);
	}
}

void CQPasteWnd::OnSetFocus(CWnd* pOldWnd)
{
	CWndEx::OnSetFocus(pOldWnd);

	if (::IsWindow(m_lstHeader.m_hWnd))
	{
		m_lstHeader.SetFocus();
	}
}

void CQPasteWnd::OnKillFocus(CWnd* pOldWnd)
{
	CWndEx::OnKillFocus(pOldWnd);
}

void CQPasteWnd::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CWndEx::OnActivate(nState, pWndOther, bMinimized);

	if (m_bHideWnd == false || IsListToolTipWnd(pWndOther))
	{
		return;
	}

	CLogger::Log(CStringUtil::Format(_T("CQPasteWnd::OnActivate, nState: %d, Other: %d, Minimized: %d"), nState, pWndOther, bMinimized));

	if (nState == WA_INACTIVE)
	{
		OnDeactivateWindow();
	}
	else if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE)
	{
		OnActivateWindow(bMinimized);
	}
}

bool CQPasteWnd::IsListToolTipWnd(CWnd* pWndOther)
{
	return (m_lstHeader.GetToolTipHWnd() != NULL && m_lstHeader.GetToolTipHWnd() == pWndOther->GetSafeHwnd());
}

void CQPasteWnd::OnDeactivateWindow()
{
	SaveWindowSize();

	m_bModifersMoveActive = false;

	if (!Settings().m_bShowPersistent && !Settings().m_bDoNotHideOnDeactivate)
	{
		HideQPasteWindow(false);
	}
	else if (Settings().GetAutoHide())
	{
		MinMaxWindow(CDittoWindow::ForceMin);
	}

	//re register the global hot keys for the last ten
	if (theApp.m_bAppExiting == false)
	{
		g_HotKeys.RegisterAll();
	}

	m_lstHeader.HidePopup(true);
}

void CQPasteWnd::OnActivateWindow(BOOL bMinimized)
{
	if (bMinimized == FALSE)
	{
		if (theApp.m_bShowingQuickPaste == false)
		{
			BOOL fillList = NeedsFillListOnActivate();

			ShowQPasteWindow(fillList);
		}

		//Unregister the global hot keys for the last ten copies
		g_HotKeys.UnregisterAll(false, true);
	}
}

BOOL CQPasteWnd::NeedsFillListOnActivate()
{
	BOOL fillList = FALSE;
	if (m_listItems.size() == 0)
	{
		fillList = TRUE;
	}
	else if (theApp.m_databaseOnNetworkShare)
	{
		__int64 lastWrite = CFileSystem::GetLastWriteTime(Settings().GetDBPath());
		if (lastWrite > m_lastDbWrite)
		{
			m_lastDbWrite = lastWrite;
			fillList = TRUE;
		}
	}

	return fillList;
}

BOOL CQPasteWnd::HideQPasteWindow(bool releaseFocus, BOOL clearSearchData)
{
	if (clearSearchData == -1)
	{
		clearSearchData = DefaultClearSearchData();
	}

	CLogger::Log(_T("Start of HideQPasteWindow"));
	ULONGLONG startTick = GetTickCount64();

	if (!theApp.m_bShowingQuickPaste)
	{
		CLogger::Log(_T("End of HideQPasteWindow, !theApp.m_bShowingQuickPaste"));
	}

	{
		ATL::CCritSecLock csLock(m_CritSection.m_sect);

		m_bStopQuery = true;
	}

	theApp.m_bShowingQuickPaste = false;

	//needs to be before we hide our window - inorder to set focus to another window we need to be the foreground window
	//http://msdn.microsoft.com/en-us/library/windows/desktop/ms632668%28v=vs.85%29.aspx
	if (releaseFocus)
	{
		theApp.m_activeWnd.ReleaseFocus();
	}

	KillTimer(TimerFillCache);

	m_lstHeader.HidePopup(true);

	m_popupMsg.Hide();

	//Save the size
	SaveWindowSize();

	HideOrMinimizeWindow();

	if (clearSearchData == TRUE)
	{
		ClearSearchOnHide();
	}

	RestoreGroupOnHide();

	m_pendingRefresh = false;

	ULONGLONG endTick = GetTickCount64();
	if ((endTick - startTick) > 150)
		CLogger::Log(CStringUtil::Format(_T("Paste Timing HideQPasteWindow: %llu"), endTick - startTick));

	CLogger::Log(CStringUtil::Format(_T("End of HideQPasteWindow, ItemCount: %d"), m_listItems.size()));

	return TRUE;
}

BOOL CQPasteWnd::DefaultClearSearchData()
{
	if ((Settings().m_maintainSearchView || Settings().m_refreshViewAfterPasting == false) &&
		m_strSearch != _T(""))
	{
		CLogger::Log(_T("Currently searching for something and setting to maintain search view is enabled, not refreshing"));
		return FALSE;
	}

	return TRUE;
}

void CQPasteWnd::HideOrMinimizeWindow()
{
	if (Settings().GetShowInTaskBar() && !Settings().GetHideTaskbarIconOnClose())
	{
		ShowWindow(SW_MINIMIZE);
	}
	else
	{
		ShowWindow(SW_HIDE);
	}
}

void CQPasteWnd::ClearSearchOnHide()
{
	//Reset the selection in the search combo
	m_bHandleSearchTextChange = false;
	m_search.SetWindowText(_T(""));
	m_bHandleSearchTextChange = true;
	m_bShowStarredClips = false;

	if (m_strSQLSearch.IsEmpty() == FALSE || m_pendingRefresh)
	{
		{
			ATL::CCritSecLock csLock(m_CritSection.m_sect);

			m_bStopQuery = true;
		}

		//Wait for the thread to stop fill the cache so we can clear it
		WaitForSingleObject(m_thread.m_SearchingEvent, 5000);

		{
			ATL::CCritSecLock csLock(m_CritSection.m_sect);

			m_listItems.clear();
			m_lstHeader.SetItemCountEx(0);
		}
	}
}

void CQPasteWnd::RestoreGroupOnHide()
{
	if (theApp.m_GroupID > 0 &&
		Settings().GetRevertToTopLevelGroup())
	{
		theApp.EnterGroupID(-1);
	}
	else
	{
		theApp.TryEnterOldGroupState();
	}
}

void CQPasteWnd::SaveWindowSize()
{
	if (this->IsIconic() == FALSE)
	{
		CRect rect;
		GetWindowRectEx(&rect);
		CSize s = rect.Size();
		Settings().SetQuickPasteSize(CSize(m_DittoWindow.m_dpi.UnScale(s.cx), m_DittoWindow.m_dpi.UnScale(s.cy)));
		Settings().SetQuickPastePoint(rect.TopLeft());
	}
}

BOOL CQPasteWnd::ShowQPasteWindow(BOOL bFillList)
{
	theApp.m_bShowingQuickPaste = true;

	CLogger::Log(CStringUtil::Format(_T("Start - ShowQPasteWindow - Fill List: %d, array count: %d"), bFillList, m_listItems.size()));

	//Ensure we have the latest theme file, this checks the last write time so it doesn't read the file each time
	Settings().m_Theme.Load(Settings(), Settings().GetTheme(), false, true);

	SetCaptionColorActive(Settings().m_bShowPersistent, theApp.GetConnectCV());
	SetCaptionOn(Settings().GetCaptionPos(), true, Settings().m_Theme.GetCaptionSize(), Settings().m_Theme.GetCaptionFontSize());

	UpdateStatus();

	m_bHideWnd = true;

	SetCurrentTransparency();

	m_lstHeader.SetNumberOfLinesPerRow(Settings().GetLinesPerRow(), false);
	m_lstHeader.SetShowTextForFirstTenHotKeys(Settings().GetShowTextForFirstTenHotKeys());
	m_lstHeader.SetShowIfClipWasPasted(Settings().GetShowIfClipWasPasted());

	if (bFillList)
	{
		FillList();
	}
	else
	{
		MoveControls();
	}

	// always on top... for persistent showing (Settings().m_bShowPersistent)
	// SHOWWINDOW was also integrated into this function rather than calling it separately
	if (Settings().GetShowPersistent())
	{
		::SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
	}

	//SetKeyModiferState(true);

	CLogger::Log(CStringUtil::Format(_T("END - ShowQPasteWindow - Fill List: %d, array count: %d"), bFillList, m_listItems.size()));

	return TRUE;
}

bool CQPasteWnd::Add(const CString& csHeader, const CString& /*csText*/, int nID)
{
	int nNewIndex;

	if ((nNewIndex = m_lstHeader.InsertItem(m_lstHeader.GetItemCount(), csHeader)) == -1)
	{
		return false;
	}

	m_lstHeader.SetItemData(nNewIndex, nID);

	return true;
}

BOOL CQPasteWnd::OpenID(int id, CSpecialPasteOptions pasteOptions)
{
	CLogger::Log(CStringUtil::Format(_T("Start OpenId, Id: %d, Only CF_TEXT: %s"), id, pasteOptions.ToString().GetString()));

	if (pasteOptions.m_pPasteFormats == NULL)
	{
		if (theApp.EnterGroupID(id, FALSE, FALSE))
		{
			CLogger::Log(_T("Entered group"));
			return TRUE;
		}
	}

	// else, it is a clip, so paste it
	CProcessPaste paste(Settings());

	paste.m_bSendPaste = Settings().m_bSendPasteMessageAfterSelection == TRUE ? true : false;
	paste.m_pasteOptions = pasteOptions;
	paste.m_pastedFromGroup = (theApp.m_GroupID > 0);

	paste.GetClipIDs().Add(id);

	if (paste.DoPaste())
	{
		theApp.OnPasteCompleted();

		if (Settings().m_bSendPasteMessageAfterSelection == FALSE)
		{
			theApp.m_activeWnd.ActivateTarget();
		}

		if (Settings().m_bShowPersistent && Settings().GetAutoHide())
		{
			MinMaxWindow(CDittoWindow::ForceMin);
		}
	}
	else
	{
		CString errorMessage;
		errorMessage.Format(_T("Paste Error - %s"), paste.m_lastErrorMessage.GetString());
		m_popupMsg.Show(errorMessage, CPoint(0, 0), true);
		SetTimer(TimerErrorMsg, Settings().GetErrorMsgPopupTimeout(), NULL);
	}

	CLogger::Log(CStringUtil::Format(_T("End OpenId, Id: %d, Only CF_TEXT: %s"), id, pasteOptions.ToString().GetString()));

	return TRUE;
}

BOOL CQPasteWnd::OpenSelection(CSpecialPasteOptions pasteOptions)
{
	CLogger::Log(_T("Start Open Selection"));
	ARRAY IDs;
	m_lstHeader.GetSelectionItemData(IDs);

	INT_PTR count = IDs.GetSize();

	if (count <= 0)
	{
		return FALSE;
	}

	if (count == 1)
	{
		return OpenID(IDs[0], pasteOptions);
	}

	CProcessPaste paste(Settings());

	paste.m_bSendPaste = Settings().m_bSendPasteMessageAfterSelection == TRUE ? true : false;
	paste.m_pasteOptions = pasteOptions;
	paste.m_pastedFromGroup = (theApp.m_GroupID > 0);


	paste.GetClipIDs().Copy(IDs);
	if (paste.DoPaste())
	{
		theApp.OnPasteCompleted();

		if (Settings().m_bSendPasteMessageAfterSelection == FALSE)
		{
			theApp.m_activeWnd.ActivateTarget();
		}

		if (Settings().m_bShowPersistent && Settings().GetAutoHide())
		{
			MinMaxWindow(CDittoWindow::ForceMin);
		}
	}
	else
	{
		CString errorMessage;
		errorMessage.Format(_T("Paste Error - %s"), paste.m_lastErrorMessage.GetString());
		m_popupMsg.Show(errorMessage, CPoint(0, 0), true);
		SetTimer(TimerErrorMsg, Settings().GetErrorMsgPopupTimeout(), NULL);
	}

	CLogger::Log(_T("End Open Selection"));
	return TRUE;
}

BOOL CQPasteWnd::OpenIndex(int item, bool plainTextOnly)
{
	if (item >= m_lstHeader.GetItemCount())
	{
		return FALSE;
	}

	CSpecialPasteOptions pasteOptions;
	pasteOptions.m_pasteAsPlainText = plainTextOnly;
	return OpenID(m_lstHeader.GetItemData(item), pasteOptions);
}

BOOL CQPasteWnd::NewGroup(bool bGroupSelection, int parentId)
{
	CGroupName Name;
	CString csName("");

	if (Settings().m_bPrompForNewGroupName)
	{
		m_bHideWnd = false;

		CDimWnd dimmer(this);

		INT_PTR nRet = Name.DoModal();

		m_bHideWnd = true;

		if (nRet == IDOK)
		{
			csName = Name.m_csName;
		}
		else
		{
			return false;
		}
	}

	int id = CClipDatabase::NewGroupID(parentId, csName);

	if (id <= 0)
	{
		return FALSE;
	}

	if (!bGroupSelection)
	{
		theApp.m_FocusID = id; // focus on the new group
		FillList();
		return TRUE;
	}

	CClipIDs IDs;
	m_lstHeader.GetSelectionItemData(IDs);
	IDs.MoveTo(id);
	theApp.EnterGroupID(id);
	return TRUE;
}

LRESULT CQPasteWnd::OnListSelect_DB_ID(WPARAM wParam, LPARAM /*lParam*/)
{
	CSpecialPasteOptions pasteOptions;
	OpenID((int)wParam, pasteOptions);
	return TRUE;
}

LRESULT CQPasteWnd::OnListMoveSelectionToGroup(WPARAM wParam, LPARAM /*lParam*/)
{
	int groupId = (int)wParam;
	if (groupId >= -1)
	{
		CClipIDs IDs;
		m_lstHeader.GetSelectionItemData(IDs);

		IDs.MoveTo(groupId);
	}
	return TRUE;
}

LRESULT CQPasteWnd::OnCopyClip(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	DoCopySelection();
	return TRUE;
}

LRESULT CQPasteWnd::OnSearchEnterKeyPressed(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	CString csText;
	m_search.GetWindowText(csText);

	MSG msg;
	msg.lParam = 0;
	msg.wParam = VK_RETURN;
	msg.message = WM_KEYDOWN;
	if (CheckActions(&msg) == false)
	{
	}
	return TRUE;
}

LRESULT CQPasteWnd::OnListEnd(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	return 0;
}

LRESULT CQPasteWnd::OnReloadClipInUI(WPARAM wParam, LPARAM lParam)
{
	BOOL foundClip = FALSE;
	int clipId = (int)wParam;
	int updateFlags = (int)lParam;

	if (Settings().m_maintainSearchView &&
		m_strSearch != _T("") &&
		updateFlags & CClipRefreshFlags::AfterPasteSelectClip)
	{
		CLogger::Log(_T("Currently searching for something and setting to maintain search view is enabled, not refreshing clip order"));
		return FALSE;
	}

	ULONGLONG startTick = GetTickCount64();

	theApp.m_FocusID = -1;

	CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT clipOrder, clipGroupOrder, lastPasteDate, mText FROM Main WHERE lID = %d"), clipId);
	if (q.eof() == false)
	{
		ReloadedClip reloaded{};
		reloaded.order = q.getFloatField(_T("clipOrder"));
		reloaded.orderGroup = q.getFloatField(_T("clipGroupOrder"));
		reloaded.lastPasted = q.getInt64Field(_T("lastPasteDate"));
		reloaded.description = q.getStringField(_T("mText"));

		std::vector<CMainTable>::iterator iter = m_listItems.begin();
		while (iter != m_listItems.end())
		{
			if (iter->m_lID == clipId)
			{
				foundClip = ApplyReloadedClip(*iter, reloaded, updateFlags, clipId);

				break;
			}

			iter++;
		}
	}

	ULONGLONG endTick = GetTickCount64();
	if ((endTick - startTick) > 150)
		CLogger::Log(CStringUtil::Format(_T("Paste Timing OnReloadClipInUI: %llu, ClipId: %d"), endTick - startTick, clipId));

	return foundClip;
}

BOOL CQPasteWnd::ApplyReloadedClip(CMainTable &item, const ReloadedClip &reloaded, int updateFlags, int clipId)
{
	BOOL foundClip = FALSE;

	if (updateFlags & CClipRefreshFlags::AfterPasteSelectClip)
	{
		item.m_datePasted = reloaded.lastPasted;

		if (item.m_clipOrder != reloaded.order || item.m_clipGroupOrder != reloaded.orderGroup)
		{
			item.m_clipOrder = reloaded.order;
			item.m_clipGroupOrder = reloaded.orderGroup;

			if (theApp.m_GroupID > 0)
			{
				std::sort(m_listItems.begin(), m_listItems.end(), CMainTable::GroupSortDesc);
			}
			else
			{
				std::sort(m_listItems.begin(), m_listItems.end(), CMainTable::SortDesc);
			}
		}

		foundClip = TRUE;

		m_lstHeader.RefreshVisibleRows();
		m_lstHeader.RedrawWindow();
		SelectFocusID();
	}
	else if (updateFlags & CClipRefreshFlags::ClipDescription)
	{
		item.m_Desc = reloaded.description;

		foundClip = TRUE;

		RemoveFromImageRtfCache(-1, clipId);

		m_lstHeader.RefreshVisibleRows();
		m_lstHeader.RedrawWindow();
	}

	return foundClip;
}

LRESULT CQPasteWnd::OnRefreshView(WPARAM wParam, LPARAM /*lParam*/)
{
	MSG msg;
	// remove all additional refresh view messages from the queue
	while (::PeekMessage(&msg, m_hWnd, CDittoMessage::RefreshView, CDittoMessage::RefreshView, PM_REMOVE)) {}

	if (theApp.m_bShowingQuickPaste)
	{
		CopyReasonEnum::CopyReason copyReason = (CopyReasonEnum::CopyReason)wParam;
		if (copyReason == CopyReasonEnum::COPY_FROM_TOOLTIP)
		{
			m_pendingRefresh = true;
			return FALSE;
		}
	}

	CLogger::Log(_T("OnRefreshView - Start"));
	CString action;

	theApp.m_FocusID = -1;

	m_bHandleSearchTextChange = false;

	m_search.SetWindowText(_T(""));
	m_bHandleSearchTextChange = true;

	if (theApp.m_bShowingQuickPaste)
	{
		FillList(_T(""));
		action = _T("Filled List");
	}
	else
	{
		//Wait for the thread to stop fill the cache so we can clear it
		WaitForSingleObject(m_thread.m_SearchingEvent, 5000);

		{
			ATL::CCritSecLock csLock(m_CritSection.m_sect);
			m_listItems.clear();
		}

		m_lstHeader.SetItemCountEx(0);
		UpdateStatus();

		action = _T("Cleared Items");
	}

	CLogger::Log(CStringUtil::Format(_T("OnRefreshView - End - Count: %d, Action: %s"), m_listItems.size(), action.GetString()));

	return TRUE;
}

void CQPasteWnd::RefreshNc()
{
	if (!theApp.m_bShowingQuickPaste)
	{
		return;
	}

	InvalidateNc();
}

void CQPasteWnd::UpdateStatus(bool /*bRepaintImmediately*/)
{
	CString title = m_Title;

	if (Settings().m_bShowPersistent)
	{
		title = (CStringUtil::Format(_T("%s %s"), s_qpasteTitle, theApp.m_Language.GetString("top_window", "[Always on top]").GetString()));
	}

	if (theApp.IsClipboardViewerConnected() == FALSE)
	{
		title += _T(" ");
		title += theApp.m_Language.GetString("disconnected", "[Disconnected]");
	}

	if (m_bShowStarredClips)
	{
		title += _T(" ");
		title += theApp.m_Language.GetString("starred_clips", "[Starred clips]");
	}

	CString cs;
	cs.Format(_T(" - %d/%d"), m_lstHeader.GetSelectedCount(), m_lstHeader.GetItemCount());
	title += cs;

	if (theApp.m_Status != "")
	{
		title += " [ ";
		title += theApp.m_Status;
		title += " ] - ";
	}
	else
	{
		title += " - ";
	}

	if (::IsWindow(theApp.m_activeWnd.ActiveWnd()))
	{
		title += theApp.m_activeWnd.ActiveWndName();
	}
	else
	{
		title += theApp.m_Language.GetString("No_Target", "No target");
	}

	SetToolTipText(title);

	CString windowTitle = s_qpasteTitle;

	if (Settings().m_bShowPersistent)
	{
		windowTitle += CStringUtil::Format(_T(" %s"), theApp.m_Language.GetString("top_window", "[Always on top]").GetString());
	}

	if (theApp.IsClipboardViewerConnected() == FALSE)
	{
		windowTitle += CStringUtil::Format(_T(" %s"), theApp.m_Language.GetString("disconnected", "[Disconnected]").GetString());
	}

	if (m_bShowStarredClips)
	{
		windowTitle += CStringUtil::Format(_T(" %s"), theApp.m_Language.GetString("starred_clips", "[Starred clips]").GetString());
	}

	SetCustomWindowTitle(windowTitle);
}

BOOL CQPasteWnd::FillList(CString csSQLSearch)
{
	KillTimer(TimerDoSearch);

	m_lstHeader.HidePopup(true);

	CLogger::Log(CStringUtil::Format(_T("Start Fill List - %s"), csSQLSearch.GetString()));

	m_lstHeader.SetSearchText(csSQLSearch);

	{
		ATL::CCritSecLock csLock(m_CritSection.m_sect);
		m_bStopQuery = true;
	}

	CString strStarredFilter = _T("Main.bIsGroup = 0 AND Main.lDontAutoDelete > 0");
	FillListQuery query{};

	SetGroupFilter(query, strStarredFilter);

	CRect crRect;
	GetClientRect(crRect);

	CString csSQL;

	CString sqlSearch = "";

	if (csSQLSearch == "")
	{
		m_strSQLSearch = m_bShowStarredClips ? query.filter : CString();
		m_strSearch = "";
	}
	else
	{
		SetSearchFilter(csSQLSearch, query, strStarredFilter);

		m_strSQLSearch = query.filter;
		m_strSearch = csSQLSearch;
	}

	CString sql;
	CString countSql;

	//Format the count and select sql queries for the thread
	countSql.Format(_T("SELECT COUNT(%s Main.lID) FROM Main %s where %s"), query.isDistinct.GetString(), query.dataJoin.GetString(), query.filter.GetString());

	sql.Format(_T("SELECT %s Main.lID, Main.mText, Main.lParentID, Main.lDontAutoDelete, ")
		_T("Main.lShortCut, Main.bIsGroup, Main.QuickPasteText, Main.clipOrder, Main.clipGroupOrder, ")
		_T("Main.stickyClipOrder, Main.stickyClipGroupOrder, Main.lDate, Main.lastPasteDate FROM Main %s ")
		_T("where %s order by %s"), query.isDistinct.GetString(), query.dataJoin.GetString(), query.filter.GetString(), query.sort.GetString());


	{
		ATL::CCritSecLock csLock(m_CritSection.m_sect);
		m_listItems.clear();
	}

	m_noSearchResults = false;
	m_lstHeader.SetItemCount(0);
	m_lstHeader.RefreshVisibleRows();

	CPoint loadItem(-1, m_lstHeader.GetCountPerPage() + 2);
	m_loadItems.push_back(loadItem);

	m_thread.SetSearchSql(sql, countSql);
	m_thread.FireLoadItems(true);

	MoveControls();

	countSql.Replace(_T("%"), _T("%%"));
	sql.Replace(_T("%"), _T("%%"));
	CLogger::Log(CStringUtil::Format(_T("Start Fill List - Count SQL: %s, Query SQL: %s"), countSql.GetString(), sql.GetString()));

	return TRUE;
}

void CQPasteWnd::SetGroupFilter(FillListQuery &query, const CString &strStarredFilter)
{
	// History Groupiter->m_stickyClipGroupOrder = clip.m_stickyClipGroupOrder;
	if (m_bShowStarredClips)
	{
		query.sort = "Main.stickyClipOrder DESC, "
			"Main.bIsGroup ASC, "
			"Main.clipOrder DESC";

		query.filter = strStarredFilter;
	}
	else if (theApp.m_GroupID < 0)
	{
		//do not change this this directly relates to the views in the Main table
		query.sort = "Main.stickyClipOrder DESC, "
			"Main.bIsGroup ASC, "
			"Main.clipOrder DESC";

		query.filter = MainListFilter();
	}
	else
		// it's some other group
	{
		//do not change this this directly relates to the views in the Main table
		query.sort = "Main.stickyClipGroupOrder DESC, "
			"Main.bIsGroup ASC, "
			"Main.clipGroupOrder DESC";

		//Main.stickyClipGroupOrder DESC, Main.clipGroupOrder DESC";//

		if (theApp.m_GroupID >= 0)
		{
			query.filter.Format(_T("Main.lParentID = %d"), theApp.m_GroupID);
			query.parentFilter = query.filter;
		}

		m_stGroup.SetWindowText(theApp.m_GroupText);
	}
}

CString CQPasteWnd::MainListFilter() const
{
	CString strFilter;

	if (Settings().m_bShowAllClipsInMainList)
	{
		if (Settings().GetShowGroupsInMainList())
		{
			//found to be slower on large databases
			strFilter = "((Main.bIsGroup = 1 AND Main.lParentID = -1) OR Main.bIsGroup = 0)";
		}
		else
		{
			strFilter = "(Main.bIsGroup = 0)";
		}
	}
	else
	{
		strFilter = "((Main.bIsGroup = 1 AND Main.lParentID = -1) OR (Main.bIsGroup = 0 AND Main.lParentID = -1))";
	}

	return strFilter;
}

void CQPasteWnd::SetSearchFilter(CString &csSQLSearch, FillListQuery &query, const CString &strStarredFilter)
{
	CString descriptionSql = SearchDescriptionSql(csSQLSearch);
	CString quickPasteSql = SearchQuickPasteSql(csSQLSearch);
	CString fullTextSql = SearchFullTextSql(csSQLSearch, descriptionSql, quickPasteSql, query);

	query.filter = JoinSearchSql(descriptionSql, quickPasteSql, fullTextSql);

	if (query.parentFilter.IsEmpty() == FALSE)
	{
		query.filter += " AND ";
		query.filter += query.parentFilter;
	}

	if (m_bShowStarredClips)
	{
		query.filter += " AND (";
		query.filter += strStarredFilter;
		query.filter += ")";
	}
}

CString CQPasteWnd::SearchDescriptionSql(const CString &csSQLSearch) const
{
	CString descriptionSql;

	//If other are off then always search the description
	if (Settings().GetSearchDescription() ||
		(Settings().GetSearchFullText() == FALSE && Settings().GetSearchQuickPaste() == FALSE))
	{
		CFormatSQL descriptionFormat(Settings());
		descriptionFormat.SetVariable("Main.mText");

		descriptionFormat.Parse(csSQLSearch);
		descriptionSql = descriptionFormat.GetSQLString();
	}

	return descriptionSql;
}

CString CQPasteWnd::SearchQuickPasteSql(CString &csSQLSearch) const
{
	CString quickPasteSql;

	if (csSQLSearch.Left(3) == _T("/q ") ||
		csSQLSearch.Left(3) == _T("\\q ") ||
		Settings().GetSearchQuickPaste())
	{
		CFormatSQL quickPasteFormat(Settings());
		quickPasteFormat.SetVariable("Main.QuickPasteText");

		if (csSQLSearch.Left(3) == _T("/q ") ||
			csSQLSearch.Left(3) == _T("\\q "))
		{
			csSQLSearch = csSQLSearch.Mid(3);
		}

		quickPasteFormat.Parse(csSQLSearch);
		quickPasteSql = quickPasteFormat.GetSQLString();
	}

	return quickPasteSql;
}

CString CQPasteWnd::SearchFullTextSql(CString &csSQLSearch, const CString &descriptionSql, const CString &quickPasteSql, FillListQuery &query) const
{
	CString fullTextSql;

	if (csSQLSearch.Left(3) == _T("/f ") ||
		csSQLSearch.Left(3) == _T("\\f ") ||
		Settings().GetSearchFullText())
	{
		query.dataJoin = _T("INNER JOIN Data on Data.lParentID = Main.lID");

		if (csSQLSearch.Left(3) == _T("/f ") ||
			csSQLSearch.Left(3) == _T("\\f "))
		{
			csSQLSearch = csSQLSearch.Mid(3);
		}

		CFormatSQL fullTextFormat(Settings());
		fullTextFormat.SetVariable("Data.ooData");
		fullTextFormat.Parse(csSQLSearch);
		fullTextSql = fullTextFormat.GetSQLString();

		fullTextSql.Insert(1, _T("Data.strClipBoardFormat = 'CF_UNICODETEXT' AND "));

		//If we are also search for other text make sure we only get one entry, including the data rows will cause multiple rows to be returned
		if (descriptionSql != _T(""))
		{
			query.isDistinct = _T("DISTINCT");
		}

		if (quickPasteSql != _T(""))
		{
			query.isDistinct = _T("DISTINCT");
		}
	}

	return fullTextSql;
}

CString CQPasteWnd::JoinSearchSql(const CString &descriptionSql, const CString &quickPasteSql, const CString &fullTextSql)
{
	CString strFilter = _T("(");

	if (descriptionSql != _T(""))
	{
		strFilter += descriptionSql;
	}

	if (quickPasteSql != _T(""))
	{
		if (descriptionSql != _T(""))
		{
			strFilter += _T(" OR ");
		}

		strFilter += quickPasteSql;
	}

	if (fullTextSql != _T(""))
	{
		if (descriptionSql != _T("") ||
			quickPasteSql != _T(""))
		{
			strFilter += _T(" OR ");
		}

		strFilter += fullTextSql;
	}

	strFilter += _T(")");

	return strFilter;
}

void CQPasteWnd::ShowRightClickMenu()
{
	POINT pp;
	CMenu cmPopUp;
	CMenu* cmSubMenu = NULL;

	GetCursorPos(&pp);
	if (cmPopUp.LoadMenu(IDR_QUICK_PASTE) != 0)
	{
		cmSubMenu = cmPopUp.GetSubMenu(0);
		if (!cmSubMenu)
		{
			return;
		}

		int nItem = m_lstHeader.GetCaret();

		CRect rc;
		m_lstHeader.GetItemRect(nItem, rc, LVIR_BOUNDS);
		ClientToScreen(rc);

		if (rc.PtInRect(pp) == FALSE)
		{
			pp.x = rc.left;
			pp.y = rc.bottom;
		}

		theApp.m_Addins.AddPrePasteAddinsToMenu(cmSubMenu);

		AddShowStarredClipsMenuItem(cmSubMenu);

		theApp.m_Language.UpdateRightClickMenu(cmSubMenu);

		if (m_bShowStarredClips)
		{
			cmSubMenu->CheckMenuItem(ID_MENU_SHOWSTARREDCLIPS, MF_CHECKED);
		}

		cmSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, pp.x, pp.y, this, NULL);
	}
}

void CQPasteWnd::AddShowStarredClipsMenuItem(CMenu* pMenu)
{
	if (pMenu == NULL ||
		pMenu->GetMenuState(ID_MENU_SHOWSTARREDCLIPS, MF_BYCOMMAND) != 0xFFFFFFFF)
	{
		return;
	}

	CString csText = theApp.m_Language.GetString(_T("ShowStarredClips"), _T("Show Starred Clips"));
	CString shortcutText = m_actions.GetCmdKeyText(ActionEnums::SHOW_STARRED_CLIPS);
	if (shortcutText != _T("") &&
		csText.Find(_T("\t")) < 0)
	{
		csText += _T("\t");
		csText += shortcutText;
	}

	CString csFilterOn(_T("Filter On Selected Clip"));
	int nPos = -1;
	CMenu* pParentMenu = CMultiLanguage::GetMenuPos(pMenu, csFilterOn, nPos);
	if (pParentMenu != NULL &&
		nPos >= 0)
	{
		pParentMenu->InsertMenu(nPos + 1, MF_BYPOSITION | MF_STRING, ID_MENU_SHOWSTARREDCLIPS, csText);
		return;
	}

	CString csSearchQuickPaste(_T("Search Quick Paste"));
	nPos = -1;
	pParentMenu = CMultiLanguage::GetMenuPos(pMenu, csSearchQuickPaste, nPos);
	if (pParentMenu != NULL &&
		nPos >= 0)
	{
		pParentMenu->InsertMenu(nPos + 1, MF_BYPOSITION | MF_STRING, ID_MENU_SHOWSTARREDCLIPS, csText);
	}
}

const std::array<CQPasteWnd::MenuValueCheck, 8> CQPasteWnd::s_transparencyMenuChecks{ {
	{ 5, ID_MENU_TRANSPARENCY_5 },
	{ 10, ID_MENU_TRANSPARENCY_10 },
	{ 15, ID_MENU_TRANSPARENCY_15 },
	{ 20, ID_MENU_TRANSPARENCY_20 },
	{ 25, ID_MENU_TRANSPARENCY_25 },
	{ 30, ID_MENU_TRANSPARENCY_30 },
	{ 35, ID_TRANSPARENCY_35 },
	{ 40, ID_MENU_TRANSPARENCY_40 },
} };

const std::array<CQPasteWnd::MenuValueCheck, 5> CQPasteWnd::s_linesPerRowMenuChecks{ {
	{ 1, ID_MENU_LINESPERROW_1 },
	{ 2, ID_MENU_LINESPERROW_2 },
	{ 3, ID_MENU_LINESPERROW_3 },
	{ 4, ID_MENU_LINESPERROW_4 },
	{ 5, ID_MENU_LINESPERROW_5 },
} };

const std::array<CQPasteWnd::MenuValueCheck, 3> CQPasteWnd::s_positionMenuChecks{ {
	{ CGetSetOptions::PosAtCaret, ID_MENU_POSITIONING_ATCARET },
	{ CGetSetOptions::PosAtCursor, ID_MENU_POSITIONING_ATCURSOR },
	{ CGetSetOptions::PosAtPrevious, ID_MENU_POSITIONING_ATPREVIOUSPOSITION },
} };

const std::array<CQPasteWnd::MenuValueCheck, 4> CQPasteWnd::s_captionPosMenuChecks{ {
	{ 1, ID_VIEWCAPTIONBARON_RIGHT },
	{ 2, ID_VIEWCAPTIONBARON_BOTTOM },
	{ 3, ID_VIEWCAPTIONBARON_LEFT },
	{ 4, ID_VIEWCAPTIONBARON_TOP },
} };

const std::array<CQPasteWnd::MenuValueCheck, 3> CQPasteWnd::s_doubleClickCaptionMenuChecks{ {
	{ CGetSetOptions::TogglesAlwaysOnTop, ID_MENU_QUICKOPTIONS_DOUBLECLICKINGONCAPTION_TOGGLESALWAYSONTOP },
	{ CGetSetOptions::TogglesAlwaysShowDescription, ID_MENU_QUICKOPTIONS_DOUBLECLICKINGONCAPTION_TOGGLESALWAYSSHOWDESCRIPTION },
	{ CGetSetOptions::RollsUpWindow, ID_MENU_QUICKOPTIONS_DOUBLECLICKINGONCAPTION_ROLLUPWINDOW },
} };

void CQPasteWnd::SetMenuChecks(CMenu* pMenu)
{
	//Set the transparency Check
	if (!Settings().GetEnableTransparency())
	{
		pMenu->CheckMenuItem(ID_MENU_TRANSPARENCY_NONE, MF_CHECKED);
	}
	else
	{
		CheckMenuItemForValue(pMenu, s_transparencyMenuChecks, Settings().GetTransparencyPercent());
	}

	//Set the lines per row check
	CheckMenuItemForValue(pMenu, s_linesPerRowMenuChecks, Settings().GetLinesPerRow());

	//Set the position check
	CheckMenuItemForValue(pMenu, s_positionMenuChecks, Settings().GetQuickPastePosition());

	theApp.UpdateMenuConnectCV(pMenu, ID_MENU_TOGGLECONNECTCV);

	CheckMenuItemIf(pMenu, Settings().GetShowTextForFirstTenHotKeys(), ID_MENU_FIRSTTENHOTKEYS_SHOWHOTKEYTEXT);
	CheckMenuItemIf(pMenu, Settings().GetUseCtrlNumForFirstTenHotKeys(), ID_MENU_FIRSTTENHOTKEYS_USECTRLNUM);
	CheckMenuItemIf(pMenu, Settings().m_bShowPersistent, ID_MENU_ALLWAYSONTOP);
	CheckMenuItemIf(pMenu, Settings().GetAutoHide(), ID_MENU_AUTOHIDE);

	CheckMenuItemForValue(pMenu, s_captionPosMenuChecks, Settings().GetCaptionPos());

	CheckMenuItemIf(pMenu, Settings().GetAllwaysShowDescription(), ID_MENU_QUICKOPTIONS_ALLWAYSSHOWDESCRIPTION);

	CheckMenuItemForValue(pMenu, s_doubleClickCaptionMenuChecks, Settings().GetDoubleClickingOnCaptionDoes());

	CheckMenuItemIf(pMenu, Settings().m_bPrompForNewGroupName, ID_MENU_QUICKOPTIONS_PROMPTFORNEWGROUPNAMES);
	CheckMenuItemIf(pMenu, Settings().m_bDrawThumbnail, ID_MENU_QUICKOPTIONS_SHOWTHUMBNAILS);
	CheckMenuItemIf(pMenu, Settings().m_bDrawRTF, ID_MENU_QUICKOPTIONS_DRAWRTFTEXT);
	CheckMenuItemIf(pMenu, Settings().m_bSendPasteMessageAfterSelection, ID_MENU_QUICKOPTIONS_PASTECLIPAFTERSELECTION);
	CheckMenuItemIf(pMenu, Settings().m_bFindAsYouType, ID_MENU_QUICKOPTIONS_FINDASYOUTYPE);
	CheckMenuItemIf(pMenu, Settings().m_bEnsureEntireWindowCanBeSeen, ID_MENU_QUICKOPTIONS_ENSUREENTIREWINDOWISVISIBLE);
	CheckMenuItemIf(pMenu, Settings().m_bShowAllClipsInMainList, ID_MENU_QUICKOPTIONS_SHOWCLIPSTHATAREINGROUPSINMAINLIST);
	CheckMenuItemIf(pMenu, Settings().GetPromptWhenDeletingClips(), ID_QUICKOPTIONS_PROMPTTODELETECLIP);
	CheckMenuItemIf(pMenu, Settings().GetPasteAsAdmin(), ID_QUICKOPTIONS_ELEVATEPREVILEGESTOPASTEINTOELEVATEDAPPS);
	CheckMenuItemIf(pMenu, Settings().GetShowInTaskBar(), ID_QUICKOPTIONS_SHOWINTASKBAR);
	CheckMenuItemIf(pMenu, Settings().GetShowTextForFirstTenHotKeys(), ID_QUICKOPTIONS_SHOWTEXTFORFIRSTTENCOPYHOTKEYS);
	CheckMenuItemIf(pMenu, Settings().GetShowIfClipWasPasted(), ID_QUICKOPTIONS_SHOWINDICATORACLIPHASBEENPASTED);

	CheckMenuItemIf(pMenu, Settings().GetSearchDescription(), ID_MENU_SEARCHDESCRIPTION);
	CheckMenuItemIf(pMenu, Settings().GetSearchFullText(), ID_MENU_SEARCHFULLTEXT);
	CheckMenuItemIf(pMenu, Settings().GetSearchQuickPaste(), ID_MENU_SEARCHQUICKPASTE);
	CheckMenuItemIf(pMenu, m_bShowStarredClips, ID_MENU_SHOWSTARREDCLIPS);
	CheckMenuItemIf(pMenu, Settings().GetSimpleTextSearch(), ID_MENU_CONTAINSTEXTSEARCHONLY);
	CheckMenuItemIf(pMenu, Settings().GetRegExTextSearch(), ID_MENU_REGULAREXPRESSIONSEARCH);

	if (Settings().GetSimpleTextSearch() == FALSE &&
		Settings().GetRegExTextSearch() == FALSE)
	{
		pMenu->CheckMenuItem(ID_MENU_WILDCARDSEARCH, MF_CHECKED);
	}
}

void CQPasteWnd::CheckMenuItemIf(CMenu* pMenu, BOOL condition, UINT menuId)
{
	if (condition)
	{
		pMenu->CheckMenuItem(menuId, MF_CHECKED);
	}
}

void CQPasteWnd::CheckMenuItemForValue(CMenu* pMenu, std::span<const MenuValueCheck> checks, long value)
{
	for (const MenuValueCheck &check : checks)
	{
		if (check.value == value)
		{
			pMenu->CheckMenuItem(check.menuId, MF_CHECKED);
			return;
		}
	}
}


LRESULT CQPasteWnd::OnSearch(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	CString csText;
	m_search.GetWindowText(csText);

	FillList(csText);

	m_lstHeader.SetFocus();

	MoveControls();

	m_search.SetSel(-1, 0);

	return TRUE;
}


///////////////////////////////////////////////////////////////////////
//Menu Stuff
///////////////////////////////////////////////////////////////////////
void CQPasteWnd::OnMenuLinesperrow1()
{
	SetLinesPerRow(1, false, true);
}

void CQPasteWnd::OnMenuLinesperrow2()
{
	SetLinesPerRow(2, false, true);
}

void CQPasteWnd::OnMenuLinesperrow3()
{
	SetLinesPerRow(3, false, true);
}

void CQPasteWnd::OnMenuLinesperrow4()
{
	SetLinesPerRow(4, false, true);
}

void CQPasteWnd::OnMenuLinesperrow5()
{
	SetLinesPerRow(5, false, true);
}

void CQPasteWnd::SetLinesPerRow(int lines, bool force, bool resetListCount)
{
	ARRAY Indexs;
	int listCount = 0;

	if (resetListCount)
	{
		//save and restore state, list box seems to pain funny (gap at header) if items are not reset
		m_lstHeader.GetSelectionIndexes(Indexs);
		if (Indexs.GetCount() <= 0)
		{
			Indexs.Add(0);
		}
		listCount = m_lstHeader.GetItemCount();
		m_lstHeader.SetItemCountEx(0);
	}

	Settings().SetLinesPerRow(lines);
	m_lstHeader.SetNumberOfLinesPerRow(lines, force);

	ATL::CCritSecLock csLock(m_CritSection.m_sect);

	m_cf_dibCache.clear();
	m_cf_NO_dibCache.clear();
	m_cf_rtfCache.clear();
	m_cf_NO_rtfCache.clear();

	if (resetListCount)
	{
		m_lstHeader.SetItemCountEx(listCount);
		m_lstHeader.SetListPos(Indexs[0]);
		m_lstHeader.RefreshVisibleRows();
	}
}

void CQPasteWnd::OnMenuDelete()
{
	DeleteSelectedRows();
}

void CQPasteWnd::OnMenuPositioningAtcaret()
{
	Settings().SetQuickPastePosition(CGetSetOptions::PosAtCaret);
}

void CQPasteWnd::OnMenuPositioningAtcursor()
{
	Settings().SetQuickPastePosition(CGetSetOptions::PosAtCursor);
}

void CQPasteWnd::OnMenuPositioningAtpreviousposition()
{
	Settings().SetQuickPastePosition(CGetSetOptions::PosAtPrevious);
}

void CQPasteWnd::OnMenuOptions()
{
	theApp.m_pMainFrame->SendMessage(CDittoMessage::ShowOptions, 0, 0);
}

void CQPasteWnd::OnMenuExitprogram()
{
	::SendMessage(theApp.m_MainhWnd, WM_CLOSE, 0, 0);
}

void CQPasteWnd::OnMenuToggleConnectCV()
{
	this->DoAction(ActionEnums::TOGGLE_CLIPBOARD_CONNECTION);
}

void CQPasteWnd::OnUpdate32775(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::TOGGLE_CLIPBOARD_CONNECTION);
}

void CQPasteWnd::OnMenuProperties()
{
	this->DoAction(ActionEnums::CLIP_PROPERTIES);
}

void CQPasteWnd::UpdateFont()
{
	LOGFONT lf;
	Settings().GetFont(lf);
	lf.lfHeight = m_DittoWindow.m_dpi.Scale(lf.lfHeight);
	m_lstHeader.SetLogFont(lf);

	m_SearchFont.DeleteObject();
	m_SearchFont.CreateFont(-m_DittoWindow.m_dpi.Scale(15), 0, 0, 0, 400, 0, 0, 0, DEFAULT_CHARSET, 3, 2, 1, 34, _T("Segoe UI"));
	m_search.SetFont(&m_SearchFont);
	m_search.SetPromptFont(m_SearchFont);

	m_GroupTree.SetFont(&m_SearchFont);

	m_groupFont.DeleteObject();
	m_groupFont.CreateFont(-m_DittoWindow.m_dpi.Scale(12), 0, 0, 0, 400, 0, 1, 0, DEFAULT_CHARSET, 3, 2, 1, 34, _T("Segoe UI"));
	m_stGroup.SetFont(&m_groupFont);
	m_stGroup.SetBkColor(Settings().m_Theme.MainWindowBG());
	m_stGroup.SetTextColor(Settings().m_Theme.ListBoxEvenRowsText());

	m_noSearchResultsStatic.SetBkColor(Settings().m_Theme.MainWindowBG());
	m_noSearchResultsStatic.SetTextColor(Settings().m_Theme.ListBoxEvenRowsText());
	m_noSearchResultsStatic.SetFont(&m_SearchFont);

	m_lstHeader.CreateSmallFont();
}

void CQPasteWnd::OnMenuFirsttenhotkeysUsectrlnum()
{
	Settings().SetUseCtrlNumForFirstTenHotKeys(!Settings().GetUseCtrlNumForFirstTenHotKeys());
	m_lstHeader.RefreshVisibleRows();
}

void CQPasteWnd::OnMenuFirsttenhotkeysShowhotkeytext()
{
	Settings().SetShowTextForFirstTenHotKeys(!Settings().GetShowTextForFirstTenHotKeys());
	m_lstHeader.SetShowTextForFirstTenHotKeys(Settings().GetShowTextForFirstTenHotKeys());

	m_lstHeader.RefreshVisibleRows();
}

void CQPasteWnd::OnViewcaptionbaronRight()
{
	SetCaptionOn(CGetSetOptions::CaptionOnRight, false, Settings().m_Theme.GetCaptionSize(), Settings().m_Theme.GetCaptionFontSize());
	Settings().SetCaptionPos(CGetSetOptions::CaptionOnRight);
}

void CQPasteWnd::OnViewcaptionbaronBottom()
{
	SetCaptionOn(CGetSetOptions::CaptionOnBottom, false, Settings().m_Theme.GetCaptionSize(), Settings().m_Theme.GetCaptionFontSize());
	Settings().SetCaptionPos(CGetSetOptions::CaptionOnBottom);
}

void CQPasteWnd::OnViewcaptionbaronLeft()
{
	SetCaptionOn(CGetSetOptions::CaptionOnLeft, false, Settings().m_Theme.GetCaptionSize(), Settings().m_Theme.GetCaptionFontSize());
	Settings().SetCaptionPos(CGetSetOptions::CaptionOnLeft);
}

void CQPasteWnd::OnViewcaptionbaronTop()
{
	SetCaptionOn(CGetSetOptions::CaptionOnTop, false, Settings().m_Theme.GetCaptionSize(), Settings().m_Theme.GetCaptionFontSize());
	Settings().SetCaptionPos(CGetSetOptions::CaptionOnTop);
}

void CQPasteWnd::OnMenuAutohide()
{
	bool bAutoHide = !Settings().GetAutoHide();
	Settings().SetAutoHide(bAutoHide);
}

void CQPasteWnd::OnMenuViewfulldescription()
{
	this->DoAction(ActionEnums::SHOWDESCRIPTION);
}

void CQPasteWnd::OnMenuAllwaysontop()
{
	this->DoAction(ActionEnums::TOGGLESHOWPERSISTANT);
}

void CQPasteWnd::OnMenuNewGroup()
{
	this->DoAction(ActionEnums::NEWGROUP);
}

void CQPasteWnd::OnMenuNewGroupSelection()
{
	this->DoAction(ActionEnums::NEWGROUPSELECTION);
}

void CQPasteWnd::OnMenuQuickoptionsAllwaysshowdescription()
{
	Settings().SetAllwaysShowDescription(!Settings().m_bAllwaysShowDescription);

}

void CQPasteWnd::OnMenuQuickoptionsDoubleclickingoncaptionTogglesalwaysontop()
{
	Settings().SetDoubleClickingOnCaptionDoes(CGetSetOptions::TogglesAlwaysOnTop);

}

void CQPasteWnd::OnMenuQuickoptionsDoubleclickingoncaptionRollupwindow()
{
	Settings().SetDoubleClickingOnCaptionDoes(CGetSetOptions::RollsUpWindow);

}

void CQPasteWnd::OnMenuQuickoptionsDoubleclickingoncaptionTogglesshowdescription()
{
	Settings().SetDoubleClickingOnCaptionDoes(CGetSetOptions::TogglesAlwaysShowDescription);
}

void CQPasteWnd::OnMenuQuickoptionsPromptfornewgroupnames()
{
	Settings().SetPrompForNewGroupName(!Settings().m_bPrompForNewGroupName);
}

void CQPasteWnd::OnMenuViewgroups()
{
	this->DoAction(ActionEnums::SHOWGROUPS);
}

void CQPasteWnd::OnMenuQuickpropertiesSettoneverautodelete()
{
	CWaitCursor wait;
	ARRAY IDs;
	ARRAY Indexs;
	m_lstHeader.GetSelectionItemData(IDs);
	m_lstHeader.GetSelectionIndexes(Indexs);

	INT_PTR count = IDs.GetSize();

	for (int i = 0; i < count; i++)
	{
		try
		{
			theApp.m_db.execDMLEx(_T("UPDATE Main SET lDontAutoDelete = %d where lID = %d;"), (int)CTime::GetCurrentTime().GetTime(), IDs[i]);
		}
		catch (CppSQLite3Exception& e)
		{
			// stops before the list rows are marked: the list does not show a change that was not saved
			CErrorReport::Show(CStringUtil::Format(_T("Setting clip %d to never auto delete failed: %s"), IDs[i], e.errorMessage()));
			return;
		}
	}

	{
		ATL::CCritSecLock csLock(m_CritSection.m_sect);

		count = Indexs.GetSize();
		for (int row = 0; row < count; row++)
		{
			if (Indexs[row] < (int)m_listItems.size())
			{
				m_listItems[Indexs[row]].m_bDontAutoDelete = true;
			}
		}
	}

	m_lstHeader.RefreshVisibleRows();
}

void CQPasteWnd::OnMenuQuickpropertiesAutodelete()
{
	CWaitCursor wait;
	ARRAY IDs;
	ARRAY Indexs;
	m_lstHeader.GetSelectionItemData(IDs);
	m_lstHeader.GetSelectionIndexes(Indexs);

	INT_PTR count = IDs.GetSize();

	for (int i = 0; i < count; i++)
	{
		try
		{
			theApp.m_db.execDMLEx(_T("UPDATE Main SET lDontAutoDelete = 0 where lID = %d;"), IDs[i]);
		}
		catch (CppSQLite3Exception& e)
		{
			// stops before the list rows are marked: the list does not show a change that was not saved
			CErrorReport::Show(CStringUtil::Format(_T("Setting clip %d to auto delete failed: %s"), IDs[i], e.errorMessage()));
			return;
		}
	}

	{
		ATL::CCritSecLock csLock(m_CritSection.m_sect);
		count = Indexs.GetSize();
		for (int row = 0; row < count; row++)
		{
			if (Indexs[row] < (int)m_listItems.size())
			{
				m_listItems[Indexs[row]].m_bDontAutoDelete = false;
			}
		}
	}

	m_lstHeader.RefreshVisibleRows();
}

void CQPasteWnd::OnMenuQuickpropertiesRemovehotkey()
{
	CWaitCursor wait;
	ARRAY IDs;
	ARRAY Indexs;
	m_lstHeader.GetSelectionItemData(IDs);
	m_lstHeader.GetSelectionIndexes(Indexs);

	INT_PTR count = IDs.GetSize();

	for (int i = 0; i < count; i++)
	{
		try
		{
			theApp.m_db.execDMLEx(_T("UPDATE Main SET lShortCut = 0, globalShortCut = 0 where lID = %d;"), IDs[i]);
			g_HotKeys.Remove(IDs[i], CHotKey::PASTE_OPEN_CLIP);
			theApp.m_db.execDMLEx(_T("UPDATE Main SET MoveToGroupShortCut = 0, GlobalMoveToGroupShortCut = 0 where lID = %d;"), IDs[i]);
			g_HotKeys.Remove(IDs[i], CHotKey::MOVE_TO_GROUP);
		}
		catch (CppSQLite3Exception& e)
		{
			// stops before the list rows are marked: the list does not show a change that was not saved
			CErrorReport::Show(CStringUtil::Format(_T("Removing the hot keys of clip %d failed: %s"), IDs[i], e.errorMessage()));
			return;
		}
	}

	{
		ATL::CCritSecLock csLock(m_CritSection.m_sect);

		count = Indexs.GetSize();
		for (int row = 0; row < count; row++)
		{
			if (Indexs[row] < (int)m_listItems.size())
			{
				m_listItems[Indexs[row]].m_bHasShortCut = false;
			}
		}
	}

	m_lstHeader.RefreshVisibleRows();
}

void CQPasteWnd::OnQuickpropertiesRemovequickpaste()
{
	CWaitCursor wait;
	ARRAY IDs;
	ARRAY Indexs;
	m_lstHeader.GetSelectionItemData(IDs);
	m_lstHeader.GetSelectionIndexes(Indexs);

	INT_PTR count = IDs.GetSize();

	for (int i = 0; i < count; i++)
	{
		try
		{
			theApp.m_db.execDMLEx(_T("UPDATE Main SET QuickPasteText = '' where lID = %d;"), IDs[i]);
		}
		catch (CppSQLite3Exception& e)
		{
			// stops before the list rows are marked: the list does not show a change that was not saved
			CErrorReport::Show(CStringUtil::Format(_T("Removing the quick paste text of clip %d failed: %s"), IDs[i], e.errorMessage()));
			return;
		}
	}

	{
		ATL::CCritSecLock csLock(m_CritSection.m_sect);

		count = Indexs.GetSize();
		for (int row = 0; row < count; row++)
		{
			if (Indexs[row] < (int)m_listItems.size())
			{
				m_listItems[Indexs[row]].m_QuickPaste.Empty();
			}
		}
	}

	m_lstHeader.RefreshVisibleRows();
}

void CQPasteWnd::OnMenuGroupsMovetogroup()
{
	this->DoAction(ActionEnums::MOVE_CLIP_TO_GROUP);
}

void CQPasteWnd::OnMenuPasteplaintextonly()
{
	this->DoAction(ActionEnums::PASTE_SELECTED_PLAIN_TEXT);
}

void CQPasteWnd::OnPromptToDeleteClip()
{
	Settings().SetPromptWhenDeletingClips(!Settings().GetPromptWhenDeletingClips());
}

void CQPasteWnd::OnMakeTopStickyClip()
{
	this->DoAction(ActionEnums::MAKE_TOP_STICKY);
}

void CQPasteWnd::OnMakeLastStickyClip()
{
	this->DoAction(ActionEnums::MAKE_LAST_STICKY);
}

void CQPasteWnd::OnRemoveSticky()
{
	this->DoAction(ActionEnums::REMOVE_STICKY);
}

void CQPasteWnd::OnUpdateStickyclipsMaketopstickyclip(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::MAKE_TOP_STICKY);
}

void CQPasteWnd::OnUpdateStickyclipsMakelaststickyclip(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::MAKE_LAST_STICKY);
}

void CQPasteWnd::OnUpdateStickyclipsRemovestickysetting(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::REMOVE_STICKY);
}

void CQPasteWnd::OnElevateAppToPasteIntoElevatedApp()
{
	this->DoAction(ActionEnums::ELEVATE_PRIVlEGES);
}

void CQPasteWnd::OnMenuExport()
{
	CClipIDs IDs;
	INT_PTR lCount = m_lstHeader.GetSelectedCount();
	if (lCount <= 0)
	{
		return;
	}

	m_lstHeader.GetSelectionItemData(IDs);
	lCount = IDs.GetSize();
	if (lCount <= 0)
	{
		return;
	}

	OPENFILENAME ofn;
	TCHAR szFile[400];
	TCHAR szDir[400];

	memset(&szFile, 0, sizeof(szFile));
	memset(szDir, 0, sizeof(szDir));
	memset(&ofn, 0, sizeof(ofn));

	CString csInitialDir = Settings().GetLastImportDir();
	_tcscpy(szDir, csInitialDir);

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrFilter = _T("Exported Ditto Clips (.dto)\0*.dto\0\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = szDir;
	ofn.lpstrDefExt = _T("dto");
	// a save dialog: the file may be new (no OFN_FILEMUSTEXIST)
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

	m_bHideWnd = false;

	if (GetSaveFileName(&ofn))
	{
		using namespace nsPath;
		CPath path(CFileDialogPath::From(ofn));
		CString csPath(path.GetPath());
		Settings().SetLastExportDir(csPath);

		CString csFile(CFileDialogPath::From(ofn));
		IDs.Export(Settings(), csFile);
	}

	m_bHideWnd = true;
}

void CQPasteWnd::OnMenuImport()
{
	m_bHideWnd = false;
	theApp.ImportClips(m_hWnd);
	m_bHideWnd = true;
}

void CQPasteWnd::OnMenuQuickoptionsFont()
{
	m_bHideWnd = false;

	CDimWnd dimmer(this);

	CFont* pFont = m_lstHeader.GetFont();
	LOGFONT lf;
	pFont->GetLogFont(&lf);

	lf.lfHeight = m_DittoWindow.m_dpi.UnScale(lf.lfHeight);

	CFontDialog dlg(&lf);
	if (dlg.DoModal() == IDOK)
	{
		Settings().SetFont(*dlg.m_cf.lpLogFont);
		(*dlg.m_cf.lpLogFont).lfHeight = m_DittoWindow.m_dpi.Scale((*dlg.m_cf.lpLogFont).lfHeight);
		m_lstHeader.SetLogFont(*dlg.m_cf.lpLogFont);
		this->SetLinesPerRow(Settings().GetLinesPerRow(), true, true);
	}

	m_bHideWnd = true;
}

void CQPasteWnd::OnMenuQuickoptionsShowthumbnails()
{
	Settings().SetDrawThumbnail(!Settings().m_bDrawThumbnail);
	m_lstHeader.RefreshVisibleRows();
}

void CQPasteWnd::OnMenuQuickoptionsDrawrtftext()
{
	Settings().SetDrawRTF(!Settings().m_bDrawRTF);
	m_lstHeader.RefreshVisibleRows();
}

void CQPasteWnd::OnMenuQuickoptionsPasteclipafterselection()
{
	Settings().SetSendPasteAfterSelection(!Settings().m_bSendPasteMessageAfterSelection);
}

void CQPasteWnd::OnMenuQuickoptionsFindasyoutype()
{
	Settings().SetFindAsYouType(!Settings().m_bFindAsYouType);
}

void CQPasteWnd::OnMenuQuickoptionsEnsureentirewindowisvisible()
{
	Settings().SetEnsureEntireWindowCanBeSeen(!Settings().m_bEnsureEntireWindowCanBeSeen);
}

void CQPasteWnd::OnMenuQuickoptionsShowclipsthatareingroupsinmainlist()
{
	Settings().SetShowAllClipsInMainList(!Settings().m_bShowAllClipsInMainList);

	CString csText;
	m_search.GetWindowText(csText);
	FillList(csText);
}

void CQPasteWnd::OnMenuEdititem()
{
	this->DoAction(ActionEnums::EDITCLIP);
}

void CQPasteWnd::OnMenuNewclip()
{
	this->DoAction(ActionEnums::NEWCLIP);
}


///////////////////////////////////////////////////////////////////////
//END END Menu Stuff
///////////////////////////////////////////////////////////////////////


LRESULT CQPasteWnd::OnDelete(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	DeleteSelectedRows();
	return TRUE;
}

void CQPasteWnd::DeleteSelectedRows()
{
	if (Settings().GetPromptWhenDeletingClips())
	{
		bool bStartValue = m_bHideWnd;
		m_bHideWnd = false;

		int nRet = MessageBox(theApp.m_Language.GetString("Delete_Clip", "Delete Selected Clips?"), _T("Ditto"), MB_OKCANCEL | MB_TOPMOST);

		m_bHideWnd = bStartValue;

		if (nRet != IDOK)
		{
			return;
		}
	}

	CClipIDs IDs;
	ARRAY Indexs;

	if (m_lstHeader.GetSelectedCount() == 0)
	{
		return;
	}


	m_lstHeader.GetSelectionItemData(IDs);
	m_lstHeader.GetSelectionIndexes(Indexs);

	DeleteClips(IDs, Indexs);
}

bool CQPasteWnd::DeleteClips(CClipIDs& IDs, ARRAY& Indexs)
{
	POSITION pos = m_lstHeader.GetFirstSelectedItemPosition();
	int nFirstSel = m_lstHeader.GetNextSelectedItem(pos);

	IDs.DeleteIDs(true, theApp.m_db);

	Indexs.SortDescending();
	INT_PTR count = Indexs.GetSize();

	int erasedCount = 0;

	{
		ATL::CCritSecLock csLock(m_CritSection.m_sect);

		for (int i = 0; i < count; i++)
		{
			if (Indexs[i] < (int)m_listItems.size())
			{
				RemoveFromImageRtfCache(Indexs[i]);
				g_HotKeys.Remove(m_lstHeader.GetItemData(Indexs[i]), CHotKey::PASTE_OPEN_CLIP);

				m_listItems.erase(m_listItems.begin() + Indexs[i]);
				erasedCount++;
			}
		}
	}

	CClip::m_LastAddedCRC = 0;

	m_extraDataThread.FireLoadAccelerators();

	m_lstHeader.SetItemCountEx(m_lstHeader.GetItemCount() - erasedCount);

	// if there are no items after the one we deleted, then select the last one.
	if (nFirstSel >= m_lstHeader.GetItemCount())
	{
		nFirstSel = m_lstHeader.GetItemCount() - 1;
	}

	m_lstHeader.SetListPos(nFirstSel);
	UpdateStatus();

	if (m_lstHeader.IsToolTipWindowVisible())
	{
		m_lstHeader.ShowFullDescription(false, true);
	}

	return true;
}

void CQPasteWnd::RemoveFromImageRtfCache(int row, int id)
{
	ATL::CCritSecLock csLock(m_CritSection.m_sect);

	if (id < 0)
	{
		id = m_lstHeader.GetItemData(row);
	}

	CF_DibTypeMap::iterator iterDib = m_cf_dibCache.find(id);
	if (iterDib != m_cf_dibCache.end())
	{
		m_cf_dibCache.erase(iterDib);
	}

	CF_NoDibTypeMap::iterator iterNoDib = m_cf_NO_dibCache.find(id);
	if (iterNoDib != m_cf_NO_dibCache.end())
	{
		m_cf_NO_dibCache.erase(iterNoDib);
	}

	CF_DibTypeMap::iterator iterRtf = m_cf_rtfCache.find(id);
	if (iterRtf != m_cf_rtfCache.end())
	{
		m_cf_rtfCache.erase(iterRtf);
	}

	CF_NoDibTypeMap::iterator iterNoRtf = m_cf_NO_rtfCache.find(id);
	if (iterNoRtf != m_cf_NO_rtfCache.end())
	{
		m_cf_NO_rtfCache.erase(iterNoRtf);
	}
}

CString CQPasteWnd::LoadDescription(int nItem)
{
	if (nItem < 0 || nItem >= m_lstHeader.GetItemCount())
	{
		return "";
	}

	CString cs;
	try
	{
		int id = m_lstHeader.GetItemData(nItem);

		CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT mText FROM Main WHERE lID = %d"), id);
		if (q.eof() == false)
		{
			cs = q.getStringField(0);
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Loading the clip's description failed: %s"), e.errorMessage()));
		return _T("");
	}

		return cs;
}

void CQPasteWnd::MoveSelection(bool down, bool requireModifersActive)
{
	if (m_bModifersMoveActive || requireModifersActive == false)
	{
		ARRAY arr;
		m_lstHeader.GetSelectionIndexes(arr);
		if (arr.GetCount() > 0)
		{
			int index = arr[0];

			if (down)
			{
				if (index < m_lstHeader.GetItemCount() - 1)
				{
					m_lstHeader.SetListPos(index + 1);
				}
			}
			else
			{
				if (index > 0)
				{
					m_lstHeader.SetListPos(index - 1);
				}
			}
		}
		else
		{
			if (m_lstHeader.GetItemCount() > 0)
			{
				m_lstHeader.SetListPos(0);
			}
		}
	}
}

void CQPasteWnd::OnKeyStateUp()
{
	if (Settings().m_moveSelectionOnOpenHotkey)
	{
		if (m_bModifersMoveActive)
		{
			CLogger::Log(_T("OnKeyStateUp"));
			SetTimer(TimerPasteFromModifier, Settings().GetKeyStatePasteDelay(), NULL);
		}
		else
		{
			CLogger::Log(_T("OnKeyStateUp - Modifers not active"));
		}
	}
}

void CQPasteWnd::SetKeyModiferState(bool bActive)
{
	if (Settings().m_moveSelectionOnOpenHotkey)
	{
		CLogger::Log(CStringUtil::Format(_T("SetKeyModiferState %d"), bActive));
		m_bModifersMoveActive = bActive;
	}
}

BOOL CQPasteWnd::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_MBUTTONUP:
	{
		CheckMiddleClickActions();
	}
	break;
	case WM_NCMOUSEMOVE:
	case WM_MOUSEMOVE:
	{
		TrackNonActiveMouseMove();
	}
	break;
	default:
		if (PreTranslateActionOrChar(pMsg))
		{
			return TRUE;
		}
		break;
	}
	return CWndEx::PreTranslateMessage(pMsg);
}

void CQPasteWnd::CheckMiddleClickActions()
{
	MSG msg;
	msg.lParam = 0;
	msg.wParam = CMouseKey::MiddleClick;
	msg.message = WM_KEYDOWN;
	if (CheckActions(&msg) == false)
	{
	}
}

void CQPasteWnd::TrackNonActiveMouseMove()
{
	if (Settings().m_bShowPersistent)
	{
		bool hasFocus = ::GetForegroundWindow() == m_hWnd;
		if (hasFocus == false)
		{
			ULONGLONG tick = GetTickCount64();
			if ((tick - m_lastNonActiveMouseMove) > 1000)
			{
				theApp.m_activeWnd.TrackActiveWnd(true);
				m_lastNonActiveMouseMove = GetTickCount64();
			}
		}
		else
		{
			m_lastNonActiveMouseMove = 0;
		}
	}
}

bool CQPasteWnd::PreTranslateActionOrChar(MSG* pMsg)
{
	if (CheckActions(pMsg))
	{
		return true;
	}
	else if (pMsg->message == WM_CHAR)
	{
		if (pMsg->hwnd == m_lstHeader.m_hWnd)
		{
			StartSearchWithChar((TCHAR)pMsg->wParam);

			return true;
		}
	}

	return false;
}

void CQPasteWnd::StartSearchWithChar(TCHAR ch)
{
	bool bStartNewSearch = (::GetFocus() != m_search.m_hWnd);
	m_search.SetFocus();
	CString csSearch;
	if (bStartNewSearch == false)
	{
		m_search.GetWindowText(csSearch);
	}
	csSearch += ch;
	m_search.SetWindowText(csSearch);
	int nLen = csSearch.GetLength();
	m_search.SetSel(nLen, nLen);

	OnSearchEditChange();
}

bool CQPasteWnd::CheckActions(MSG* pMsg)
{
	bool ret = false;
	CAccel a;

	if (m_bModifersMoveActive)
	{
		if (m_modifierKeyActions.OnMsg(pMsg, a, Settings().m_doubleKeyStrokeTimeout))
		{
			ret = DoAction(a);
		}
	}

	if (ret == false)
	{
		if (m_actions.OnMsg(pMsg, a, Settings().m_doubleKeyStrokeTimeout))
		{
			KillTimer(TimerDoAction);
			ret = DoAction(a);
		}
		else if (a.Cmd > 0)
		{
			m_timerAction = a;
			SetTimer(TimerDoAction, Settings().m_doubleKeyStrokeTimeout, NULL);

			ret = true;
		}
	}

	return ret;
}

bool CQPasteWnd::DoAction(DWORD cmd)
{
	CAccel a(0, cmd);
	return DoAction(a);
}

// The actions DoAction runs through a member function without arguments. returnsResult is false
// for the actions whose handler result DoAction ignores (it returns false for them).
const std::array<CQPasteWnd::ActionHandler, 88> CQPasteWnd::s_actionHandlers{ {
	{ ActionEnums::SHOWDESCRIPTION, &CQPasteWnd::DoActionShowDescription, true },
	{ ActionEnums::NEXTDESCRIPTION, &CQPasteWnd::DoActionNextDescription, true },
	{ ActionEnums::PREVDESCRIPTION, &CQPasteWnd::DoActionPrevDescription, true },
	{ ActionEnums::SHOWMENU, &CQPasteWnd::DoActionShowMenu, true },
	{ ActionEnums::SYSTEM_MENU, &CQPasteWnd::DoActionShowSystemMenu, true },
	{ ActionEnums::NEWGROUP, &CQPasteWnd::DoActionNewGroup, true },
	{ ActionEnums::NEWGROUPSELECTION, &CQPasteWnd::DoActionNewGroupSelection, true },
	{ ActionEnums::TOGGLEFILELOGGING, &CQPasteWnd::DoActionToggleFileLogging, true },
	{ ActionEnums::TOGGLEOUTPUTDEBUGSTRING, &CQPasteWnd::DoActionToggleOutputDebugString, true },
	{ ActionEnums::CLOSEWINDOW, &CQPasteWnd::DoActionCloseWindow, true },
	{ ActionEnums::FORCE_CLOSE_WINDOW, &CQPasteWnd::DoActionForceCloseWindow, true },
	{ ActionEnums::NEXTTABCONTROL, &CQPasteWnd::DoActionNextTabControl, true },
	{ ActionEnums::PREVTABCONTROL, &CQPasteWnd::DoActionPrevTabControl, true },
	{ ActionEnums::SHOWGROUPS, &CQPasteWnd::DoActionShowGroups, true },
	{ ActionEnums::NEWCLIP, &CQPasteWnd::DoActionNewClip, true },
	{ ActionEnums::EDITCLIP, &CQPasteWnd::DoActionEditClip, true },
	{ ActionEnums::MODIFIER_ACTVE_SELECTIONUP, &CQPasteWnd::DoModifierActiveActionSelectionUp, true },
	{ ActionEnums::MODIFIER_ACTVE_SELECTIONDOWN, &CQPasteWnd::DoModifierActiveActionSelectionDown, true },
	{ ActionEnums::MODIFIER_ACTVE_MOVEFIRST, &CQPasteWnd::DoModifierActiveActionMoveFirst, true },
	{ ActionEnums::MODIFIER_ACTVE_MOVELAST, &CQPasteWnd::DoModifierActiveActionMoveLast, true },
	{ ActionEnums::CANCELFILTER, &CQPasteWnd::DoActionCancelFilter, true },
	{ ActionEnums::HOMELIST, &CQPasteWnd::DoActionHomeList, true },
	{ ActionEnums::BACKGRROUP, &CQPasteWnd::DoActionBackGroup, true },
	{ ActionEnums::TOGGLESHOWPERSISTANT, &CQPasteWnd::DoActionToggleShowPersistant, true },
	{ ActionEnums::PASTE_SELECTED, &CQPasteWnd::DoActionPasteSelected, true },
	{ ActionEnums::DELETE_SELECTED, &CQPasteWnd::DoActionDeleteSelected, true },
	{ ActionEnums::CLIP_PROPERTIES, &CQPasteWnd::DoActionClipProperties, true },
	{ ActionEnums::PASTE_SELECTED_PLAIN_TEXT, &CQPasteWnd::DoActionPasteSelectedPlainText, true },
	{ ActionEnums::MOVE_CLIP_TO_GROUP, &CQPasteWnd::DoActionMoveClipToGroup, true },
	{ ActionEnums::ELEVATE_PRIVlEGES, &CQPasteWnd::DoActionElevatePrivleges, true },
	{ ActionEnums::SHOW_IN_TASKBAR, &CQPasteWnd::DoShowInTaskBar, true },
	{ ActionEnums::COMPARE_SELECTED_CLIPS, &CQPasteWnd::DoClipCompare, true },
	{ ActionEnums::SELECT_LEFT_SIDE_COMPARE, &CQPasteWnd::DoSelectLeftSideCompare, true },
	{ ActionEnums::SELECT_RIGHT_SITE_AND_DO_COMPARE, &CQPasteWnd::DoSelectRightSideAndDoCompare, true },
	{ ActionEnums::EXPORT_TO_TEXT_FILE, &CQPasteWnd::DoExportToTextFile, true },
	{ ActionEnums::EXPORT_TO_QR_CODE, &CQPasteWnd::DoExportToQRCode, true },
	{ ActionEnums::EXPORT_TO_BITMAP_FILE, &CQPasteWnd::DoExportToBitMapFile, true },
	{ ActionEnums::SAVE_CURRENT_CLIPBOARD, &CQPasteWnd::DoSaveCurrentClipboard, true },
	{ ActionEnums::MOVE_CLIP_DOWN, &CQPasteWnd::DoMoveClipDown, true },
	{ ActionEnums::MOVE_CLIP_UP, &CQPasteWnd::DoMoveClipUp, true },
	{ ActionEnums::MOVE_CLIP_TOP, &CQPasteWnd::DoMoveClipTOP, true },
	{ ActionEnums::MOVE_CLIP_LAST, &CQPasteWnd::DoMoveClipLast, true },
	{ ActionEnums::FILTER_ON_SELECTED_CLIP, &CQPasteWnd::DoFilterOnSelectedClip, true },
	{ ActionEnums::PASTE_UPPER_CASE, &CQPasteWnd::DoPasteUpperCase, true },
	{ ActionEnums::PASTE_LOWER_CASE, &CQPasteWnd::DoPasteLowerCase, true },
	{ ActionEnums::PASTE_CAPITALiZE, &CQPasteWnd::DoPasteCapitalize, true },
	{ ActionEnums::PASTE_SENTENCE_CASE, &CQPasteWnd::DoPasteSentenceCase, true },
	{ ActionEnums::INVERT_CASE, &CQPasteWnd::DoInvertCase, true },
	{ ActionEnums::PASTE_REMOVE_LINE_FEEDS, &CQPasteWnd::DoPasteRemoveLineFeeds, true },
	{ ActionEnums::PASTE_ADD_ONE_LINE_FEED, &CQPasteWnd::DoPastePlusAddLineFeed, true },
	{ ActionEnums::PASTE_ADD_TWO_LINE_FEEDS, &CQPasteWnd::DoPasteAddTwoLineFeeds, true },
	{ ActionEnums::PASTE_TYPOGLYCEMIA, &CQPasteWnd::DoPasteTypoglycemia, true },
	{ ActionEnums::PASTE_ADD_CURRENT_TIME, &CQPasteWnd::DoPasteAddCurrentTime, true },
	{ ActionEnums::CONFIG_SHOW_FIRST_TEN_TEXT, &CQPasteWnd::OnShowFirstTenText, true },
	{ ActionEnums::CONFIG_SHOW_CLIP_WAS_PASTED, &CQPasteWnd::OnShowClipWasPasted, true },
	{ ActionEnums::TOGGLE_LAST_GROUP_TOGGLE, &CQPasteWnd::OnToggleLastGroupToggle, true },
	{ ActionEnums::MAKE_TOP_STICKY, &CQPasteWnd::DoActionMakeTopSticky, true },
	{ ActionEnums::MAKE_LAST_STICKY, &CQPasteWnd::OnMakeLastSticky, true },
	{ ActionEnums::REMOVE_STICKY, &CQPasteWnd::OnRemoveStickySetting, true },
	{ ActionEnums::GLOBAl_HOTKEYS, &CQPasteWnd::OnGlobalHotkyes, true },
	{ ActionEnums::DELETE_CLIP_DATA, &CQPasteWnd::OnDeleteClipData, true },
	{ ActionEnums::IMPORT_CLIP, &CQPasteWnd::OnImportClip, true },
	{ ActionEnums::REPLACE_TOP_STICKY_CLIP, &CQPasteWnd::DoActionReplaceTopStickyClip, true },
	{ ActionEnums::SAVE_CF_HDROP_FIlE_DATA, &CQPasteWnd::DoActionSaveCF_HDROP_FileData, true },
	{ ActionEnums::TOGGLE_CLIPBOARD_CONNECTION, &CQPasteWnd::DoActionToggleClipboardConnection, true },
	{ ActionEnums::MOVE_SELECTION_UP, &CQPasteWnd::DoActionMoveSelectionUp, true },
	{ ActionEnums::MOVE_SELECTION_DOWN, &CQPasteWnd::DoActionMoveSelectionDown, true },
	{ ActionEnums::TOGGLE_DESCRIPTION_WORD_WRAP, &CQPasteWnd::DoActionToggleDescriptionWordWrap, true },
	{ ActionEnums::APPLY_LAST_SEARCH, &CQPasteWnd::DoActionApplyLastSearch, true },
	{ ActionEnums::TOGGLE_SEARCH_METHOD, &CQPasteWnd::DoActionToggleSearchMethod, true },
	{ ActionEnums::PASTE_DONT_MOVE_CLIP, &CQPasteWnd::DoActionPasteDontMoveClip, true },
	{ ActionEnums::PASTE_TRIM_WHITE_SPACE, &CQPasteWnd::DoActionPasteTrimWhiteSpace, true },
	{ ActionEnums::PASTE_POSIXIFY_PATHS, &CQPasteWnd::DoActionPastePosixifyPaths, true },
	{ ActionEnums::TRANSPARENCY_TOGGLE, &CQPasteWnd::DoActionToggleTransparency, false },
	{ ActionEnums::TRANSPARENCY_INCREASE, &CQPasteWnd::DoActionIncreaseTransparency, false },
	{ ActionEnums::TRANSPARENCY_DECREASE, &CQPasteWnd::DoActionDecreaseTransparency, false },
	{ ActionEnums::SLUGIFY, &CQPasteWnd::DoActionSlugify, false },
	{ ActionEnums::COPY_SELECTION, &CQPasteWnd::DoCopySelection, true },
	{ ActionEnums::REFRESH_LIST, &CQPasteWnd::DoRefreshList, true },
	{ ActionEnums::DELETE_ALL_NON_USED_CLIPS, &CQPasteWnd::DoDeleteAllNonUsedClips, true },
	{ ActionEnums::SET_DRAG_FILE_NAME, &CQPasteWnd::DoSetDragFileName, true },
	{ ActionEnums::PASTE_CAMEL_CASE, &CQPasteWnd::DoPasteCamelCase, true },
	{ ActionEnums::PASTE_MULTI_IMAGE_HORIZONTAL, &CQPasteWnd::DoPasteImagesHorz, true },
	{ ActionEnums::PASTE_MULTI_IMAGE_VERTICAL, &CQPasteWnd::DoPasteImagesVert, true },
	{ ActionEnums::ASCII_TEXT_ONLY, &CQPasteWnd::DoPasteAsciiOnly, true },
	{ ActionEnums::GENERATE_GUID, &CQPasteWnd::DoActionGenerateGuid, true },
	{ ActionEnums::PASTE_AS_IMAGE, &CQPasteWnd::DoPasteAsImage, true },
	{ ActionEnums::SHOW_STARRED_CLIPS, &CQPasteWnd::DoActionShowStarredClips, true },
} };

// The paste-by-position actions: OpenIndex(index, plainText)
const std::array<CQPasteWnd::PastePositionAction, 20> CQPasteWnd::s_pastePositionActions{ {
	{ ActionEnums::PASTE_POSITION_1, 0, false },
	{ ActionEnums::PASTE_POSITION_2, 1, false },
	{ ActionEnums::PASTE_POSITION_3, 2, false },
	{ ActionEnums::PASTE_POSITION_4, 3, false },
	{ ActionEnums::PASTE_POSITION_5, 4, false },
	{ ActionEnums::PASTE_POSITION_6, 5, false },
	{ ActionEnums::PASTE_POSITION_7, 6, false },
	{ ActionEnums::PASTE_POSITION_8, 7, false },
	{ ActionEnums::PASTE_POSITION_9, 8, false },
	{ ActionEnums::PASTE_POSITION_10, 9, false },
	{ ActionEnums::PASTE_POSITION_1_PLAIN_TEXT, 0, true },
	{ ActionEnums::PASTE_POSITION_2_PLAIN_TEXT, 1, true },
	{ ActionEnums::PASTE_POSITION_3_PLAIN_TEXT, 2, true },
	{ ActionEnums::PASTE_POSITION_4_PLAIN_TEXT, 3, true },
	{ ActionEnums::PASTE_POSITION_5_PLAIN_TEXT, 4, true },
	{ ActionEnums::PASTE_POSITION_6_PLAIN_TEXT, 5, true },
	{ ActionEnums::PASTE_POSITION_7_PLAIN_TEXT, 6, true },
	{ ActionEnums::PASTE_POSITION_8_PLAIN_TEXT, 7, true },
	{ ActionEnums::PASTE_POSITION_9_PLAIN_TEXT, 8, true },
	{ ActionEnums::PASTE_POSITION_10_PLAIN_TEXT, 9, true },
} };

// The fixed transparency actions: SetTransparency(percent); DoAction returns false for them
const std::array<CQPasteWnd::TransparencyAction, 9> CQPasteWnd::s_transparencyActions{ {
	{ ActionEnums::TRANSPARENCY_NONE, 0 },
	{ ActionEnums::TRANSPARENCY_5, 5 },
	{ ActionEnums::TRANSPARENCY_10, 10 },
	{ ActionEnums::TRANSPARENCY_15, 15 },
	{ ActionEnums::TRANSPARENCY_20, 20 },
	{ ActionEnums::TRANSPARENCY_25, 25 },
	{ ActionEnums::TRANSPARENCY_30, 30 },
	{ ActionEnums::TRANSPARENCY_35, 35 },
	{ ActionEnums::TRANSPARENCY_40, 40 },
} };

bool CQPasteWnd::DoAction(CAccel a)
{
	for (const ActionHandler &entry : s_actionHandlers)
	{
		if (static_cast<DWORD>(entry.action) == a.Cmd)
		{
			return RunActionHandler(entry);
		}
	}

	for (const PastePositionAction &entry : s_pastePositionActions)
	{
		if (static_cast<DWORD>(entry.action) == a.Cmd)
		{
			return OpenIndex(entry.index, entry.plainText);
		}
	}

	for (const TransparencyAction &entry : s_transparencyActions)
	{
		if (static_cast<DWORD>(entry.action) == a.Cmd)
		{
			SetTransparency(entry.percent);
			return false;
		}
	}

	//unknown action
	return false;
}

bool CQPasteWnd::RunActionHandler(const ActionHandler &entry)
{
	bool result = (this->*entry.handler)();
	if (entry.returnsResult == false)
	{
		return false;
	}

	return result;
}

bool CQPasteWnd::DoActionMakeTopSticky()
{
	return OnMakeTopSticky(false);
}

bool CQPasteWnd::DoActionShowStarredClips()
{
	OnMenuShowStarredClips();
	return true;
}

bool CQPasteWnd::DoSetDragFileName()
{
	m_bHideWnd = false;

	CGroupName Name;

	CDimWnd dimmer(this);

	INT_PTR nRet = Name.DoModal();

	if (nRet == IDOK)
	{
		CString csName = Name.m_csName;
		Settings().SetTempDragFileName(csName);
	}

	m_bHideWnd = true;

	return true;
}

bool CQPasteWnd::DoActionPasteDontMoveClip()
{
	CSpecialPasteOptions pasteOptions;
	pasteOptions.m_updateClipOrder = false;
	OpenSelection(pasteOptions);

	return true;
}

bool CQPasteWnd::DoActionPasteTrimWhiteSpace()
{
	CSpecialPasteOptions pasteOptions;
	pasteOptions.m_trimWhiteSpace = true;
	OpenSelection(pasteOptions);

	return true;
}

bool CQPasteWnd::DoActionPastePosixifyPaths()
{
	CSpecialPasteOptions pasteOptions;
	pasteOptions.m_PosixifyPaths = true;
	OpenSelection(pasteOptions);

	return true;
}

bool CQPasteWnd::DoActionToggleSearchMethod()
{
	if (Settings().GetRegExTextSearch())
	{
		//if regex go back to wildcard
		Settings().SetSimpleTextSearch(FALSE);
		Settings().SetRegExTextSearch(FALSE);
	}
	else if (Settings().GetSimpleTextSearch())
	{
		//if contains search go to regex
		Settings().SetSimpleTextSearch(FALSE);
		Settings().SetRegExTextSearch(TRUE);
	}
	else
	{
		//if wildcard to to contains
		Settings().SetSimpleTextSearch(TRUE);
		Settings().SetRegExTextSearch(FALSE);
	}
	return true;
}

bool CQPasteWnd::DoActionApplyLastSearch()
{
	return m_search.ApplyLastSearch();
}

bool CQPasteWnd::DoActionToggleDescriptionWordWrap()
{
	if (m_lstHeader.IsToolTipWindowVisible() == FALSE)
		return false;

	bool ret = m_lstHeader.ToggleToolTipWordWrap();
	return (ret == true);
}

bool CQPasteWnd::DoActionShowDescription()
{
	bool ret = false;

	m_actions.m_handleRepeatKeys = true;

	CString csText;
	m_search.GetWindowText(csText);
	if (csText != _T(""))
	{
		m_search.AddToSearchHistory();
	}

	if (m_lstHeader.IsToolTipWindowVisible() == false)
	{
		ret = m_lstHeader.ShowFullDescription(false, false);
	}
	else
	{
		if (csText != _T(""))
		{
			m_lstHeader.DoToolTipSearch();
			ret = true;
		}
		else
		{
			m_lstHeader.HideToolTip();
			ret = true;
		}
	}

	return (ret == true);
}

bool CQPasteWnd::DoActionNextDescription()
{
	if (m_lstHeader.IsToolTipWindowVisible() == FALSE)
		return false;

	if (Settings().m_bAllwaysShowDescription)
		return false;

	m_actions.m_handleRepeatKeys = true;

	ARRAY Indexes;
	m_lstHeader.GetSelectionIndexes(Indexes);

	long caret = m_lstHeader.GetCaret();

	if (Indexes.GetCount() > 1)
	{
		for (int i = 0; i < Indexes.GetCount(); i++)
		{
			int index = Indexes[i];
			if (index == caret)
			{
				if (i < Indexes.GetCount() - 1)
				{
					caret = Indexes[i + 1];
					break;
				}
				else
				{
					caret = Indexes[0];
				}
			}
		}

		m_lstHeader.SetCaret(caret);
	}
	else
	{
		caret++;
		m_lstHeader.SetListPos(caret);
	}

	m_lstHeader.ShowFullDescription(false, true);

	return true;
}

bool CQPasteWnd::DoActionPrevDescription()
{
	if (m_lstHeader.IsToolTipWindowVisible() == FALSE)
		return false;

	if (Settings().m_bAllwaysShowDescription)
		return false;

	m_actions.m_handleRepeatKeys = true;

	ARRAY Indexes;
	m_lstHeader.GetSelectionIndexes(Indexes);

	long caret = m_lstHeader.GetCaret();

	if (Indexes.GetCount() > 1)
	{
		for (int i = ((int)Indexes.GetCount()) - 1; i >= 0; i--)
		{
			int index = Indexes[i];
			if (index == caret)
			{
				if (i > 0)
				{
					caret = Indexes[i - 1];
					break;
				}
				else
				{
					caret = Indexes[((int)Indexes.GetCount()) - 1];
				}
			}
		}

		m_lstHeader.SetCaret(caret);
	}
	else
	{
		caret--;
		m_lstHeader.SetListPos(caret);
	}

	m_lstHeader.ShowFullDescription(false, true);

	return true;
}

bool CQPasteWnd::DoActionShowMenu()
{
	ShowRightClickMenu();
	return true;
}

bool CQPasteWnd::DoActionShowSystemMenu()
{
	OnSystemButton();
	return true;
}

bool CQPasteWnd::DoActionNewGroup()
{
	NewGroup(false, theApp.GetValidGroupID());

	return true;
}

bool CQPasteWnd::DoActionNewGroupSelection()
{
	NewGroup(true, theApp.GetValidGroupID());

	return true;
}

bool CQPasteWnd::DoActionToggleFileLogging()
{
	if (Settings().m_bEnableDebugLogging)
	{
		CLogger::Log(_T("turning file logging OFF"));
	}

	Settings().m_bEnableDebugLogging = !Settings().m_bEnableDebugLogging;

	if (Settings().m_bEnableDebugLogging)
	{
		CLogger::Log(_T("turning file logging ON"));
	}

	return true;
}

bool CQPasteWnd::DoActionToggleOutputDebugString()
{
	if (Settings().m_bEnableDebugLogging)
	{
		CLogger::Log(_T("turning DebugString logging OFF"));
	}

	Settings().m_outputDebugStringLogging = !Settings().m_outputDebugStringLogging;

	if (Settings().m_bEnableDebugLogging)
	{
		CLogger::Log(_T("turning DebugString logging ON"));
	}

	return true;
}

bool CQPasteWnd::DoActionForceCloseWindow()
{
	CLogger::Log(_T("Force closing window from hot keys"));
	HideQPasteWindow(true);

	return true;
}

bool CQPasteWnd::DoActionCloseWindow()
{
	bool ret = false;
	if (m_bModifersMoveActive)
	{
		CLogger::Log(_T("Escape key hit setting modifers to NOT active"));
		m_bModifersMoveActive = false;
		ret = true;
	}
	else
	{
		CLogger::Log(_T("close 1"));

		if (m_lstHeader.IsToolTipShowPersistant() == false &&
			m_lstHeader.IsToolTipWindowVisible())
		{
			m_lstHeader.HidePopup(true);
			CLogger::Log(_T("close 2"));
			ret = true;
		}
		else if (m_strSQLSearch.IsEmpty() == FALSE)
		{
			OnCancelFilter(0, 0);
			CLogger::Log(_T("close 3"));
			ret = true;
		}
		else
		{
			if (Settings().GetShowPersistent() && this->GetMinimized() == false)
			{
				MinMaxWindow(CDittoWindow::ForceMin);
				theApp.m_activeWnd.ReleaseFocus();

				CLogger::Log(_T("close 4"));

				ret = true;
			}
			else
			{
				if (m_GroupTree.IsWindowVisible() == FALSE)
				{
					HideQPasteWindow(true);
					ret = true;
					CLogger::Log(_T("close 5"));
				}
			}
		}
	}

	CLogger::Log(_T("close 6"));

	return ret;
}

bool CQPasteWnd::DoActionNextTabControl()
{
	BOOL bPrev = FALSE;

	CWnd* pFocus = GetFocus();
	if (pFocus)
	{
		CWnd* pNextWnd = GetNextDlgTabItem(pFocus, bPrev);
		if (pNextWnd)
		{
			pNextWnd->SetFocus();
		}
	}
	return true;
}

bool CQPasteWnd::DoActionPrevTabControl()
{
	BOOL bPrev = TRUE;

	CWnd* pFocus = GetFocus();
	if (pFocus)
	{
		CWnd* pNextWnd = GetNextDlgTabItem(pFocus, bPrev);
		if (pNextWnd)
		{
			pNextWnd->SetFocus();
		}
	}
	return true;
}

bool CQPasteWnd::DoActionShowGroups()
{
	OnShowGroupsTop();

	return true;
}

bool CQPasteWnd::DoActionNewClip()
{
	CClipIDs IDs;
	IDs.Add(-1);
	theApp.EditItems(IDs, true, true);

	return true;
}

bool CQPasteWnd::DoActionEditClip()
{
	if (m_lstHeader.GetSelectedCount() == 0)
	{
		return false;
	}

	CClipIDs IDs;
	m_lstHeader.GetSelectionItemData(IDs);

	bool textOnly = false;
	if (GetKeyState(VK_SHIFT) & 0x8000)
	{
		textOnly = true;
	}

	theApp.EditItems(IDs, true, textOnly);

	return true;
}

bool CQPasteWnd::DoActionMoveSelectionUp()
{
	MoveSelection(false, false);
	m_actions.m_handleRepeatKeys = true;
	return true;
}

bool CQPasteWnd::DoActionMoveSelectionDown()
{
	MoveSelection(true, false);
	m_actions.m_handleRepeatKeys = true;
	return true;
}

bool CQPasteWnd::DoModifierActiveActionSelectionUp()
{
	if (m_bModifersMoveActive)
	{
		MoveSelection(false, true);
		m_modifierKeyActions.m_handleRepeatKeys = true;
		return true;
	}

	return false;
}

bool CQPasteWnd::DoModifierActiveActionSelectionDown()
{
	if (m_bModifersMoveActive)
	{
		MoveSelection(true, true);
		m_modifierKeyActions.m_handleRepeatKeys = true;
		return true;
	}

	return false;
}

bool CQPasteWnd::DoModifierActiveActionMoveFirst()
{
	if (m_bModifersMoveActive)
	{
		m_lstHeader.SetListPos(0);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoModifierActiveActionMoveLast()
{
	if (m_bModifersMoveActive)
	{
		if (m_lstHeader.GetItemCount() > 0)
		{
			m_lstHeader.SetListPos(m_lstHeader.GetItemCount() - 1);
		}
		return true;
	}

	return false;
}

bool CQPasteWnd::DoActionCancelFilter()
{
	m_bShowStarredClips = false;

	FillList();

	m_bHandleSearchTextChange = false;
	m_search.SetWindowText(_T(""));
	m_bHandleSearchTextChange = true;

	MoveControls();

	m_lstHeader.SetFocus();

	return true;
}

bool CQPasteWnd::DoActionHomeList()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		theApp.EnterGroupID(-1); // History
		return true;
	}

	return false;
}

bool CQPasteWnd::DoActionBackGroup()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		theApp.EnterGroupID(theApp.m_GroupParentID);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoActionToggleShowPersistant()
{
	if (m_lstHeader.IsToolTipWindowVisible())
	{
		m_lstHeader.ToggleToolTipShowPersistant();
	}
	else
	{
		theApp.ShowPersistent(!Settings().m_bShowPersistent);
		if (Settings().m_bShowPersistent)
		{
			::SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
		}

		MoveControls();

		UpdateStatus();
	}
	return true;
}

bool CQPasteWnd::DoActionPasteSelected()
{
	CSpecialPasteOptions pasteOptions;
	OpenSelection(pasteOptions);
	return true;
}

bool CQPasteWnd::DoActionDeleteSelected()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		m_actions.m_handleRepeatKeys = true;
		DeleteSelectedRows();
		return true;
	}

	return false;
}

bool CQPasteWnd::DoActionClipProperties()
{
	bool ret = false;

	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		m_bHideWnd = false;

		ARRAY IDs, Indexes;
		m_lstHeader.GetSelectionItemData(IDs);
		m_lstHeader.GetSelectionIndexes(Indexes);

		INT_PTR size = IDs.GetSize();
		if (size < 1)
		{
			return ret;
		}

		int id = IDs[0];
		int row = Indexes[0];

		if (id < 0)
		{
			return ret;
		}

		m_lstHeader.RemoveAllSelection();
		m_lstHeader.SetSelection(row);

		ret = ShowProperties(id, row);

		m_lstHeader.SetListPos(row);
	}

	return false;
}

bool CQPasteWnd::ShowProperties(int id, int row)
{
	if (id < 0)
	{
		return false;
	}

	m_bHideWnd = false;

	CDimWnd dimmer(this);

	CCopyProperties props(id, this);
	INT_PTR doModalRet = props.DoModal();

	if (doModalRet == IDOK)
	{
		{
			ATL::CCritSecLock csLock(m_CritSection.m_sect);

			if (row < 0)
			{
				row = FindListRow(id, row);
			}

			if (row >= 0 &&
				row < (int)m_listItems.size())
			{
				CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT * FROM Main WHERE lID = %d"), id);
				if (!q.eof())
				{
					FillMainTable(m_listItems[row], q);
				}

				RemoveFromImageRtfCache(row);
			}
		}

		m_extraDataThread.FireLoadAccelerators();

		m_lstHeader.RefreshVisibleRows();

		if (props.m_lGroupChangedTo >= 0)
		{
			CSpecialPasteOptions pasteOptions;
			OpenID(props.m_lGroupChangedTo, pasteOptions);
		}

		m_lstHeader.SetFocus();
	}

	m_bHideWnd = true;
	return true;
}

int CQPasteWnd::FindListRow(int id, int notFoundRow)
{
	int row = notFoundRow;
	int index = 0;
	std::vector<CMainTable>::iterator iter = m_listItems.begin();
	while (iter != m_listItems.end())
	{
		if (iter->m_lID == id)
		{
			row = index;
			break;
		}
		iter++;
		index++;
	}

	return row;
}

bool CQPasteWnd::DoActionPasteSelectedPlainText()
{
	CSpecialPasteOptions pasteOptions;
	pasteOptions.m_pasteAsPlainText = true;
	OpenSelection(pasteOptions);
	return true;
}

bool CQPasteWnd::DoActionMoveClipToGroup()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		m_bHideWnd = false;

		CDimWnd dimmer(this);

		CMoveToGroupDlg dlg;

		INT_PTR nRet = dlg.DoModal();
		if (nRet == IDOK)
		{
			int nGroup = dlg.GetSelectedGroup();

			CLogger::Log(CStringUtil::Format(_T("Move to Group, GroupId: %d"), nGroup));

			if (nGroup >= -1)
			{
				CClipIDs IDs;
				m_lstHeader.GetSelectionItemData(IDs);

				IDs.MoveTo(nGroup);
			}
			FillList();
		}

		m_bHideWnd = true;
		return true;
	}

	return false;
}

bool CQPasteWnd::DoActionElevatePrivleges()
{
	Settings().SetPasteAsAdmin(!Settings().GetPasteAsAdmin());

	return true;
}

bool CQPasteWnd::DoShowInTaskBar()
{
	Settings().SetShowInTaskBar(!Settings().GetShowInTaskBar());

	theApp.RefreshShowInTaskBar();

	return true;
}

bool CQPasteWnd::DoClipCompare()
{
	ARRAY IDs;
	m_lstHeader.GetSelectionItemData(IDs);

	if (IDs.GetCount() > 1)
	{
		if (!Settings().m_bShowPersistent)
		{
			HideQPasteWindow(false, false);
		}
		else if (Settings().GetAutoHide())
		{
			MinMaxWindow(CDittoWindow::ForceMin);
		}

		CClipCompare compare(Settings());
		compare.Compare(IDs[0], IDs[1]);

		return true;
	}
	else
	{
		CLogger::Log(CStringUtil::Format(_T("DoClipCompare, at least 2 clips need to be selected, count: %d"), IDs.GetCount()));
	}

	return false;
}

bool CQPasteWnd::DoSelectLeftSideCompare()
{
	ARRAY IDs;
	m_lstHeader.GetSelectionItemData(IDs);

	if (IDs.GetCount() > 0)
	{
		m_leftSelectedCompareId = IDs[0];

		return true;
	}
	else
	{
		CLogger::Log(CStringUtil::Format(_T("DoSelectLeftSideCompare, no selected clip, not assigning left side")));
	}

	return false;
}

bool CQPasteWnd::DoSelectRightSideAndDoCompare()
{
	if (m_leftSelectedCompareId > 0)
	{
		ARRAY IDs;
		m_lstHeader.GetSelectionItemData(IDs);

		if (IDs.GetCount() > 0)
		{
			int rightId = IDs[0];

			if (!Settings().m_bShowPersistent)
			{
				HideQPasteWindow(false, false);
			}
			else if (Settings().GetAutoHide())
			{
				MinMaxWindow(CDittoWindow::ForceMin);
			}

			CClipCompare compare(Settings());
			compare.Compare(m_leftSelectedCompareId, rightId);

			return true;
		}
		else
		{
			CLogger::Log(CStringUtil::Format(_T("DoSelectRightSideAndDoCompare, no selected clips")));
		}
	}
	else
	{
		CLogger::Log(CStringUtil::Format(_T("DoSelectRightSideAndDoCompare, no left side selected, select left side first")));
	}

	return false;
}

bool CQPasteWnd::DoExportToTextFile()
{
	bool ret = false;

	CClipIDs IDs;
	INT_PTR lCount = m_lstHeader.GetSelectedCount();
	if (lCount <= 0)
	{
		return ret;
	}

	m_lstHeader.GetSelectionItemData(IDs);
	lCount = IDs.GetSize();
	if (lCount <= 0)
	{
		return ret;
	}

	OPENFILENAME ofn;
	TCHAR szFile[400];
	TCHAR szDir[400];

	memset(&szFile, 0, sizeof(szFile));
	memset(szDir, 0, sizeof(szDir));
	memset(&ofn, 0, sizeof(ofn));

	CString csInitialDir = Settings().GetLastImportDir();
	_tcscpy(szDir, csInitialDir);

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrFilter = _T("Exported Ditto Clips (.txt)\0*.txt\0\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = szDir;
	ofn.lpstrDefExt = _T("txt");
	// a save dialog: the file may be new (no OFN_FILEMUSTEXIST)
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

	m_bHideWnd = false;

	if (GetSaveFileName(&ofn))
	{
		using namespace nsPath;
		CString startingFilePath = CFileDialogPath::From(ofn);
		CPath path(CFileDialogPath::From(ofn));
		CString csPath(path.GetPath());
		CString csExt = path.GetExtension();
		path.RemoveExtension();
		CString csFileName = path.GetName();

		Settings().SetLastExportDir(csPath);
		ExportFileNames names{};
		names.startingFilePath = startingFilePath;
		names.path = csPath;
		names.fileName = csFileName;
		names.ext = csExt;
		names.lastFileCheckId = 1;

		for (int i = 0; i < IDs.GetCount(); i++)
		{
			int id = IDs[i];

			CClip clip(Settings());
			if (clip.LoadFormats(id, true))
			{
				CString savePath = NextExportFilePath(names, IDs.GetCount());

				if (savePath != _T(""))
				{
					clip.WriteTextToFile(savePath, true, true, false);

					ret = true;
				}
				else
				{
					CLogger::Log(CStringUtil::Format(_T("Failed to find a valid file name for starting path: %s"), startingFilePath.GetString()));
				}
			}
		}
	}

	m_bHideWnd = true;

	return ret;
}

CString CQPasteWnd::NextExportFilePath(ExportFileNames &names, INT_PTR clipCount)
{
	CString savePath = names.startingFilePath;
	if (clipCount > 1 ||
		CFileSystem::FileExists(names.startingFilePath))
	{
		savePath = _T("");

		for (int y = names.lastFileCheckId; y < 1000000; y++)
		{
			CString testFilePath;
			testFilePath.Format(_T("%s%s_%d.%s"), names.path.GetString(), names.fileName.GetString(), y, names.ext.GetString());
			if (CFileSystem::FileExists(testFilePath) == FALSE)
			{
				savePath = testFilePath;
				names.lastFileCheckId = y + 1;
				break;
			}
		}
	}

	return savePath;
}

bool CQPasteWnd::ShowQRCode(const CString& clipText, const CString& description)
{
	std::vector<std::byte> bitmap;
	try
	{
		bitmap = DittoCore::QrBitmap::Render(std::string(CTextConvert::UnicodeToUTF8(clipText).GetString()));
	}
	catch (const std::length_error&)
	{
		CErrorReport::Show(CStringUtil::Format(_T("The clip's text (%d characters) is too long for a QR code."), clipText.GetLength()));
		return false;
	}

	auto viewer = std::make_unique<QRCodeViewer>();

	// before any window exists: on failure the unique_ptr deletes the viewer
	if (!viewer->LoadQrBitmap(std::move(bitmap)))
	{
		CErrorReport::Show(CStringUtil::Format(_T("Ditto could not create the QR code window (error %u)."), ::GetLastError()));
		return false;
	}

	LOGFONT lf;
	Settings().GetFont(lf);

	QRCodeViewer *pViewer{ viewer.release() }; // ownership: the window (PostNcDestroy deletes it, also when Create fails)
	if (!pViewer->CreateEx(this, description, m_lstHeader.GetRowHeight(), lf))
	{
		CErrorReport::Show(CStringUtil::Format(_T("Ditto could not create the QR code window (error %u)."), ::GetLastError()));
		return false;
	}

	pViewer->ShowWindow(SW_SHOW);
	return true;
}

bool CQPasteWnd::DoExportToQRCode()
{
	bool ret = false;

	ARRAY IDs;
	m_lstHeader.GetSelectionItemData(IDs);

	if (IDs.GetCount() > 0)
	{
		int id = IDs[0];
		CClip clip(Settings());
		if (clip.LoadMainTable(id))
		{
			if (clip.LoadFormats(id, true))
			{
				CString clipText = clip.GetUnicodeTextFormat();

				ret = ShowQRCode(clipText, clip.Description());
			}
		}
	}

	return ret;
}

bool CQPasteWnd::DoActionGenerateGuid()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		CSpecialPasteOptions pasteOptions;
		pasteOptions.m_pasteGuid = true;
		OpenSelection(pasteOptions);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoPasteAsImage()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		CSpecialPasteOptions pasteOptions;
		pasteOptions.m_pasteAsImage = true;
		OpenSelection(pasteOptions);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoSaveCurrentClipboard()
{
	theApp.m_pMainFrame->PostMessage(CDittoMessage::SaveClipboard, 0, 0);

	return true;
}

bool CQPasteWnd::DoMoveClipDown()
{
	ARRAY IDs;
	m_lstHeader.GetSelectionItemData(IDs);

	m_actions.m_handleRepeatKeys = true;

	if (IDs.GetCount() > 0)
	{
		bool sort = false;
		for (int i = ((int)IDs.GetCount()) - 1; i >= 0; i--)
		{
			int id = IDs[i];
			CClip clip(Settings());
			if (clip.LoadMainTable(id))
			{
				clip.MoveDown(theApp.m_GroupID);
				clip.ModifyMainTable();

				sort = SyncClipDataToArrayData(clip);
			}
		}

		if (sort)
		{
			if (theApp.m_GroupID > 0)
			{
				std::sort(m_listItems.begin(), m_listItems.end(), CMainTable::GroupSortDesc);
			}
			else
			{
				std::sort(m_listItems.begin(), m_listItems.end(), CMainTable::SortDesc);
			}

			SelectIds(IDs);

			m_lstHeader.RefreshVisibleRows();
			m_lstHeader.RedrawWindow();
		}
	}

	return true;
}

bool CQPasteWnd::DoMoveClipUp()
{
	ARRAY IDs;
	m_lstHeader.GetSelectionItemData(IDs);

	m_actions.m_handleRepeatKeys = true;

	if (IDs.GetCount() > 0)
	{
		bool sort = false;
		for (int i = 0; i < IDs.GetCount(); i++)
		{
			int id = IDs[i];
			CClip clip(Settings());
			if (clip.LoadMainTable(id))
			{
				clip.MoveUp(theApp.m_GroupID);
				clip.ModifyMainTable();

				sort = SyncClipDataToArrayData(clip);
			}
		}

		if (sort)
		{
			if (theApp.m_GroupID > 0)
			{
				std::sort(m_listItems.begin(), m_listItems.end(), CMainTable::GroupSortDesc);
			}
			else
			{
				std::sort(m_listItems.begin(), m_listItems.end(), CMainTable::SortDesc);
			}

			SelectIds(IDs);

			m_lstHeader.RefreshVisibleRows();
			m_lstHeader.RedrawWindow();
		}
	}

	return true;
}

bool CQPasteWnd::DoMoveClipLast()
{
	ARRAY IDs;
	m_lstHeader.GetSelectionItemData(IDs);

	if (IDs.GetCount() > 0)
	{
		bool sort = false;
		for (int i = 0; i < IDs.GetCount(); i++)
		{
			int id = IDs[i];
			CClip clip(Settings());
			if (clip.LoadMainTable(id))
			{
				if (theApp.m_GroupID > 0)
				{
					clip.MakeLastGroupOrder();
				}
				else
				{
					clip.MakeLastOrder();
				}
				clip.ModifyMainTable();

				//have we loaded all clips, if so then sort and select
				// a list control's item count is never negative
				if (m_listItems.size() == static_cast<size_t>(m_lstHeader.GetItemCount()))
				{
					sort = SyncClipDataToArrayData(clip);
				}
				else
				{
					//haven't loaded all clips so this will be out of what we have loaded
					//remove this from the list, will be shown as they scroll down the list
					EraseListItem(clip.ID());
				}
			}
		}

		if (sort)
		{
			if (theApp.m_GroupID > 0)
			{
				std::sort(m_listItems.begin(), m_listItems.end(), CMainTable::GroupSortDesc);
			}
			else
			{
				std::sort(m_listItems.begin(), m_listItems.end(), CMainTable::SortDesc);
			}

			SelectIds(IDs);
		}

		m_lstHeader.RefreshVisibleRows();
		m_lstHeader.RedrawWindow();
	}

	return true;
}

void CQPasteWnd::EraseListItem(int id)
{
	std::vector<CMainTable>::iterator iter = m_listItems.begin();
	while (iter != m_listItems.end())
	{
		if (iter->m_lID == id)
		{
			m_listItems.erase(iter);
			break;
		}
		iter++;
	}
}

bool CQPasteWnd::DoMoveClipTOP()
{
	ARRAY IDs;
	m_lstHeader.GetSelectionItemData(IDs);

	if (IDs.GetCount() > 0)
	{
		bool sort = false;
		for (int i = 0; i < IDs.GetCount(); i++)
		{
			int id = IDs[i];
			CClip clip(Settings());
			if (clip.LoadMainTable(id))
			{
				if (theApp.m_GroupID > 0)
				{
					clip.MakeLatestGroupOrder();
				}
				else
				{
					clip.MakeLatestOrder();
				}
				clip.ModifyMainTable();

				sort = SyncClipDataToArrayData(clip);
			}
		}

		if (sort)
		{
			if (theApp.m_GroupID > 0)
			{
				std::sort(m_listItems.begin(), m_listItems.end(), CMainTable::GroupSortDesc);
			}
			else
			{
				std::sort(m_listItems.begin(), m_listItems.end(), CMainTable::SortDesc);
			}

			SelectIds(IDs);

			m_lstHeader.RefreshVisibleRows();
			m_lstHeader.RedrawWindow();
		}
	}

	return true;
}

bool CQPasteWnd::DoFilterOnSelectedClip()
{
	bool ret = false;
	ARRAY IDs, Indexes;
	m_lstHeader.GetSelectionItemData(IDs);

	INT_PTR size = IDs.GetSize();
	if (size > 0)
	{
		int id = IDs[0];

		ATL::CCritSecLock csLock(m_CritSection.m_sect);
		std::vector<CMainTable>::iterator iter = m_listItems.begin();
		while (iter != m_listItems.end())
		{
			if (iter->m_lID == id)
			{
				m_bHandleSearchTextChange = false;
				m_search.SetWindowText(iter->m_Desc);
				m_bHandleSearchTextChange = true;
				OnSearch(0, 0);
				ret = true;
				break;
			}
			iter++;
		}
	}

	return ret;
}

bool CQPasteWnd::DoPasteUpperCase()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		CSpecialPasteOptions pasteOptions;
		pasteOptions.m_pasteUpperCase = true;
		OpenSelection(pasteOptions);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoPasteCamelCase()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		CSpecialPasteOptions pasteOptions;
		pasteOptions.m_pasteCamelCase = true;
		OpenSelection(pasteOptions);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoPasteImagesHorz()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		CSpecialPasteOptions pasteOptions;
		pasteOptions.m_pasteImagesHorizontal = true;
		OpenSelection(pasteOptions);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoPasteImagesVert()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		CSpecialPasteOptions pasteOptions;
		pasteOptions.m_pasteImagesVertically = true;
		OpenSelection(pasteOptions);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoPasteAsciiOnly()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		CSpecialPasteOptions pasteOptions;
		pasteOptions.m_pasteAsciiOnly = true;
		OpenSelection(pasteOptions);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoPasteLowerCase()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		CSpecialPasteOptions pasteOptions;
		pasteOptions.m_pasteLowerCase = true;
		OpenSelection(pasteOptions);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoPasteCapitalize()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		CSpecialPasteOptions pasteOptions;
		pasteOptions.m_pasteCapitalize = true;
		OpenSelection(pasteOptions);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoPasteSentenceCase()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		CSpecialPasteOptions pasteOptions;
		pasteOptions.m_pasteSentenceCase = true;
		OpenSelection(pasteOptions);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoInvertCase()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		CSpecialPasteOptions pasteOptions;
		pasteOptions.m_invertCase = true;
		OpenSelection(pasteOptions);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoPasteRemoveLineFeeds()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		CSpecialPasteOptions pasteOptions;
		pasteOptions.m_pasteRemoveLineFeeds = true;
		OpenSelection(pasteOptions);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoPastePlusAddLineFeed()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		CSpecialPasteOptions pasteOptions;
		pasteOptions.m_pasteAddOneLineFeed = true;
		OpenSelection(pasteOptions);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoPasteAddTwoLineFeeds()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		CSpecialPasteOptions pasteOptions;
		pasteOptions.m_pasteAddTwoLineFeeds = true;
		OpenSelection(pasteOptions);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoPasteTypoglycemia()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		CSpecialPasteOptions pasteOptions;
		pasteOptions.m_pasteTypoglycemia = true;
		OpenSelection(pasteOptions);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoPasteAddCurrentTime()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		CSpecialPasteOptions pasteOptions;
		pasteOptions.m_pasteAddingDateTime = true;
		OpenSelection(pasteOptions);
		return true;
	}

	return false;
}

bool CQPasteWnd::OnShowFirstTenText()
{
	Settings().SetShowTextForFirstTenHotKeys(!Settings().GetShowTextForFirstTenHotKeys());
	m_lstHeader.SetShowTextForFirstTenHotKeys(Settings().GetShowTextForFirstTenHotKeys());

	m_lstHeader.RefreshVisibleRows();
	m_lstHeader.RedrawWindow();
	return true;
}

bool CQPasteWnd::OnShowClipWasPasted()
{
	Settings().SetShowIfClipWasPasted(!Settings().GetShowIfClipWasPasted());
	m_lstHeader.SetShowIfClipWasPasted(Settings().GetShowIfClipWasPasted());

	m_lstHeader.RefreshVisibleRows();
	m_lstHeader.RedrawWindow();
	return true;
}

bool CQPasteWnd::OnToggleLastGroupToggle()
{
	int newGroupId = -2;
	if (theApp.m_GroupID > 0)
	{
		Settings().SetLastGroupToggle(theApp.m_GroupID);
		newGroupId = -1;
	}
	else
	{
		newGroupId = Settings().GetLastGroupToggle();
	}

	if (newGroupId >= -1)
	{
		theApp.EnterGroupID(newGroupId);
	}

	return true;
}

bool CQPasteWnd::OnMakeTopSticky(bool forceSort)
{
	ARRAY IDs;
	m_lstHeader.GetSelectionItemData(IDs);

	if (IDs.GetCount() > 0)
	{
		bool sort = forceSort;
		for (int i = ((int)IDs.GetCount()) - 1; i >= 0; i--)
		{
			int id = IDs[i];
			CClip clip(Settings());
			if (clip.LoadMainTable(id))
			{
				clip.MakeStickyTop(theApp.m_GroupID);
				clip.ModifyMainTable();

				sort = SyncClipDataToArrayData(clip);
			}
		}

		if (sort)
		{
			if (theApp.m_GroupID > 0)
			{
				std::sort(m_listItems.begin(), m_listItems.end(), CMainTable::GroupSortDesc);
			}
			else
			{
				std::sort(m_listItems.begin(), m_listItems.end(), CMainTable::SortDesc);
			}

			SelectIds(IDs);

			m_lstHeader.RefreshVisibleRows();
			m_lstHeader.RedrawWindow();
		}
	}

	return true;
}

bool CQPasteWnd::OnMakeLastSticky()
{
	ARRAY IDs;
	m_lstHeader.GetSelectionItemData(IDs);

	if (IDs.GetCount() > 0)
	{
		bool sort = false;
		for (int i = ((int)IDs.GetCount()) - 1; i >= 0; i--)
		{
			int id = IDs[i];
			CClip clip(Settings());
			if (clip.LoadMainTable(id))
			{
				clip.MakeStickyLast(theApp.m_GroupID);
				clip.ModifyMainTable();

				sort = SyncClipDataToArrayData(clip);
			}
		}

		if (sort)
		{
			if (theApp.m_GroupID > 0)
			{
				std::sort(m_listItems.begin(), m_listItems.end(), CMainTable::GroupSortDesc);
			}
			else
			{
				std::sort(m_listItems.begin(), m_listItems.end(), CMainTable::SortDesc);
			}

			SelectIds(IDs);

			m_lstHeader.RefreshVisibleRows();
			m_lstHeader.RedrawWindow();
		}
	}

	return true;
}

bool CQPasteWnd::OnRemoveStickySetting()
{
	ARRAY IDs;
	m_lstHeader.GetSelectionItemData(IDs);

	if (IDs.GetCount() > 0)
	{
		bool sort = false;
		for (int i = ((int)IDs.GetCount()) - 1; i >= 0; i--)
		{
			RemoveStickyInternal(IDs[i], sort);
		}

		//theApp.m_FocusID = id;

		if (sort)
		{
			if (theApp.m_GroupID > 0)
			{
				std::sort(m_listItems.begin(), m_listItems.end(), CMainTable::GroupSortDesc);
			}
			else
			{
				std::sort(m_listItems.begin(), m_listItems.end(), CMainTable::SortDesc);
			}

			//SelectFocusID();

			m_lstHeader.RefreshVisibleRows();
			m_lstHeader.RedrawWindow();
		}
	}

	return true;
}

void CQPasteWnd::RemoveStickyInternal(int id, bool& sort)
{
	CClip clip(Settings());
	if (clip.LoadMainTable(id))
	{
		if (clip.RemoveStickySetting(theApp.m_GroupID))
		{
			clip.ModifyMainTable();

			std::vector<CMainTable>::iterator iter = m_listItems.begin();
			while (iter != m_listItems.end())
			{
				if (iter->m_lID == id)
				{
					if (theApp.m_GroupID > 0)
					{
						iter->m_stickyClipGroupOrder = clip.m_stickyClipGroupOrder;
					}
					else
					{
						iter->m_stickyClipOrder = clip.m_stickyClipOrder;
					}
					sort = true;
					break;
				}
				iter++;
			}
		}
	}
}

bool CQPasteWnd::OnNewClip()
{
	CWnd* pWnd = AfxGetMainWnd();
	if (pWnd != NULL)
	{
		pWnd->SendMessage(WM_COMMAND, ID_FIRST_NEWCLIP, 0);
	}

	return true;
}

bool CQPasteWnd::OnImportClip()
{
	CWnd* pWnd = AfxGetMainWnd();
	if (pWnd != NULL)
	{
		pWnd->SendMessage(WM_COMMAND, ID_FIRST_IMPORT, 0);
	}

	return true;
}

bool CQPasteWnd::DoActionReplaceTopStickyClip()
{
	ARRAY IDs;
	IDs.Add(m_lstHeader.GetItemData(0));

	if (IDs.GetCount() > 0)
	{
		bool sort = false;
		for (int i = ((int)IDs.GetCount()) - 1; i >= 0; i--)
		{
			RemoveStickyInternal(IDs[i], sort);
		}

		OnMakeTopSticky(true);
	}

	return true;
}

bool CQPasteWnd::DoActionSaveCF_HDROP_FileData()
{
	if (m_lstHeader.GetSelectedCount() == 0)
	{
		return false;
	}

	CWaitCursor wait;

	CString errorMessage;

	CClipIDs IDs;
	m_lstHeader.GetSelectionItemData(IDs);
	ARRAY Indexs;
	m_lstHeader.GetSelectionIndexes(Indexs);

	if (IDs.GetCount() > 0)
	{
		for (int i = 0; i < min(Indexs.GetCount(), IDs.GetCount()); i++)
		{
			int row = Indexs[i];
			int id = IDs[i];
			SaveClipFileData(row, id, errorMessage);
		}
	}

	if (errorMessage.GetLength() > 0)
	{
		m_popupMsg.Show(errorMessage, CPoint(0, 0), true);
		SetTimer(TimerErrorMsg, Settings().GetErrorMsgPopupTimeout(), NULL);
	}

	m_lstHeader.RefreshVisibleRows();

	return true;
}

void CQPasteWnd::SaveClipFileData(int row, int id, CString &errorMessage)
{
	CClip clip(Settings());
	if (clip.LoadMainTable(id))
	{
		if (clip.LoadFormats(id))
		{
			CString localErrorMessage;
			if (clip.AddFileDataToData(localErrorMessage))
			{
				if (row >= 0 &&
					row < (int)m_listItems.size())
				{
					CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT * FROM Main WHERE lID = %d"), id);
					if (!q.eof())
					{
						FillMainTable(m_listItems[row], q);
					}
				}
			}

			errorMessage += localErrorMessage;
		}
	}
}

bool CQPasteWnd::DoActionToggleClipboardConnection()
{
	theApp.ToggleConnectCV();
	UpdateStatus();

	return true;
}

bool CQPasteWnd::OnDeleteClipData()
{
	CWnd* pWnd = AfxGetMainWnd();
	if (pWnd != NULL)
	{
		pWnd->SendMessage(WM_COMMAND, ID_FIRST_DELETECLIPDATA, 0);
	}

	return true;
}

bool CQPasteWnd::OnGlobalHotkyes()
{
	CWnd* pWnd = AfxGetMainWnd();
	if (pWnd != NULL)
	{
		pWnd->SendMessage(WM_COMMAND, ID_FIRST_GLOBALHOTKEYS, 0);
	}

	return true;
}

bool CQPasteWnd::DoExportToBitMapFile()
{
	bool ret = false;

	CClipIDs IDs;
	INT_PTR lCount = m_lstHeader.GetSelectedCount();
	if (lCount <= 0)
	{
		return ret;
	}

	m_lstHeader.GetSelectionItemData(IDs);
	lCount = IDs.GetSize();
	if (lCount <= 0)
	{
		return ret;
	}

	OPENFILENAME ofn;
	TCHAR szFile[400];
	TCHAR szDir[400];

	memset(&szFile, 0, sizeof(szFile));
	memset(szDir, 0, sizeof(szDir));
	memset(&ofn, 0, sizeof(ofn));

	CString csInitialDir = Settings().GetLastImportDir();
	_tcscpy(szDir, csInitialDir);

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	// the list of filter strings ends with an empty one
	ofn.lpstrFilter = _T("PNG (*.png)\0*.png\0BMP (*.bmp)\0*.bmp\0JPEG (*.jpeg)\0*.jpeg\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = szDir;
	ofn.lpstrDefExt = _T("png");
	// a save dialog: the file may be new (no OFN_FILEMUSTEXIST)
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

	m_bHideWnd = false;

	if (GetSaveFileName(&ofn))
	{
		CWaitCursor wait;

		using namespace nsPath;
		CString startingFilePath = CFileDialogPath::From(ofn);
		CPath path(CFileDialogPath::From(ofn));
		CString csPath(path.GetPath());
		CString csExt = path.GetExtension();
		path.RemoveExtension();
		CString csFileName = path.GetName();

		Settings().SetLastExportDir(csPath);

		ExportFileNames names{};
		names.startingFilePath = startingFilePath;
		names.path = csPath;
		names.fileName = csFileName;
		names.ext = csExt;
		names.lastFileCheckId = 1;

		for (int i = 0; i < IDs.GetCount(); i++)
		{
			int id = IDs[i];

			CClip toSave(Settings());
			toSave.LoadFormats(id);

			if (HasExportImage(toSave) == false)
				continue;

			CString savePath = NextExportFilePath(names, IDs.GetCount());

			if (savePath != _T(""))
			{
				ret = toSave.WriteImageToFileOrReport(savePath, _T("export"));
			}
			else
			{
				CLogger::Log(CStringUtil::Format(_T("Failed to find a valid file name for starting path: %s"), startingFilePath.GetString()));
			}
		}
	}

	m_bHideWnd = true;

	return ret;
}

bool CQPasteWnd::HasExportImage(CClip &toSave)
{
	CClipFormat* png = NULL;
	CClipFormat* bitmap = toSave.m_Formats.FindFormat(CF_DIB);
	if (bitmap == NULL)
	{
		png = toSave.m_Formats.FindFormat(theApp.m_PNG_Format);
	}

	return !(bitmap == NULL && png == NULL);
}

LRESULT CQPasteWnd::OnCancelFilter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	this->DoAction(ActionEnums::CANCELFILTER);
	return 1;
}

LRESULT CQPasteWnd::OnPostOptions(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	UpdateFont();
	LoadShortcuts();

	m_lstHeader.SetShowTextForFirstTenHotKeys(Settings().GetShowTextForFirstTenHotKeys());
	m_lstHeader.SetShowIfClipWasPasted(Settings().GetShowIfClipWasPasted());
	m_lstHeader.SetNumberOfLinesPerRow(Settings().GetLinesPerRow(), true);

	SetCurrentTransparency();

	if (Settings().m_tooltipTimeout > 0 ||
		Settings().m_tooltipTimeout == -1)
	{
		m_lstHeader.EnableToolTips();
	}
	else
	{
		m_lstHeader.EnableToolTips(FALSE);
	}

	return 1;
}

void CQPasteWnd::OnClose()
{
	HideQPasteWindow(true);
}

void CQPasteWnd::OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pLV = (NM_LISTVIEW*)pNMHDR;
	CProcessPaste paste(Settings());
	paste.m_pastedFromGroup = (theApp.m_GroupID > 0);

	if (CKeyboard::IsControlPressed())
	{
		paste.m_pasteOptions.m_dragDropFilesOnly = true;
	}
	else
	{
		paste.m_pasteOptions.m_placeCF_HDROP_OnDrag = Settings().GetAddCFHDROP_OnDrag();
	}

	CClipIDs& clips = paste.GetClipIDs();

	m_lstHeader.GetSelectionItemData(clips);

	if (clips.GetSize() <= 0)
	{
		ASSERT(0); // does this ever happen ??
		clips.Add(m_lstHeader.GetItemData(pLV->iItem));
	}

	this->SetTimer(TimerDragHideWindow, 500, NULL);

	if (!paste.DoDrag() && !paste.m_lastErrorMessage.IsEmpty())  // FALSE without a message: drop cancelled
	{
		CString errorMessage;
		errorMessage.Format(_T("Drag Error - %s"), paste.m_lastErrorMessage.GetString());
		m_popupMsg.Show(errorMessage, CPoint(0, 0), true);
		SetTimer(TimerErrorMsg, Settings().GetErrorMsgPopupTimeout(), NULL);
	}

	KillTimer(TimerDragHideWindow);


	if (Settings().m_bShowPersistent)
	{
		ShowQPasteWindow(0);
	}

	*pResult = 0;
}

DROPEFFECT CQPasteWnd::OnDragEnter(COleDataObject* /*pDataObject*/, DWORD /*dwKeyState*/, CPoint /*point*/)
{
	return DROPEFFECT_COPY;
}

DROPEFFECT CQPasteWnd::OnDragOver(COleDataObject* /*pDataObject*/, DWORD /*dwKeyState*/, CPoint /*point*/)
{
	return DROPEFFECT_COPY;
}

BOOL CQPasteWnd::OnDrop(COleDataObject* /*pDataObject*/, DROPEFFECT /*dropEffect*/, CPoint /*point*/)
{
	return TRUE;
}

void CQPasteWnd::OnDragLeave()
{

}

void CQPasteWnd::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CWndEx::OnSysKeyDown(nChar, nRepCnt, nFlags);
}

void CQPasteWnd::GetDispInfo(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	LV_ITEM* pItem = &(pDispInfo)->item;

	if (pItem->mask & LVIF_TEXT)
	{
		switch (pItem->iSubItem)
		{
		case 0:
			// reads only the loaded list rows (no database access), so it has no SQLite failure to report
			GetDispInfoText(pItem);

				break;
		}
	}

	if (pItem->mask & LVIF_PARAM)
	{
		GetDispInfoParam(pItem);
	}

	if (pItem->mask & CQListCtrl::s_lvifCfDib && Settings().m_bDrawThumbnail)
	{
		GetDispInfoExtraFormat(pItem, CF_DIB, m_cf_NO_dibCache, m_cf_dibCache);
	}

	if (pItem->mask & CQListCtrl::s_lvifCfRichText && Settings().m_bDrawRTF)
	{
		GetDispInfoExtraFormat(pItem, theApp.m_RTFFormat, m_cf_NO_rtfCache, m_cf_rtfCache);
	}
}

void CQPasteWnd::GetDispInfoText(LV_ITEM* pItem)
{
	ATL::CCritSecLock csLock(m_CritSection.m_sect);

	if ((int)m_listItems.size() > pItem->iItem &&
		m_listItems[pItem->iItem].m_lID > 0)
	{
		CString cs = ListItemDisplayText(m_listItems[pItem->iItem]);

		CControlTextBuffer::CopyCut(pItem->pszText, pItem->cchTextMax, cs);

		//						CLogger::Log(CStringUtil::Format(_T("DrawItem index %d - "), pItem->iItem));//, pItem->pszText));
	}
	else
	{
		QueueListItemLoad(pItem->iItem);
	}
}

CString CQPasteWnd::ListItemDisplayText(const CMainTable &item) const
{
	CString cs;
	if (item.m_bDontAutoDelete)
	{
		cs += _T("<noautodelete>");
	}

	if (item.m_bHasShortCut)
	{
		cs += _T("<shortcut>");
	}

	if (item.m_bIsGroup)
	{
		cs += _T("<group>");
	}

	if (IsListItemSticky(item))
	{
		cs += _T("<sticky>");
	}

	// attached to a group
	if (item.m_bHasParent)
	{
		cs += _T("<ingroup>");
	}

	if (item.m_QuickPaste.IsEmpty() == FALSE)
	{
		cs += _T("<qpastetext>");
	}

	if (item.m_dateCopied != item.m_datePasted)
	{
		cs += "<pasted>";
	}

	// pipe is the "end of symbols" marker
	cs += "|" + CMainTableFunctions::GetDisplayText(Settings().m_nLinesPerRow, item.m_Desc, Settings().m_bDescShowLeadingWhiteSpace);

	return cs;
}

bool CQPasteWnd::IsListItemSticky(const CMainTable &item)
{
	if (theApp.m_GroupID > 0)
	{
		return item.m_stickyClipGroupOrder != CClip::InvalidSticky;
	}

	return item.m_stickyClipOrder != CClip::InvalidSticky;
}

void CQPasteWnd::QueueListItemLoad(int item)
{
	bool addToLoadItems = true;

	for (std::list<CPoint>::iterator it = m_loadItems.begin(); it != m_loadItems.end(); it++)
	{
		if (item >= it->x && item <= it->y)
		{
			addToLoadItems = false;
			break;
		}
	}

	if (addToLoadItems)
	{
		CPoint loadItem(item, (m_lstHeader.GetTopIndex() + (m_lstHeader.GetCountPerPage() * 2)));

		//CLogger::Log(CStringUtil::Format(_T("DrawItem index %d, add: %d"), loadItem.x, loadItem.y));
		m_loadItems.push_back(loadItem);
	}

	m_thread.FireLoadItems(false);
}

void CQPasteWnd::GetDispInfoParam(LV_ITEM* pItem)
{
	switch (pItem->iSubItem)
	{
	case 0:
	{
		ATL::CCritSecLock csLock(m_CritSection.m_sect);

		if ((int)m_listItems.size() > pItem->iItem)
		{
			pItem->lParam = m_listItems[pItem->iItem].m_lID;
		}
	}

	break;
	}
}

void CQPasteWnd::GetDispInfoExtraFormat(LV_ITEM* pItem, CLIPFORMAT cfType, CF_NoDibTypeMap &noFormatCache, CF_DibTypeMap &formatCache)
{
	ATL::CCritSecLock csLock(m_CritSection.m_sect);

	if ((int)m_listItems.size() > pItem->iItem)
	{
		CF_NoDibTypeMap::iterator iterNoFormat = noFormatCache.find(m_listItems[pItem->iItem].m_lID);
		if (iterNoFormat == noFormatCache.end())
		{
			CF_DibTypeMap::iterator iterFormat = formatCache.find(m_listItems[pItem->iItem].m_lID);
			if (iterFormat == formatCache.end())
			{
				QueueExtraDataLoad(pItem->iItem, cfType);
			}
			else
			{
				if (iterFormat->second.m_hgData != NULL)
				{
					pItem->lParam = (LPARAM) & (iterFormat->second);
				}
			}
		}
	}
}

void CQPasteWnd::QueueExtraDataLoad(int row, CLIPFORMAT cfType)
{
	bool exists = false;
	for (std::list<CClipFormatQListCtrl>::iterator it = m_ExtraDataLoadItems.begin(); it != m_ExtraDataLoadItems.end(); it++)
	{
		if (it->m_cfType == cfType && it->m_parentId == m_listItems[row].m_lID)
		{
			exists = true;
			break;
		}
	}

	if (exists == false)
	{
		CClipFormatQListCtrl format;
		format.m_cfType = cfType;
		format.m_parentId = m_listItems[row].m_lID;
		format.m_clipRow = row;
		format.m_autoDeleteData = true;
		format.m_counter = m_extraDataCounter++;
		m_ExtraDataLoadItems.push_back(format);

		m_extraDataThread.FireLoadExtraData(m_lstHeader.GetRowHeight());
	}
}

CString CQPasteWnd::GetDisplayText(int dontAutoDelete, int shortCut, bool isGroup, int parentID, CString text)
{
	CString cs;
	if (dontAutoDelete)
	{
		cs += "*";
	}

	if (shortCut > 0)
	{
		cs += "s";
	}

	if (isGroup)
	{
		cs += "G";
	}

	// attached to a group
	if (parentID > 0)
	{
		cs += "!";
	}

	// pipe is the "end of symbols" marker
	cs += "|" + CMainTableFunctions::GetDisplayText(Settings().m_nLinesPerRow, text, Settings().m_bDescShowLeadingWhiteSpace);

	return cs;
}

void CQPasteWnd::OnGetToolTipText(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	CQListToolTipText* pInfo = (CQListToolTipText*)pNMHDR;
	if (!pInfo)
	{
		return;
	}

	if (pInfo->lItem < 0)
	{
		CControlTextBuffer::CopyCut(pInfo->pszText, pInfo->cchTextMax, _T("no item selected"));
		return;
	}

	try
	{
		CString cs;
		CString clipData = _T("");

		int id = m_lstHeader.GetItemData(pInfo->lItem);
		CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT lID, mText, lDate, lShortCut, clipOrder, clipGroupOrder, stickyClipOrder, stickyClipGroupOrder, lDontAutoDelete, QuickPasteText, lastPasteDate, globalShortCut, lParentID FROM Main WHERE lID = %d"), id);
		if (q.eof() == false)
		{
			CString clipText = q.getStringField(1);

			cs += ToolTipClipLines(clipText);


#ifdef _DEBUG
			clipData += CStringUtil::Format(_T("(Index = %d) (Seq = %f) (Group Seq = %f) (Sticky Seq = %f) (Sticky Group Seq = %f)\n"),
				pInfo->lItem,
				q.getFloatField(_T("clipOrder")), q.getFloatField(_T("clipGroupOrder")),
				q.getFloatField(_T("stickyClipOrder")), q.getFloatField(_T("stickyClipGroupOrder")));
#endif

			AppendToolTipClipDetails(q, clipData);
		}

		cs = cs.Left(pInfo->cchTextMax - clipData.GetLength());

		cs += "\r\n\r\n";
		cs += clipData;

		CControlTextBuffer::CopyCut(pInfo->pszText, pInfo->cchTextMax, cs);
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Loading the clip's tool tip failed: %s"), e.errorMessage()));
		return;
	}
}

void CQPasteWnd::AppendToolTipClipDetails(CppSQLite3Query &q, CString &clipData)
{
	clipData += CStringUtil::Format(_T("\r\nDatabase ID: %d"), q.getIntField(_T("lID")));

	COleDateTime time((time_t)q.getInt64Field(_T("lDate")));
	clipData += "\r\nAdded: " + time.Format();

	COleDateTime modified((time_t)q.getInt64Field(_T("lastPasteDate")));
	clipData += "\r\nLast Used: " + modified.Format();

	if (q.getIntField(_T("lDontAutoDelete")) > 0)
	{
		clipData += "\r\nNever Auto Delete";
	}

	CString csQuickPaste = q.getStringField(_T("QuickPasteText"));

	if (csQuickPaste.IsEmpty() == FALSE)
	{
		clipData += "\nQuick Paste = ";
		clipData += csQuickPaste;
	}

	AppendToolTipShortCut(q, clipData);

	AppendToolTipSticky(q, clipData);

	int parentId = q.getIntField(_T("lParentID"));
	if (parentId > 0)
	{
		clipData += "\r\n";
		clipData += CClipDatabase::FolderPath(parentId);
	}
}

void CQPasteWnd::AppendToolTipShortCut(CppSQLite3Query &q, CString &clipData)
{
	int shortCut = q.getIntField(_T("lShortCut"));
	if (shortCut > 0)
	{
		clipData += "\r\n";
		clipData += CHotKey::GetHotKeyDisplayStatic(shortCut);

		BOOL globalShortCut = q.getIntField(_T("globalShortCut"));
		if (globalShortCut)
		{
			clipData += " - Global Shortcut Key";
		}
	}
}

void CQPasteWnd::AppendToolTipSticky(CppSQLite3Query &q, CString &clipData)
{
	if (theApp.m_GroupID > 0)
	{
		int sticky = q.getIntField(_T("stickyClipGroupOrder"));
		if (sticky != CClip::InvalidSticky)
		{
			clipData += "\r\n";
			clipData += _T(" - Sticky In Group");
		}
	}
	else
	{
		int sticky = q.getIntField(_T("stickyClipOrder"));
		if (sticky != CClip::InvalidSticky)
		{
			clipData += "\r\n";
			clipData += _T(" - Sticky");
		}
	}
}

CString CQPasteWnd::ToolTipClipLines(const CString& clipText) const
{
	CString cs{};
	int lines{0};
	int maxLines{Settings().GetMaxToolTipLines()};
	CTokenizer tokenizer{clipText, "\r\n"};
	CString token{};
	while (tokenizer.Next(token))
	{
		cs += token + "\r\n";
		if (lines > maxLines)
			break;
		lines++;
	}

	return cs;
}

void CQPasteWnd::OnFindItem(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVFINDITEM* pFindInfo = (NMLVFINDITEM*)pNMHDR;
	LVFINDINFO fndItem = pFindInfo->lvfi;


	if (fndItem.flags & LVFI_STRING)
	{
		//m_search.SetWindowText(fndItem.psz);
		//m_search.SetFocus();
		//m_search.SetSel(1, 1);

		//OnSearchEditChange();

		//*pResult = m_lstHeader.GetCaret();
		//return;
	}

	*pResult = -1; // Default action.
}

void CQPasteWnd::OnNcLButtonDblClk(UINT nHitTest, CPoint point)
{
	// toggle ShowPersistent when we double click the caption
	if (nHitTest == HTCAPTION)
	{
		switch (Settings().m_bDoubleClickingOnCaptionDoes)
		{
		case CGetSetOptions::TogglesAlwaysOnTop:
		{
			theApp.ShowPersistent(!Settings().m_bShowPersistent);
			if (Settings().m_bShowPersistent)
			{
				::SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
			}

			MoveControls();

			UpdateStatus();
		}
		break;
		case CGetSetOptions::TogglesAlwaysShowDescription:
			DoAction(ActionEnums::SHOWDESCRIPTION);
			break;
		case CGetSetOptions::RollsUpWindow:
			MinMaxWindow();
			break;
		}
	}

	CWndEx::OnNcLButtonDblClk(nHitTest, point);
}

void CQPasteWnd::OnShowGroupsTop()
{
	m_lstHeader.HidePopup(true);

	OnShowGroupsBottom();
}

void CQPasteWnd::OnShowGroupsBottom()
{
	m_lstHeader.HidePopup(true);

	if (m_GroupTree.IsWindowVisible())
	{
		m_GroupTree.ShowWindow(SW_HIDE);
		return;
	}

	m_GroupTree.m_bHide = false;
	m_bHideWnd = false;

	CRect crWindow, crList;
	m_lstHeader.GetWindowRect(crList);
	GetWindowRect(crWindow);

	CRect cr(crWindow.left, crWindow.bottom, crWindow.left + crWindow.Width(), crWindow.bottom + 200);

	CMonitorGeometry::EnsureWindowVisible(&cr);

	m_GroupTree.MoveWindow(cr);
	m_GroupTree.m_selectedFolderID = theApp.m_GroupID;
	m_GroupTree.FillTree();
	m_GroupTree.ShowWindow(SW_SHOW);

	m_GroupTree.m_bHide = true;
	m_bHideWnd = true;
}

LRESULT CQPasteWnd::OnGroupTreeMessage(WPARAM wParam, LPARAM /*lParam*/)
{
	m_bHideWnd = false;

	int id = (int)wParam;

	m_GroupTree.ShowWindow(SW_HIDE);

	m_bHandleSearchTextChange = false;
	m_search.SetWindowText(_T(""));
	m_bHandleSearchTextChange = true;

	MoveControls();

	if (id == -1)
	{
		//go back to the main list
		theApp.EnterGroupID(-1);
	}
	else if (id >= 0)
	{
		//Set the app flag so it does a send message to refresh the list
		//We need to do this because we set the list pos to 0 and with Post
		//the list is not filled up yet
		bool bItWas = theApp.m_bAsynchronousRefreshView;
		theApp.m_bAsynchronousRefreshView = false;

		CSpecialPasteOptions pasteOptions;
		OpenID(id, pasteOptions);

		theApp.m_bAsynchronousRefreshView = bItWas;

		m_lstHeader.SetListPos(0);
		m_lstHeader.SetFocus();
	}

	CWnd* p = GetFocus();
	if (p == NULL)
	{
		HideQPasteWindow(false);
	}

	m_bHideWnd = true;

	return TRUE;
}

void CQPasteWnd::OnBackButton()
{
	theApp.EnterGroupID(theApp.m_GroupParentID);
}

void CQPasteWnd::OnMenuSearchDescription()
{
	Settings().SetSearchDescription(!Settings().GetSearchDescription());

	CString csText;
	m_search.GetWindowText(csText);

	if (csText != _T(""))
	{
		FillList(csText);
	}
}

void CQPasteWnd::OnMenuSearchFullText()
{
	Settings().SetSearchFullText(!Settings().GetSearchFullText());

	CString csText;
	m_search.GetWindowText(csText);

	if (csText != _T(""))
	{
		FillList(csText);
	}
}

void CQPasteWnd::OnMenuSearchQuickPaste()
{
	Settings().SetSearchQuickPaste(!Settings().GetSearchQuickPaste());

	CString csText;
	m_search.GetWindowText(csText);

	if (csText != _T(""))
	{
		FillList(csText);
	}
}

void CQPasteWnd::OnMenuShowStarredClips()
{
	m_bShowStarredClips = !m_bShowStarredClips;

	CString csText;
	m_search.GetWindowText(csText);

	FillList(csText);
}

void CQPasteWnd::OnSearchEditChange()
{
	m_search.Invalidate();
	if (Settings().m_bFindAsYouType == FALSE)
	{
		return;
	}

	if (m_bHandleSearchTextChange == false)
	{
		//CLogger::Log(_T("Handle text change is NOT set"));
		return;
	}

	KillTimer(TimerDoSearch);
	SetTimer(TimerDoSearch, 250, NULL);

	return;
}

LRESULT CQPasteWnd::OnUpDown(WPARAM wParam, LPARAM lParam)
{
	MSG msg;
	//Workaround for allow holding down arrow keys while in the search control
	msg.lParam = lParam & (~0x40000000);
	msg.wParam = wParam;
	msg.message = WM_KEYDOWN;
	if (CheckActions(&msg) == false)
	{
		if (m_lstHeader.HandleKeyDown(wParam, lParam) == FALSE)
		{
			m_lstHeader.SendMessage(WM_KEYDOWN, wParam, lParam);
		}
	}

	return TRUE;
}

LRESULT CQPasteWnd::OnToolTipWndInactive(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (!Settings().m_bShowPersistent)
	{
		CWnd* p = GetFocus();
		if (p == NULL)
		{
			HideQPasteWindow(false);
			m_lstHeader.HidePopup(true);
		}
	}
	else
	{
		CWnd* p = GetFocus();
		if (p == NULL)
		{
			m_lstHeader.HidePopup(true);
		}
	}

	return TRUE;
}

void CQPasteWnd::OnUpdateMenuNewgroup(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::NEWGROUP);
}

void CQPasteWnd::OnUpdateMenuNewgroupselection(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::NEWGROUPSELECTION);
}

void CQPasteWnd::OnUpdateMenuAllwaysontop(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::TOGGLESHOWPERSISTANT);
}

void CQPasteWnd::OnUpdateMenuViewfulldescription(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::SHOWDESCRIPTION);
}

void CQPasteWnd::OnUpdateMenuViewgroups(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::SHOWGROUPS);
}

void CQPasteWnd::OnUpdateMenuPasteplaintextonly(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::PASTE_SELECTED_PLAIN_TEXT);
}

void CQPasteWnd::OnUpdateMenuDelete(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::DELETE_SELECTED);
}

void CQPasteWnd::OnUpdateMenuProperties(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::CLIP_PROPERTIES);
}

void CQPasteWnd::OnUpdateMenuEdititem(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::EDITCLIP);
}

void CQPasteWnd::OnUpdateMenuNewclip(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::NEWCLIP);
}

LRESULT CQPasteWnd::OnSetListCount(WPARAM wParam, LPARAM /*lParam*/)
{
	m_noSearchResults = false;

	int x = m_lstHeader.GetScrollPos(SB_HORZ);
	int y = m_lstHeader.GetScrollPos(SB_VERT);
	m_lstHeader.Scroll(CSize(-x, -y));

	m_lstHeader.SetItemCountEx((int)wParam);

	if ((int)wParam == 0 &&
		(m_strSearch != _T("") || m_bShowStarredClips))
	{
		m_noSearchResults = true;
		if (m_bShowStarredClips && m_strSearch == _T(""))
		{
			CString text = theApp.m_Language.GetString("NoStarredClips", "There are no starred clips");
			m_noSearchResultsStatic.SetWindowText(text);
		}
		else
		{
			CString text = theApp.m_Language.GetString("NoSearchResults", "There are no results for");
			m_noSearchResultsStatic.SetWindowText(CStringUtil::Format(_T("%s \"%s\""), text.GetString(), m_strSearch.GetString()));
		}
	}

	SelectFocusID();
	UpdateStatus(false);

	MoveControls();

	return TRUE;
}

LRESULT CQPasteWnd::OnItemDeleted(WPARAM wParam, LPARAM /*lParam*/)
{
	m_lstHeader.OnItemDeleted((int)wParam);
	return TRUE;
}

LRESULT CQPasteWnd::OnRefeshRow(WPARAM wParam, LPARAM lParam)
{
	int clipId = (int)wParam;
	int listPos = (int)lParam;

	int topIndex = m_lstHeader.GetTopIndex();
	int lastIndex = topIndex + m_lstHeader.GetCountPerPage();

	if (listPos >= topIndex && listPos <= lastIndex)
	{
		m_lstHeader.RefreshRow(listPos);
		m_lstHeader.PostEventLoadedCheckDescription(listPos);
	}

	if (clipId == -2)
	{
		m_lstHeader.Invalidate();
		m_lstHeader.RedrawWindow();

		//CLogger::Log(_T("End of first load, showing listbox and loading actual count, then accelerators"));
	}

	return true;
}

void CQPasteWnd::SelectFocusID()
{
	ATL::CCritSecLock csLock(m_CritSection.m_sect);

	bool selectedItem = false;
	int index = 0;
	std::vector<CMainTable>::iterator iter = m_listItems.begin();
	while (iter != m_listItems.end())
	{
		if (iter->m_lID == theApp.m_FocusID)
		{
			m_lstHeader.SetListPos(index);
			selectedItem = true;
			break;
		}
		iter++;
		index++;
	}

	if (selectedItem == false)
	{
		m_lstHeader.EnsureVisible(0, FALSE);
		m_lstHeader.SetListPos(Settings().SelectedIndex());
	}
}

void CQPasteWnd::FillMainTable(CMainTable& table, CppSQLite3Query& q)
{
	table.m_lID = q.getIntField(_T("lID"));
	table.m_Desc = q.fieldValue(_T("mText"));
	table.m_bHasParent = q.getIntField(_T("lParentID")) >= 0;
	table.m_bDontAutoDelete = q.getIntField(_T("lDontAutoDelete")) > 0;
	table.m_bHasShortCut = q.getIntField(_T("lShortCut")) > 0;
	table.m_bIsGroup = q.getIntField(_T("bIsGroup")) > 0;
	table.m_QuickPaste = q.fieldValue(_T("QuickPasteText"));
	table.m_clipOrder = q.getFloatField(_T("clipOrder"));
	table.m_clipGroupOrder = q.getFloatField(_T("clipGroupOrder"));
	table.m_stickyClipOrder = q.getFloatField(_T("stickyClipOrder"));
	table.m_stickyClipGroupOrder = q.getFloatField(_T("stickyClipGroupOrder"));
	table.m_dateCopied = q.getInt64Field(_T("lDate"));
	table.m_datePasted = q.getInt64Field(_T("lastPasteDate"));
}

void CQPasteWnd::OnDestroy()
{
	Settings().SetPastSearchXml(m_search.SavePastSearches());

	CWndEx::OnDestroy();
	m_thread.Stop();
	m_extraDataThread.Stop();
}

void CQPasteWnd::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TimerDoSearch)
	{
		OnDoSearchTimer();
	}
	if (nIDEvent == TimerPasteFromModifier)
	{
		OnPasteFromModifierTimer();
	}
	else if (nIDEvent == TimerErrorMsg)
	{
		KillTimer(TimerErrorMsg);
		m_popupMsg.Hide();
	}
	else if (nIDEvent == TimerDragHideWindow)
	{
		OnDragHideWindowTimer();
	}
	else if (nIDEvent == TimerDoAction)
	{
		KillTimer(TimerDoAction);

		OutputDebugString(CStringUtil::Format(_T("DoActionTimer, cmd: %d"), m_timerAction.Cmd));

		if (m_timerAction.Cmd > 0)
		{
			DoAction(m_timerAction);
		}
		m_timerAction = CAccel();
	}

	CWndEx::OnTimer(nIDEvent);
}

void CQPasteWnd::OnDoSearchTimer()
{
	CLogger::Log(_T("TIMER_DO_SEARCH timer\n"));

	KillTimer(TimerDoSearch);

	CString csText;
	m_search.GetWindowText(csText);

	int nCaretPos = m_lstHeader.GetCaret();
	if (nCaretPos >= 0)
	{
		theApp.m_FocusID = m_lstHeader.GetItemData(nCaretPos);
	}

	FillList(csText);
}

void CQPasteWnd::OnPasteFromModifierTimer()
{
	CLogger::Log(_T("TIMER_PASTE_FROM_MODIFER timer\n"));
	KillTimer(TimerPasteFromModifier);
	if (m_bModifersMoveActive)
	{
		CLogger::Log(_T("Open Selection\n"));
		CSpecialPasteOptions pasteOptions;
		OpenSelection(pasteOptions);
	}
	else
	{
		CLogger::Log(_T("m_bModifersMoveActive set to false\n"));
	}
}

void CQPasteWnd::OnDragHideWindowTimer()
{
	OutputDebugString(_T("drag timer\n"));

	CPoint mouse;
	GetCursorPos(&mouse);

	CRect windowRect;
	this->GetWindowRect(&windowRect);

	if (PtInRect(&windowRect, mouse) == FALSE)
	{
		HideQPasteWindow(false, false);
		KillTimer(TimerDragHideWindow);
	}
}

void CQPasteWnd::OnAddinSelect(UINT idIn)
{
	ARRAY IDs;
	m_lstHeader.GetSelectionItemData(IDs);

	if (IDs.GetCount() > 0)
	{
		int id = IDs[0];
		CClip clip(Settings());
		if (clip.LoadMainTable(id))
		{
			if (clip.LoadFormats(id, false))
			{
				bool bCont = theApp.m_Addins.CallPrePasteFunction(idIn, &clip);
				if (bCont)
				{
					CSpecialPasteOptions pasteOptions;
					pasteOptions.m_pPasteFormats = &clip.m_Formats;
					OpenID(-1, pasteOptions);
				}
			}
		}
	}
}

LRESULT CQPasteWnd::OnSelectAll(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	BOOL ret = FALSE;
	ATL::CCritSecLock csLock(m_CritSection.m_sect);

	if ((int)m_listItems.size() < m_lstHeader.GetItemCount())
	{
		CLogger::Log(_T("All items selected loading all items from the db"));

		CPoint loadItem(0, m_lstHeader.GetItemCount());
		m_loadItems.push_back(loadItem);

		m_thread.FireLoadItems(false);

		ret = TRUE;

		UpdateStatus(false);
	}

	return ret;
}

LRESULT CQPasteWnd::OnShowHideScrollBar(WPARAM wParam, LPARAM /*lParam*/)
{
	if (wParam == 1)
	{
		CLogger::Log(_T("OnShowHideScrollBar Showing ScrollBars"));
		m_showScrollBars = true;
		MoveControls();
	}
	else
	{
		CLogger::Log(_T("OnShowHideScrollBar Hiding ScrollBars"));

		m_showScrollBars = false;
		MoveControls();
	}

	return 1;
}

LRESULT CQPasteWnd::OnUpdateScrollBar(WPARAM wParam, LPARAM /*lParam*/)
{
	// Update modern scrollbar position when list scrolls (only if enabled)
	if (Settings().m_useModernScrollBar)
	{
		if (wParam == TRUE)
		{
			m_modernScrollBar.Show(false);
			m_modernScrollBarHorz.Show(false);
		}
		else
		{
			m_modernScrollBar.UpdateScrollBar();
			m_modernScrollBarHorz.UpdateScrollBar();
		}
	}
	return 0;
}

//HBRUSH CQPasteWnd::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
//{
//	// Call the base class implementation first! Otherwise, it may 
//	// undo what we're trying to accomplish here.
//	HBRUSH hbr = CWnd::OnCtlColor(pDC, pWnd, nCtlColor);
//
//	switch (nCtlColor) 
//	{
//	case CTLCOLOR_STATIC:
//		switch (pWnd->GetDlgCtrlID())
//		{
//			case IdOnTopWarning:
//			{
//				pDC->SetBkMode(TRANSPARENT);
//				pDC->SetBkColor(RGB(0, 0, 255));
//
//				CBrush brush;
//				brush.CreateSolidBrush(COLORREF(RGB(255, 0, 0)));
//				return brush;
//			}
//			break;
//		}
//	}
//
//	return hbr;
//}

//void CQPasteWnd::OnPaint()
//{
//	/*CBrush brush;
//	brush.CreateSolidBrush(COLORREF(RGB(255, 0, 0)));
//
//	CRect clientRect;
//	GetClientRect(clientRect);
//
//	CPaintDC dc(this);
//	dc.FillRect(clientRect, &brush);*/
//
//	
//		CQPasteWnd::OnPaint();
//	
//}

BOOL CQPasteWnd::OnEraseBkgnd(CDC* pDC)
{
	CRect rect;
	GetClientRect(&rect);
	CBrush myBrush(Settings().m_Theme.MainWindowBG());    // dialog background color
	CBrush* pOld = pDC->SelectObject(&myBrush);
	BOOL bRes = pDC->PatBlt(0, 0, rect.Width(), rect.Height(), PATCOPY);
	pDC->SelectObject(pOld);    // restore old brush
	return bRes;                       // CDialog::OnEraseBkgnd(pDC);

	//return TRUE;
	// TODO: Add your message handler code here and/or call default

	//return CWndEx::OnEraseBkgnd(pDC);
}

void CQPasteWnd::OnQuickoptionsShowintaskbar()
{
	DoAction(ActionEnums::SHOW_IN_TASKBAR);
}


void CQPasteWnd::OnMenuViewasqrcode()
{
	DoAction(ActionEnums::EXPORT_TO_QR_CODE);
}

void CQPasteWnd::OnExportExporttotextfile()
{
	DoAction(ActionEnums::EXPORT_TO_TEXT_FILE);
}

void CQPasteWnd::OnCompareCompare()
{
	DoAction(ActionEnums::COMPARE_SELECTED_CLIPS);
}

void CQPasteWnd::OnCompareSelectleftcompare()
{
	DoAction(ActionEnums::SELECT_LEFT_SIDE_COMPARE);
}

void CQPasteWnd::OnCompareCompareagainst()
{
	DoAction(ActionEnums::SELECT_RIGHT_SITE_AND_DO_COMPARE);
}

void CQPasteWnd::OnUpdateCompareCompare(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::COMPARE_SELECTED_CLIPS);
}

void CQPasteWnd::UpdateMenuShortCut(CCmdUI* pCmdUI, DWORD action)
{
	if (pCmdUI == NULL ||
		pCmdUI->m_pMenu == NULL)
	{
		return;
	}

	CString cs;
	pCmdUI->m_pMenu->GetMenuString(pCmdUI->m_nID, cs, MF_BYCOMMAND);
	CString shortcutText = m_actions.GetCmdKeyText(action);
	if (shortcutText != _T("") &&
		cs.Find("\t" + shortcutText) < 0)
	{
		cs += "\t";
		cs += shortcutText;
		pCmdUI->SetText(cs);
	}
}

LRESULT CQPasteWnd::OnShowProperties(WPARAM wParam, LPARAM /*lParam*/)
{
	return ShowProperties((int)wParam, -1);
}

LRESULT CQPasteWnd::OnNewGroup(WPARAM wParam, LPARAM /*lParam*/)
{
	NewGroup(false, (int)wParam);

	return TRUE;
}

LRESULT CQPasteWnd::OnDeleteId(WPARAM wParam, LPARAM /*lParam*/)
{
	if (Settings().GetPromptWhenDeletingClips())
	{
		bool bStartValue = m_bHideWnd;
		m_bHideWnd = false;

		int nRet = MessageBox(theApp.m_Language.GetString("Delete_Clip_Groups", "Delete Group?"), _T("Ditto"), MB_OKCANCEL | MB_TOPMOST);

		m_bHideWnd = bStartValue;

		if (nRet != IDOK)
		{
			return FALSE;
		}
	}

	CClipIDs IDs;
	ARRAY Indexs;

	IDs.Add((int)wParam);

	int index = 0;
	{
		ATL::CCritSecLock csLock(m_CritSection.m_sect);
		std::vector<CMainTable>::iterator iter = m_listItems.begin();
		while (iter != m_listItems.end())
		{
			if (iter->m_lID == static_cast<int>(wParam))
			{
				Indexs.Add(index);
				break;
			}
			iter++;
			index++;
		}
	}

	DeleteClips(IDs, Indexs);

	return TRUE;
}

void CQPasteWnd::OnMenuSimpleTextSearch()
{
	Settings().SetSimpleTextSearch(!Settings().GetSimpleTextSearch());
	Settings().SetRegExTextSearch(FALSE);
}

void CQPasteWnd::OnMenuRegularexpressionsearch()
{
	Settings().SetSimpleTextSearch(FALSE);
	Settings().SetRegExTextSearch(!Settings().GetRegExTextSearch());
}



void CQPasteWnd::OnImportExportclipBitmap()
{
	DoAction(ActionEnums::EXPORT_TO_BITMAP_FILE);
}


void CQPasteWnd::OnUpdateImportExportclipBitmap(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::EXPORT_TO_BITMAP_FILE);
}

void CQPasteWnd::OnMenuWildcardsearch()
{
	Settings().SetSimpleTextSearch(FALSE);
	Settings().SetRegExTextSearch(FALSE);
}

void CQPasteWnd::OnMenuSavecurrentclipboard()
{
	DoAction(ActionEnums::SAVE_CURRENT_CLIPBOARD);
}


void CQPasteWnd::OnUpdateMenuSavecurrentclipboard(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::SAVE_CURRENT_CLIPBOARD);
}


bool CQPasteWnd::SyncClipDataToArrayData(CClip& clip)
{
	int row = 0;
	bool found = false;
	std::vector<CMainTable>::iterator iter = m_listItems.begin();
	while (iter != m_listItems.end())
	{
		if (iter->m_lID == clip.ID())
		{
			iter->m_clipOrder = clip.m_clipOrder;
			iter->m_clipGroupOrder = clip.m_clipGroupOrder;

			iter->m_stickyClipOrder = clip.m_stickyClipOrder;
			iter->m_stickyClipGroupOrder = clip.m_stickyClipGroupOrder;

			found = true;
			break;
		}
		iter++;
		row++;
	}

	return found;
}

bool CQPasteWnd::SelectIds(ARRAY& ids)
{
	int row = 0;
	bool found = false;
	std::vector<CMainTable>::iterator iter = m_listItems.begin();

	//sort so .Find works
	ids.SortAscending();

	while (iter != m_listItems.end())
	{
		if (ids.Find(iter->m_lID))
		{
			if (found == false)
			{
				m_lstHeader.SetListPos(row);
			}
			else
			{
				m_lstHeader.SetSelection(row);
			}

			found = true;
		}
		iter++;
		row++;
	}

	return found;
}

void CQPasteWnd::OnCliporderMoveup()
{
	DoAction(ActionEnums::MOVE_CLIP_UP);
}

void CQPasteWnd::OnUpdateCliporderMoveup(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::MOVE_CLIP_UP);
}

void CQPasteWnd::OnCliporderMovedown()
{
	DoAction(ActionEnums::MOVE_CLIP_DOWN);
}

void CQPasteWnd::OnUpdateCliporderMovedown(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::MOVE_CLIP_DOWN);
}

void CQPasteWnd::OnCliporderMovetotop()
{
	DoAction(ActionEnums::MOVE_CLIP_TOP);
}

void CQPasteWnd::OnUpdateCliporderMovetotop(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::MOVE_CLIP_TOP);
}

void CQPasteWnd::OnMenuFilteron()
{
	DoAction(ActionEnums::FILTER_ON_SELECTED_CLIP);
}

void CQPasteWnd::OnUpdateMenuFilteron(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::FILTER_ON_SELECTED_CLIP);
}

void CQPasteWnd::OnMenuGoToEntry()
{
	ARRAY IDs;
	m_lstHeader.GetSelectionItemData(IDs);
	if (IDs.GetSize() <= 0)
	{
		return;
	}

	long targetID = IDs[0];

	GoToEntryKey key{};
	bool gotKey = LoadGoToEntryKey(targetID, key);

	if (!gotKey)
	{
		return;
	}

	CString filter = MainListFilter();

	int targetIndex = GoToEntryRank(filter, key);

	theApp.m_FocusID = targetID;

	if (theApp.m_GroupID >= 0 && key.parent != theApp.m_GroupID)
	{
		theApp.EnterGroupID(-1);
	}

	m_bHandleSearchTextChange = false;
	m_search.SetWindowText(_T(""));
	m_bHandleSearchTextChange = true;

	FillList(_T(""));

	WaitForListLoad();

	int totalRows = m_lstHeader.GetItemCount();

	if (targetIndex < 0 || targetIndex >= totalRows)
	{
		MoveControls();
		SelectFocusID();
		m_lstHeader.SetFocus();
		return;
	}

	MoveControls();

	m_lstHeader.EnsureVisible(targetIndex, FALSE);
	m_lstHeader.SetListPos(targetIndex);
	m_lstHeader.SetFocus();

	CLogger::Log(CStringUtil::Format(_T("GoToEntry: scrolled to index %d of %d"), targetIndex, totalRows));
}

bool CQPasteWnd::LoadGoToEntryKey(long targetID, GoToEntryKey &key)
{
	bool gotKey = false;
	try
	{
		CppSQLite3Query q = theApp.m_db.execQueryEx(
			_T("SELECT stickyClipOrder, bIsGroup, clipOrder, lParentID FROM Main WHERE lID = %d"),
			targetID);
		if (!q.eof())
		{
			key.sticky = q.getIntField(_T("stickyClipOrder"));
			key.isGroup = q.getIntField(_T("bIsGroup"));
			key.clipOrder = q.getIntField(_T("clipOrder"));
			key.parent = q.getIntField(_T("lParentID"));
			gotKey = true;
		}
	}
	catch (CppSQLite3Exception&)
	{
	}

	return gotKey;
}

int CQPasteWnd::GoToEntryRank(const CString &filter, const GoToEntryKey &key)
{
	int targetIndex = -1;
	try
	{
		CString rankSql;
		rankSql.Format(
			_T("SELECT COUNT(*) FROM Main WHERE %s AND (")
			_T("(stickyClipOrder > %d) OR ")
			_T("(stickyClipOrder = %d AND bIsGroup < %d) OR ")
			_T("(stickyClipOrder = %d AND bIsGroup = %d AND clipOrder > %d))"),
			(LPCTSTR)filter,
			key.sticky,
			key.sticky, key.isGroup,
			key.sticky, key.isGroup, key.clipOrder);

		targetIndex = theApp.m_db.execScalar(rankSql);
	}
	catch (CppSQLite3Exception&)
	{
		targetIndex = -1;
	}

	return targetIndex;
}

void CQPasteWnd::WaitForListLoad()
{
	// The list loader thread reports completion via PostMessage(CQListCtrl::NmSetListCount),
	// so we must pump messages while waiting or that handler never runs.
	ULONGLONG waitStart = GetTickCount64();
	while (WaitForSingleObject(m_thread.m_SearchingEvent, 0) == WAIT_TIMEOUT)
	{
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (GetTickCount64() - waitStart > 5000)
			break;
		Sleep(10);
	}

	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void CQPasteWnd::OnUpdateMenuGoToEntry(CCmdUI* pCmdUI)
{
	CString csSearch;
	m_search.GetWindowText(csSearch);
	pCmdUI->Enable(!csSearch.IsEmpty());
}

void CQPasteWnd::OnAlwaysOnTopClicked()
{
	DoAction(ActionEnums::TOGGLESHOWPERSISTANT);
}


void CQPasteWnd::OnSpecialpasteUppercase()
{
	DoAction(ActionEnums::PASTE_UPPER_CASE);
}


void CQPasteWnd::OnUpdateSpecialpasteUppercase(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::PASTE_UPPER_CASE);
}


void CQPasteWnd::OnSpecialpasteLowercase()
{
	DoAction(ActionEnums::PASTE_LOWER_CASE);
}


void CQPasteWnd::OnUpdateSpecialpasteLowercase(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::PASTE_LOWER_CASE);
}


void CQPasteWnd::OnSpecialpasteCapitalize()
{
	DoAction(ActionEnums::PASTE_CAPITALiZE);
}

void CQPasteWnd::OnUpdateSpecialpasteCapitalize(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::PASTE_CAPITALiZE);
}


void CQPasteWnd::OnSpecialpasteSentence()
{
	DoAction(ActionEnums::PASTE_SENTENCE_CASE);
}


void CQPasteWnd::OnUpdateSpecialpasteSentence(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::PASTE_SENTENCE_CASE);
}

void CQPasteWnd::OnSystemButton()
{
	m_lstHeader.HidePopup(true);

	POINT pp;
	CMenu cmPopUp;
	CMenu* cmSubMenu = NULL;

	GetCursorPos(&pp);
	if (cmPopUp.LoadMenu(IDR_QUICK_PASTE_SYSTEM_MENU) != 0)
	{
		cmSubMenu = cmPopUp.GetSubMenu(0);
		if (!cmSubMenu)
		{
			return;
		}

		if (Settings().GetShowStartupMessage())
		{
			cmSubMenu->CheckMenuItem(ID_FIRST_SHOWSTARTUPMESSAGE, MF_CHECKED);
		}

		AddShowStarredClipsMenuItem(cmSubMenu);

		theApp.m_Language.UpdateRightClickMenu(cmSubMenu);

		SetMenuChecks(cmSubMenu);

		cmSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, pp.x, pp.y, this, NULL);
	}
}

void CQPasteWnd::OnSpecialpasteRemovelinefeeds()
{
	DoAction(ActionEnums::PASTE_REMOVE_LINE_FEEDS);
}


void CQPasteWnd::OnUpdateSpecialpasteRemovelinefeeds(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::PASTE_REMOVE_LINE_FEEDS);
}


void CQPasteWnd::OnSpecialpastePaste()
{
	DoAction(ActionEnums::PASTE_ADD_ONE_LINE_FEED);
}


void CQPasteWnd::OnUpdateSpecialpastePaste(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::PASTE_ADD_ONE_LINE_FEED);
}


void CQPasteWnd::OnSpecialpastePaste32919()
{
	DoAction(ActionEnums::PASTE_ADD_TWO_LINE_FEEDS);
}

void CQPasteWnd::OnUpdateSpecialpastePaste32919(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::PASTE_ADD_TWO_LINE_FEEDS);
}


void CQPasteWnd::OnSpecialpasteTypoglycemia()
{
	DoAction(ActionEnums::PASTE_TYPOGLYCEMIA);
}

void CQPasteWnd::OnUpdateSpecialpasteTypoglycemia(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::PASTE_TYPOGLYCEMIA);
}

void CQPasteWnd::OnNMClickList1(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	CString csText;
	m_search.GetWindowText(csText);
	if (csText == _T("crash"))
	{
		if (CKeyboard::IsControlPressed())
		{
			if (GetKeyState(VK_SHIFT) & 0x8000)
			{
				raise(SIGSEGV);
			}
		}
	}

	MSG msg;
	msg.lParam = 0;
	msg.wParam = CMouseKey::Click;
	msg.message = WM_KEYDOWN;
	if (CheckActions(&msg) == false)
	{
	}
	*pResult = 0;
}


void CQPasteWnd::OnNMDblclkList1(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	MSG msg;
	msg.lParam = 0;
	msg.wParam = CMouseKey::DoubleClick;
	msg.message = WM_KEYDOWN;
	if (CheckActions(&msg) == false)
	{
	}

	*pResult = 0;
}


void CQPasteWnd::OnNMRClickList1(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	MSG msg;
	msg.lParam = 0;
	msg.wParam = CMouseKey::RightClick;
	msg.message = WM_KEYDOWN;
	if (CheckActions(&msg) == false)
	{
	}
	*pResult = 0;
}

void CQPasteWnd::OnNMRDblclkList1(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	/*MSG msg;
	msg.lParam = 0;
	msg.wParam = CMouseKey::RightClick;
	msg.message = WM_KEYDOWN;
	if (CheckActions(&msg) == false)
	{
	}*/
	*pResult = 0;
}

void CQPasteWnd::OnQuickoptionsShowtextforfirsttencopyhotkeys()
{
	DoAction(ActionEnums::CONFIG_SHOW_FIRST_TEN_TEXT);
}


void CQPasteWnd::OnUpdateQuickoptionsShowtextforfirsttencopyhotkeys(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::CONFIG_SHOW_FIRST_TEN_TEXT);
}


void CQPasteWnd::OnQuickoptionsShowindicatoracliphasbeenpasted()
{
	DoAction(ActionEnums::CONFIG_SHOW_CLIP_WAS_PASTED);
}

void CQPasteWnd::OnUpdateQuickoptionsShowindicatoracliphasbeenpasted(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::CONFIG_SHOW_CLIP_WAS_PASTED);
}


void CQPasteWnd::OnGroupsTogglelastgroup()
{
	DoAction(ActionEnums::TOGGLE_LAST_GROUP_TOGGLE);
}


void CQPasteWnd::OnUpdateGroupsTogglelastgroup(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::TOGGLE_LAST_GROUP_TOGGLE);
}


void CQPasteWnd::OnSpecialpastePaste32927()
{
	DoAction(ActionEnums::PASTE_ADD_CURRENT_TIME);
}


void CQPasteWnd::OnUpdateSpecialpastePaste32927(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::PASTE_ADD_CURRENT_TIME);
}


void CQPasteWnd::OnMenuGlobalhotkeys32933()
{
	DoAction(ActionEnums::GLOBAl_HOTKEYS);
}

void CQPasteWnd::OnUpdateMenuGlobalhotkeys32933(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::GLOBAl_HOTKEYS);
}


void CQPasteWnd::OnMenuDeleteclipdata32934()
{
	DoAction(ActionEnums::DELETE_CLIP_DATA);
}



void CQPasteWnd::OnUpdateMenuDeleteclipdata32934(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::DELETE_CLIP_DATA);
}



void CQPasteWnd::OnMenuImportclip32935()
{
	DoAction(ActionEnums::IMPORT_CLIP);
}

void CQPasteWnd::OnUpdateMenuImportclip32935(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::IMPORT_CLIP);
}

void CQPasteWnd::OnMenuNewclip32937()
{
	DoAction(ActionEnums::NEWCLIP);
}

void CQPasteWnd::OnUpdateMenuNewclip32937(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::NEWCLIP);
}

LRESULT CQPasteWnd::OnSearchFocused(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_lstHeader.HidePopup(true);

	return TRUE;
}

#include <ShellScalingApi.h>


LRESULT CQPasteWnd::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	ARRAY Indexs;
	m_lstHeader.GetSelectionIndexes(Indexs);
	if (Indexs.GetCount() <= 0)
	{
		Indexs.Add(0);
	}
	int c = m_lstHeader.GetItemCount();
	m_lstHeader.SetItemCountEx(0);

	int dpi = HIWORD(wParam);
	m_DittoWindow.OnDpiChanged(this, dpi);

	//RECT* const prcNewWindow = (RECT*)lParam;
	CRect r(*(RECT*)lParam);
	if (Settings().m_bEnsureEntireWindowCanBeSeen)
	{
		CMonitorGeometry::EnsureWindowVisible(&r);
	}

	SetWindowPos(NULL, r.left, r.top, r.Width(), r.Height(), SWP_NOZORDER | SWP_NOACTIVATE);

	CLogger::Write(CStringUtil::Format(_T("CQPasteWnd::OnDpiChanged dpi: %d width: %d, height: %d"), dpi, r.Width(), r.Height()));

	m_systemMenu.Reset();
	m_systemMenu.LoadStdImageDPI(m_DittoWindow.m_dpi.GetDPI(), system_menu_2_24, system_menu_2_30, system_menu_2_36, system_menu_2_42, system_menu_2_48, _T("PNG"), system_menu_54, system_menu_60, system_menu_66, system_menu_72, system_menu_78, system_menu_84);

	m_BackButton.Reset();
	m_BackButton.LoadStdImageDPI(m_DittoWindow.m_dpi.GetDPI(), return_16, return_20, return_24, return_28, return_32, _T("PNG"));

	m_ShowGroupsFolderBottom.Reset();
	m_ShowGroupsFolderBottom.LoadStdImageDPI(m_DittoWindow.m_dpi.GetDPI(), open_folder_24, open_folder_30, open_folder_36, open_folder_42, open_folder_48, _T("PNG"), open_folder_54, open_folder_60, open_folder_66, open_folder_72, open_folder_78, open_folder_84);

	m_search.OnDpiChanged();
	m_lstHeader.OnDpiChanged();

	UpdateFont();
	this->SetLinesPerRow(Settings().GetLinesPerRow(), true, false);

	MoveControls();

	m_lstHeader.SetItemCountEx(c);
	m_lstHeader.SetListPos(Indexs[0]);

	InvalidateNc();
	this->Invalidate();
	m_lstHeader.RefreshVisibleRows();
	this->RedrawWindow();

	return TRUE;
}

void CQPasteWnd::OnCliporderReplacetopstickyclip()
{
	DoAction(ActionEnums::REPLACE_TOP_STICKY_CLIP);
}

void CQPasteWnd::OnUpdateCliporderReplacetopstickyclip(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::REPLACE_TOP_STICKY_CLIP);
}



void CQPasteWnd::OnImportImportcopiedfile()
{
	DoAction(ActionEnums::SAVE_CF_HDROP_FIlE_DATA);
}

void CQPasteWnd::OnUpdateImportImportcopiedfile(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::SAVE_CF_HDROP_FIlE_DATA);
}

void CQPasteWnd::OnCliporderMovetolast()
{
	DoAction(ActionEnums::MOVE_CLIP_LAST);
}


void CQPasteWnd::OnUpdateCliporderMovetolast(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::MOVE_CLIP_LAST);
}


void CQPasteWnd::OnSpecialpastePasteDontUpdateOrder()
{
	DoAction(ActionEnums::PASTE_DONT_MOVE_CLIP);
}


void CQPasteWnd::OnUpdateOnSpecialPasteDontUpdateOrder(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::PASTE_DONT_MOVE_CLIP);
}


void CQPasteWnd::OnSpecialpasteTrim()
{
	DoAction(ActionEnums::PASTE_TRIM_WHITE_SPACE);
}


void CQPasteWnd::OnSpecialpastePosixifyPaths()
{
	DoAction(ActionEnums::PASTE_POSIXIFY_PATHS);
}


void CQPasteWnd::OnUpdateSpecialpasteTrim(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::PASTE_TRIM_WHITE_SPACE);
}

void CQPasteWnd::OnUpdateSpecialPosixifyPaths(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::PASTE_POSIXIFY_PATHS);
}

void CQPasteWnd::OnMenuTransparencyNone()
{
	DoAction(ActionEnums::TRANSPARENCY_NONE);
}

void CQPasteWnd::OnUpdateTransparencyNone(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::TRANSPARENCY_NONE);
}

void CQPasteWnd::OnMenuTransparency5()
{
	DoAction(ActionEnums::TRANSPARENCY_5);
}

void CQPasteWnd::OnUpdateTransparency5(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::TRANSPARENCY_5);
}

void CQPasteWnd::OnMenuTransparency10()
{
	DoAction(ActionEnums::TRANSPARENCY_10);
}

void CQPasteWnd::OnUpdateTransparency10(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::TRANSPARENCY_10);
}

void CQPasteWnd::OnMenuTransparency15()
{
	DoAction(ActionEnums::TRANSPARENCY_15);
}

void CQPasteWnd::OnUpdateTransparency15(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::TRANSPARENCY_15);
}

void CQPasteWnd::OnMenuTransparency20()
{
	DoAction(ActionEnums::TRANSPARENCY_20);
}

void CQPasteWnd::OnUpdateTransparency20(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::TRANSPARENCY_20);
}

void CQPasteWnd::OnMenuTransparency25()
{
	DoAction(ActionEnums::TRANSPARENCY_25);
}

void CQPasteWnd::OnUpdateTransparency25(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::TRANSPARENCY_25);
}

void CQPasteWnd::OnMenuTransparency30()
{
	DoAction(ActionEnums::TRANSPARENCY_30);
}

void CQPasteWnd::OnUpdateTransparency30(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::TRANSPARENCY_30);
}

void CQPasteWnd::OnTransparency35()
{
	DoAction(ActionEnums::TRANSPARENCY_35);
}

void CQPasteWnd::OnUpdateTransparency35(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::TRANSPARENCY_35);
}

void CQPasteWnd::OnMenuTransparency40()
{
	DoAction(ActionEnums::TRANSPARENCY_40);
}

void CQPasteWnd::OnUpdateTransparency40(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::TRANSPARENCY_40);
}

bool CQPasteWnd::DoActionToggleTransparency()
{
	Settings().SetEnableTransparency(!Settings().GetEnableTransparency());
	SetCurrentTransparency();

	return true;
}

bool CQPasteWnd::DoActionIncreaseTransparency()
{
	int current = Settings().GetTransparencyPercent();
	current += 5;
	current = min(current, 100);
	SetTransparency(current);

	return true;
}

bool CQPasteWnd::DoActionDecreaseTransparency()
{
	int current = Settings().GetTransparencyPercent();
	current -= 5;
	current = max(current, 0);
	SetTransparency(current);

	return true;
}

void CQPasteWnd::RefreshScrollBarColors()
{
	m_modernScrollBar.SetColors(
		Settings().m_Theme.ScrollBarTrack(),
		Settings().m_Theme.ScrollBarThumb(),
		Settings().m_Theme.ScrollBarThumbHover()
	);
	m_modernScrollBarHorz.SetColors(
		Settings().m_Theme.ScrollBarTrack(),
		Settings().m_Theme.ScrollBarThumb(),
		Settings().m_Theme.ScrollBarThumbHover()
	);
}

void CQPasteWnd::RefreshThemeColors()
{
	// Refresh caption bar colors
	SetCaptionColorActive(Settings().m_bShowPersistent, theApp.GetConnectCV());
	SetCaptionOn(Settings().GetCaptionPos(), true, Settings().m_Theme.GetCaptionSize(), Settings().m_Theme.GetCaptionFontSize());
	
	// Refresh scrollbar colors
	RefreshScrollBarColors();
	
	// Force repaint of the entire window including non-client area
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_FRAME);
}

bool CQPasteWnd::DoActionSlugify()
{
	if (::GetFocus() == m_lstHeader.GetSafeHwnd())
	{
		CSpecialPasteOptions pasteOptions;
		pasteOptions.m_pasteSlugify = true;
		OpenSelection(pasteOptions);
		return true;
	}

	return false;
}

bool CQPasteWnd::DoRefreshList()
{
	theApp.m_FocusID = -1;

	CString csText;
	m_search.GetWindowText(csText);

	FillList(csText);

	return true;
}

bool CQPasteWnd::DoDeleteAllNonUsedClips()
{
	bool bStartValue = m_bHideWnd;
	m_bHideWnd = false;

	int nRet = MessageBox(theApp.m_Language.GetString("Delete_All_Non_Used_Clips", "Delete all clips that are not groups, in groups, marked as never auto delete, has a shortcut key or marked as sticky.\r\n\r\nThis cannot be undone."), _T("Ditto"), MB_OKCANCEL | MB_TOPMOST);

	m_bHideWnd = bStartValue;
	if (nRet != IDOK)
	{
		return false;
	}

	CWaitCursor wait;

	CClipRetentionPolicy::DeleteNonUsedClips(true);
	FillList();

	m_cf_dibCache.clear();
	m_cf_NO_dibCache.clear();
	m_cf_rtfCache.clear();
	m_cf_NO_rtfCache.clear();

	return true;
}

bool CQPasteWnd::DoCopySelection()
{
	ARRAY IDs;
	m_lstHeader.GetSelectionItemData(IDs);

	INT_PTR count = IDs.GetSize();

	if (count <= 0)
	{
		return FALSE;
	}

	CProcessPaste paste(Settings());

	//Don't send the paste just load it into memory
	paste.m_bSendPaste = false;

	if (count > 1)
		paste.GetClipIDs().Copy(IDs);
	else
		paste.GetClipIDs().Add(IDs[0]);

	//Don't move these to the top
	BOOL itWas = Settings().m_bUpdateTimeOnPaste;
	Settings().m_bUpdateTimeOnPaste = Settings().GetUpdateClipOrderOnCtrlC();

	paste.DoPaste();

	Settings().m_bUpdateTimeOnPaste = itWas;

	return TRUE;
}

void CQPasteWnd::SetTransparency(int percent)
{
	if (percent > 0)
	{
		Settings().SetTransparencyPercent(percent);
		Settings().SetEnableTransparency(TRUE);

		m_Alpha.SetTransparent(TRUE);

		float fPercent = percent / (float)100.0;

		m_Alpha.SetOpacity(CAlphaBlend::OpacityMax - (int)(fPercent * CAlphaBlend::OpacityMax));
	}
	else
	{
		Settings().SetEnableTransparency(FALSE);
		m_Alpha.SetTransparent(FALSE);
	}
}

void CQPasteWnd::SetCurrentTransparency()
{
	//Set the transparency
	if (Settings().GetEnableTransparency())
	{
		m_Alpha.SetTransparent(TRUE);

		float fPercent = Settings().GetTransparencyPercent() / (float)100.0;

		m_Alpha.SetOpacity(CAlphaBlend::OpacityMax - (int)(fPercent * CAlphaBlend::OpacityMax));
	}
	else
	{
		m_Alpha.SetTransparent(FALSE);
	}
}

void CQPasteWnd::OnTransparencyIncrease()
{
	DoAction(ActionEnums::TRANSPARENCY_INCREASE);
}


void CQPasteWnd::OnUpdateTransparencyIncrease(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::TRANSPARENCY_INCREASE);
}


void CQPasteWnd::OnTransparencyDecrease()
{
	DoAction(ActionEnums::TRANSPARENCY_DECREASE);
}


void CQPasteWnd::OnUpdateTransparencyDecrease(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::TRANSPARENCY_DECREASE);
}


void CQPasteWnd::OnTransparencyToggle()
{
	DoAction(ActionEnums::TRANSPARENCY_TOGGLE);
}


void CQPasteWnd::OnUpdateTransparencyToggle(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::TRANSPARENCY_TOGGLE);
}






void CQPasteWnd::OnSpecialpasteSlugify()
{
	DoAction(ActionEnums::SLUGIFY);
}

void CQPasteWnd::OnUpdateSpecialpasteSlugify(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::SLUGIFY);
}




void CQPasteWnd::OnSpecialpasteTogglecase()
{
	DoAction(ActionEnums::INVERT_CASE);
}


void CQPasteWnd::OnUpdateSpecialpasteTogglecase(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::INVERT_CASE);
}


void CQPasteWnd::OnFirstShowstartupmessage()
{
	BOOL existing = Settings().GetShowStartupMessage();
	Settings().SetShowStartupMessage(!existing);
}


void CQPasteWnd::OnFirstRestoreDb()
{
	theApp.m_pMainFrame->PostMessage(CDittoMessage::RestoreDb, 0, 0);
}

void CQPasteWnd::OnFirstBackupDb()
{
	theApp.m_pMainFrame->PostMessage(CDittoMessage::BackupDb, 0, 0);
}

void CQPasteWnd::OnMenuDeleteallnonusedclips()
{
	DoAction(ActionEnums::DELETE_ALL_NON_USED_CLIPS);
}

void CQPasteWnd::OnUpdateMenuDeleteallnonusedclips(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::DELETE_ALL_NON_USED_CLIPS);
}

void CQPasteWnd::OnImportSetdragfilename()
{
	DoAction(ActionEnums::SET_DRAG_FILE_NAME);
}

void CQPasteWnd::OnUpdateImportSetdragfilename(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::SET_DRAG_FILE_NAME);
}

void CQPasteWnd::OnSpecialpasteCamelcase()
{
	DoAction(ActionEnums::PASTE_CAMEL_CASE);
}

void CQPasteWnd::OnUpdateSpecialpasteCamelcase(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::PASTE_CAMEL_CASE);
}

void CQPasteWnd::OnSpecialpasteMultipleImagesHorz()
{
	DoAction(ActionEnums::PASTE_MULTI_IMAGE_HORIZONTAL);
}

void CQPasteWnd::OnUpdateSpecialpasteMultipleImagesHorz(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::PASTE_MULTI_IMAGE_HORIZONTAL);
}

void CQPasteWnd::OnSpecialpasteMultipleImagesVert()
{
	DoAction(ActionEnums::PASTE_MULTI_IMAGE_VERTICAL);
}

void CQPasteWnd::OnUpdateSpecialpasteMultipleImagesVert(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::PASTE_MULTI_IMAGE_VERTICAL);
}

void CQPasteWnd::OnSpecialpasteAsciitextonly()
{
	DoAction(ActionEnums::ASCII_TEXT_ONLY);
}


void CQPasteWnd::OnUpdateSpecialpasteAsciitextonly(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::ASCII_TEXT_ONLY);
}

void CQPasteWnd::OnSpecialpastePastenewguid()
{
	DoAction(ActionEnums::GENERATE_GUID);
}

void CQPasteWnd::OnUpdateSpecialpastePastenewguid(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::GENERATE_GUID);
}

void CQPasteWnd::OnSpecialpastePasteAsImage()
{
	DoAction(ActionEnums::PASTE_AS_IMAGE);
}

void CQPasteWnd::OnUpdateSpecialpastePasteAsImage(CCmdUI* pCmdUI)
{
	if (!pCmdUI->m_pMenu)
	{
		return;
	}

	UpdateMenuShortCut(pCmdUI, ActionEnums::PASTE_AS_IMAGE);
}
