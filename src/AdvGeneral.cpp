// AdvGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "CP_Main.h"
#include "AdvGeneral.h"
#include "afxdialogex.h"
#include "DimWnd.h"
#include "MoveToGroupDlg.h"
#include "SQlite/CppSQLite3.h"
#include "ErrorReport.h"

IMPLEMENT_DYNAMIC(CAdvGeneral, CDialogEx)

CAdvGeneral::CAdvGeneral(CWnd* pParent /*=NULL*/)
	: CDialogEx(CAdvGeneral::IDD, pParent)
{

}

CAdvGeneral::~CAdvGeneral()
{
}

CGetSetOptions& CAdvGeneral::Settings() const
{
	return theApp.Services().Settings();
}

void CAdvGeneral::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCPROPERTYGRID1, m_propertyGrid);
	DDX_Control(pDX, IDC_EDIT_ADV_FILTER, m_editFilter);
}


BEGIN_MESSAGE_MAP(CAdvGeneral, CDialogEx)
	ON_BN_CLICKED(IDOK, &CAdvGeneral::OnBnClickedOk)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BT_COMPACT_AND_REPAIR, &CAdvGeneral::OnBnClickedBtCompactAndRepair)
	ON_WM_GETMINMAXINFO()
	ON_WM_NCLBUTTONDOWN()
	ON_EN_CHANGE(IDC_EDIT_ADV_FILTER, &CAdvGeneral::OnEnChangeAdvFilter)
	ON_BN_CLICKED(IDC_BUTTON_NEXT_MATCH, &CAdvGeneral::OnBnClickedButtonNextMatch)
	ON_BN_CLICKED(IDC_BUTTON_COPY_SCRIPTS2, &CAdvGeneral::OnBnClickedButtonCopyScripts2)
END_MESSAGE_MAP()


// CAdvGeneral message handlers

const std::array<CAdvGeneral::LongSetting, 20> CAdvGeneral::s_longSettings{ {
	{ SettingDescSize, [](CGetSetOptions& settings, long value) { settings.SetDescTextSize(value); } },
	{ SettingSelectedIndex, [](CGetSetOptions& settings, long value) { settings.SetSelectedIndex(max((value - 1), 0)); } },
	{ SettingClipboardSaveDelay, [](CGetSetOptions& settings, long value) { settings.SetProcessDrawClipboardDelay(max(value, 0)); } },
	{ SettingMaxClipSize, [](CGetSetOptions& settings, long value) { settings.SetMaxClipSizeInBytes(value); } },
	{ SettingLinesPerRow, [](CGetSetOptions& settings, long value) { settings.SetLinesPerRow(value); } },
	{ SettingTransparency, &CAdvGeneral::WriteTransparencyPercent },
	{ SettingTooltipTimeout, [](CGetSetOptions& settings, long value) { settings.SetToolTipTimeout(value); } },
	{ SettingTooltipLines, [](CGetSetOptions& settings, long value) { settings.SetMaxToolTipLines(value); } },
	{ SettingTooltipCharacters, [](CGetSetOptions& settings, long value) { settings.SetMaxToolTipCharacters(value); } },
	{ SettingActivateWindowDelay, [](CGetSetOptions& settings, long value) { settings.SetSendKeysDelay(value); } },
	{ SettingSendKeysDelay, [](CGetSetOptions& settings, long value) { settings.SetRealSendKeysDelay(value); } },
	{ SettingClipboardRestoreAfterCopyBufferDelay, [](CGetSetOptions& settings, long value) { settings.SetDittoRestoreClipboardDelay(value); } },
	{ SettingDoubleKeystrokeTimeout, [](CGetSetOptions& settings, long value) { settings.SetDoubleKeyStrokeTimeout(value); } },
	{ SettingFirstTenHotkeysStart, [](CGetSetOptions& settings, long value) { settings.SetFirstTenHotKeysStart(value); } },
	{ SettingFirstTenHotkeysFontSize, [](CGetSetOptions& settings, long value) { settings.SetFirstTenHotKeysFontSize(value); } },
	{ SettingCopySaveDelay, [](CGetSetOptions& settings, long value) { settings.SetCopyAndSveDelay(value); } },
	{ SettingEditorFontSize, [](CGetSetOptions& settings, long value) { settings.SetEditorDefaultFontSize(value); } },
	{ SettingIgnoreFalseCopiesDelay, [](CGetSetOptions& settings, long value) { settings.SetSaveClipDelay(value); } },
	{ SettingClipEditSaveDelayAfterLoad, [](CGetSetOptions& settings, long value) { settings.SetClipEditSaveDelayAfterLoadSeconds(value); } },
	{ SettingClipEditSaveDelayAfterSave, [](CGetSetOptions& settings, long value) { settings.SetClipEditSaveDelayAfterSaveSeconds(value); } },
} };

const std::array<CAdvGeneral::BoolSetting, 43> CAdvGeneral::s_boolSettings{ {
	{ SettingShowTaskbarIcon, [](CGetSetOptions& settings, BOOL value) { settings.SetShowIconInSysTray(value); } },
	{ SettingSaveMultiPaste, [](CGetSetOptions& settings, BOOL value) { settings.SetSaveMultiPaste(value); } },
	{ SettingHideOnHotkeyIfVisible, [](CGetSetOptions& settings, BOOL value) { settings.SetHideDittoOnHotKeyIfAlreadyShown(value); } },
	{ SettingPasteInActiveWindow, [](CGetSetOptions& settings, BOOL value) { settings.SetSendPasteAfterSelection(value); } },
	{ SettingEnsureConnected, [](CGetSetOptions& settings, BOOL value) { settings.SetEnsureConnectToClipboard(value); } },
	{ SettingTextFirstTen, [](CGetSetOptions& settings, BOOL value) { settings.SetShowTextForFirstTenHotKeys(value); } },
	{ SettingShowLeadingWhitespace, [](CGetSetOptions& settings, BOOL value) { settings.SetDescShowLeadingWhiteSpace(value); } },
	{ SettingEnableTransparency, [](CGetSetOptions& settings, BOOL value) { settings.SetEnableTransparency(value); } },
	{ SettingDrawThumbnails, [](CGetSetOptions& settings, BOOL value) { settings.SetDrawThumbnail(value); } },
	{ SettingFastThumbnailMode, [](CGetSetOptions& settings, BOOL value) { settings.SetFastThumbnailMode(value); } },
	{ SettingDrawRtf, [](CGetSetOptions& settings, BOOL value) { settings.SetDrawRTF(value); } },
	{ SettingFindAsType, [](CGetSetOptions& settings, BOOL value) { settings.SetFindAsYouType(value); } },
	{ SettingEnsureWindowIsVisible, [](CGetSetOptions& settings, BOOL value) { settings.SetEnsureEntireWindowCanBeSeen(value); } },
	{ SettingShowGroupClipsInList, [](CGetSetOptions& settings, BOOL value) { settings.SetShowAllClipsInMainList(value); } },
	{ SettingPromptOnDelete, [](CGetSetOptions& settings, BOOL value) { settings.SetPromptWhenDeletingClips(value); } },
	{ SettingAlwaysShowScrollBar, [](CGetSetOptions& settings, BOOL value) { settings.SetShowScrollBar(value); } },
	{ SettingUseModernScrollbar, [](CGetSetOptions& settings, BOOL value) { settings.SetUseModernScrollBar(value); } },
	{ SettingPasteAsAdmin, [](CGetSetOptions& settings, BOOL value) { settings.SetPasteAsAdmin(value); } },
	{ SettingShowInTaskbar, [](CGetSetOptions& settings, BOOL value) { settings.SetShowInTaskBar(value); } },
	{ SettingShowClipPasted, [](CGetSetOptions& settings, BOOL value) { settings.SetShowIfClipWasPasted(value); } },
	{ SettingUpdateOrderOnPaste, [](CGetSetOptions& settings, BOOL value) { settings.SetUpdateTimeOnPaste(value); } },
	{ SettingUpdateOrderOnCtrlC, [](CGetSetOptions& settings, BOOL value) { settings.SetUpdateClipOrderOnCtrlC(value); } },
	{ SettingMultipasteReverseOrder, [](CGetSetOptions& settings, BOOL value) { settings.SetMultiPasteReverse(value); } },
	{ SettingAllowDuplicates, [](CGetSetOptions& settings, BOOL value) { settings.SetAllowDuplicates(value); } },
	{ SettingAllowBackToBackDuplicates, [](CGetSetOptions& settings, BOOL value) { settings.SetAllowBackToBackDuplicates(value); } },
	{ SettingShowStartupMessage, [](CGetSetOptions& settings, BOOL value) { settings.SetShowStartupMessage(value); } },
	{ SettingRevertToTopLevelGroup, [](CGetSetOptions& settings, BOOL value) { settings.SetRevertToTopLevelGroup(value); } },
	{ SettingOpenToGroupAsActiveExe, [](CGetSetOptions& settings, BOOL value) { settings.SetOpenToGroupByActiveExe(value); } },
	{ SettingAddCfHdropOnDrag, [](CGetSetOptions& settings, BOOL value) { settings.SetAddCFHDROP_OnDrag(value); } },
	{ SettingMoveSelectionOnOpenHotkey, [](CGetSetOptions& settings, BOOL value) { settings.SetMoveSelectionOnOpenHotkey(value); } },
	{ SettingMaintainSearchView, [](CGetSetOptions& settings, BOOL value) { settings.SetMaintainSearchView(value); } },
	{ SettingDebugToFile, [](CGetSetOptions& settings, BOOL value) { settings.SetEnableDebugLogging(value); } },
	{ SettingDebugToOutputString, [](CGetSetOptions& settings, BOOL value) { settings.SetEnableOutputDebugStringLogging(value); } },
	{ SettingRefreshViewAfterPaste, [](CGetSetOptions& settings, BOOL value) { settings.SetRefreshViewAfterPasting(value); } },
	{ SettingSupportAllTypes, [](CGetSetOptions& settings, BOOL value) { settings.SetSupportAllTypes(value); } },
	{ SettingRegexCaseInsensitive, [](CGetSetOptions& settings, BOOL value) { settings.SetRegexCaseInsensitive(value); } },
	{ SettingDrawCopiedColorCode, [](CGetSetOptions& settings, BOOL value) { settings.SetDrawCopiedColorCode(value); } },
	{ SettingCenterWindowBelowCursorCaret, [](CGetSetOptions& settings, BOOL value) { settings.SetCenterWindowBelowCursorOrCaret(value); } },
	{ SettingUpdateDescOnClipEdit, [](CGetSetOptions& settings, BOOL value) { settings.SetUpdateDescWhenSavingClip(value); } },
	{ SettingUseUtf8ForDiff, [](CGetSetOptions& settings, BOOL value) { settings.SetPreferUtf8ForCompare(value); } },
	{ SettingDoNotHideOnDeactivate, [](CGetSetOptions& settings, BOOL value) { settings.SetDoNotHideOnDeactivate(value); } },
	{ SettingHideTaskbarIconOnClose, [](CGetSetOptions& settings, BOOL value) { settings.SetHideTaskbarIconOnClose(value); } },
	{ SettingEnforceClipboardIgnoreFormats, [](CGetSetOptions& settings, BOOL value) { settings.SetEnforceClipboardIgnoreFormats(value); } },
} };

const std::array<CAdvGeneral::TextSetting, 11> CAdvGeneral::s_textSettings{ {
	{ SettingClipSeparator, [](CGetSetOptions& settings, LPCTSTR value) { settings.SetMultiPasteSeparator(value); } },
	{ SettingCopyPlaySound, [](CGetSetOptions& settings, LPCTSTR value) { settings.SetPlaySoundOnCopy(value); } },
	{ SettingDiffApp, [](CGetSetOptions& settings, LPCTSTR value) { settings.SetDiffApp(value); } },
	{ SettingDefaultPasteString, [](CGetSetOptions& settings, LPCTSTR value) { settings.SetDefaultPasteString(value); } },
	{ SettingDefaultCopyString, [](CGetSetOptions& settings, LPCTSTR value) { settings.SetDefaultCopyString(value); } },
	{ SettingDefaultCutString, [](CGetSetOptions& settings, LPCTSTR value) { settings.SetDefaultCutString(value); } },
	{ SettingSlugifySeparator, [](CGetSetOptions& settings, LPCTSTR value) { settings.SetSlugifySeparator(value); } },
	{ SettingIgnoreAnnoyingCfDib, [](CGetSetOptions& settings, LPCTSTR value) { settings.SetIgnoreAnnoyingCFDIB(value); } },
	{ SettingTextEditorPath, [](CGetSetOptions& settings, LPCTSTR value) { settings.SetTextEditorPath(value); } },
	{ SettingImageEditorPath, [](CGetSetOptions& settings, LPCTSTR value) { settings.SetImageEditorPath(value); } },
	{ SettingRtfEditorPath, [](CGetSetOptions& settings, LPCTSTR value) { settings.SetRTFEditorPath(value); } },
} };

BOOL CAdvGeneral::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CGetSetOptions& settings = Settings();

	m_propertyGrid.ModifyStyle(0, WS_CLIPCHILDREN);

	HICON b = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 64, 64, LR_SHARED);
	SetIcon(b, TRUE);

	CMFCPropertyGridProperty * pGroupTest = MakeGridProperty<CMFCPropertyGridProperty>( _T( "Ditto" ) );
	m_propertyGrid.AddProperty(pGroupTest);

	m_Resize.SetParent(m_hWnd);
	m_Resize.AddControl(IDC_MFCPROPERTYGRID1, CDialogResizer::SizeWidth | CDialogResizer::SizeHeight);
	m_Resize.AddControl(IDOK, CDialogResizer::MoveTop | CDialogResizer::MoveLeft);
	m_Resize.AddControl(IDCANCEL, CDialogResizer::MoveTop | CDialogResizer::MoveLeft);
	m_Resize.AddControl(IDC_BT_COMPACT_AND_REPAIR, CDialogResizer::MoveTop);
	m_Resize.AddControl(IDC_EDIT_ADV_FILTER, CDialogResizer::SizeWidth);
	m_Resize.AddControl(IDC_BUTTON_NEXT_MATCH, CDialogResizer::MoveLeft);

	HDITEM hdItem;
	hdItem.mask = HDI_WIDTH; // indicating cxy is width
	CDPI dpi(m_hWnd);
	hdItem.cxy = dpi.Scale(400); // whatever you want the property name column width to be
	m_propertyGrid.GetHeaderCtrl().SetItem(0, &hdItem);

	m_propertyGrid.SetFont(this->GetFont());	

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Activate window delay (100ms default)"), (long)settings.SendKeysDelay(), _T(""), SettingActivateWindowDelay));

	AddTrueFalse(pGroupTest, _T("Add file drop when dragging clips"), settings.GetAddCFHDROP_OnDrag(), SettingAddCfHdropOnDrag);

	AddTrueFalse(pGroupTest, _T("Allow duplicates"), settings.GetAllowDuplicates(), SettingAllowDuplicates);
	AddTrueFalse(pGroupTest, _T("Allow back to back duplicates (if allowing duplicates)"), settings.GetAllowBackToBackDuplicates(), SettingAllowBackToBackDuplicates);

	AddTrueFalse(pGroupTest, _T("Always show scroll bar"), settings.GetShowScrollBar(), SettingAlwaysShowScrollBar);
	AddTrueFalse(pGroupTest, _T("Use modern scroll bar"), settings.GetUseModernScrollBar(), SettingUseModernScrollbar);
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Amount of text to save for description"), settings.m_bDescTextSize, _T(""), SettingDescSize));
	AddTrueFalse(pGroupTest, _T("Center window below cursor or caret"), settings.GetCenterWindowBelowCursorOrCaret(), SettingCenterWindowBelowCursorCaret);
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Copy and save clipboard delay (ms)"), (long)settings.GetCopyAndSveDelay(), _T(""), SettingCopySaveDelay));

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Clip edit save delay after load"), (long)(settings.GetClipEditSaveDelayAfterLoadSeconds()), _T(""), SettingClipEditSaveDelayAfterLoad));
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Clip edit save delay after Save"), (long)(settings.GetClipEditSaveDelayAfterSaveSeconds()), _T(""), SettingClipEditSaveDelayAfterSave));

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Clipboard restore delay after copy buffer sent paste (ms, default: 750)"), (long)(settings.GetDittoRestoreClipboardDelay()), _T(""), SettingClipboardRestoreAfterCopyBufferDelay));

	CString defaultPasteString = settings.GetDefaultPasteString();
	CString defaultCopyString = settings.GetDefaultCopyString();
	CString defaultCutString = settings.GetDefaultCutString();
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Default paste string"), defaultPasteString, _T(""), SettingDefaultPasteString));
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Default copy string"), defaultCopyString, _T(""), SettingDefaultCopyString));
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Default cut string"), defaultCutString, _T(""), SettingDefaultCutString));
	
	static const TCHAR BASED_CODE szDiffFilter[] = _T("Diff Applications(*.exe)|*.exe||");
	CMFCPropertyGridFileProperty* pDiffProp = MakeGridProperty<CMFCPropertyGridFileProperty>(_T("Diff application path"), TRUE, settings.GetDiffApp(), _T("exe"), 0, szDiffFilter, (LPCTSTR)0, SettingDiffApp);
	pGroupTest->AddSubItem(pDiffProp);

	AddTrueFalse(pGroupTest, _T("Diff save compare files as utf8"), settings.GetPreferUtf8ForCompare(), SettingUseUtf8ForDiff);

	AddTrueFalse(pGroupTest, _T("Display icon in system tray"), settings.GetShowIconInSysTray(), SettingShowTaskbarIcon);

	AddTrueFalse(pGroupTest, _T("Do not hide Ditto window on deactivate"), settings.GetDoNotHideOnDeactivate(), SettingDoNotHideOnDeactivate);

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Double shortcut keystroke timeout)"), (long)settings.GetDoubleKeyStrokeTimeout(), _T(""), SettingDoubleKeystrokeTimeout));

	AddTrueFalse(pGroupTest, _T("Draw swatch for hex, RGB, and HSL colors"), settings.GetDrawCopiedColorCode(), SettingDrawCopiedColorCode);

	AddTrueFalse(pGroupTest, _T("Draw RTF text in list (for RTF types) (could increase memory usage an display speed)"), settings.GetDrawRTF(), SettingDrawRtf);
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Editor default font size"), (long)settings.GetEditorDefaultFontSize(), _T(""), SettingEditorFontSize));
	AddTrueFalse(pGroupTest, _T("Enforce clipboard ignore formats"), settings.GetEnforceClipboardIgnoreFormats(), SettingEnforceClipboardIgnoreFormats);
	AddTrueFalse(pGroupTest, _T("Elevated privileges to paste into elevated apps"), settings.GetPasteAsAdmin(), SettingPasteAsAdmin);
	AddTrueFalse(pGroupTest, _T("Ensure Ditto is always connected to the clipboard"), settings.GetEnsureConnectToClipboard(), SettingEnsureConnected);
	AddTrueFalse(pGroupTest, _T("Ensure entire window is visible"), settings.GetEnsureEntireWindowCanBeSeen(), SettingEnsureWindowIsVisible);

	AddTrueFalse(pGroupTest, _T("Fast thumbnails (True = fast / low quality (default). False = slow / high quality)"), settings.GetFastThumbnailMode(), SettingFastThumbnailMode);

	AddTrueFalse(pGroupTest, _T("Find as you type"), settings.GetFindAsYouType(), SettingFindAsType);

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("First ten hot keys start index"), (long)settings.GetFirstTenHotKeysStart(), _T(""), SettingFirstTenHotkeysStart));
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("First ten hot keys font size"), (long)settings.GetFirstTenHotKeysFontSize(), _T(""), SettingFirstTenHotkeysFontSize));

	AddTrueFalse(pGroupTest, _T("Hide Ditto on hot key if Ditto is visible"), settings.GetHideDittoOnHotKeyIfAlreadyShown(), SettingHideOnHotkeyIfVisible);

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Ignore copies faster than (ms) (default: 500)"), (long)settings.GetSaveClipDelay(), _T(""), SettingIgnoreFalseCopiesDelay));
	CString ignoreAnnoyingCFDIB = settings.GetIgnoreAnnoyingCFDIB();
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Ignore CF_DIB when a clip is detected as text content"), ignoreAnnoyingCFDIB, _T("Case insensitive. Recommended option is \"excel.exe; onenote.exe; powerpnt.exe\" "), SettingIgnoreAnnoyingCfDib));

	static const TCHAR BASED_CODE szImageEditorFilter[] = _T("Applications(*.exe)|*.exe||");
	CMFCPropertyGridFileProperty* pImageEditorProp = MakeGridProperty<CMFCPropertyGridFileProperty>(_T("Image editor path (empty for system mapping)"), TRUE, settings.GetImageEditorPath(), _T("exe"), 0, szImageEditorFilter, (LPCTSTR)0, SettingImageEditorPath);
	pGroupTest->AddSubItem(pImageEditorProp);

	pGroupTest->AddSubItem( MakeGridProperty<CMFCPropertyGridProperty>(_T("Maximum clip size in bytes (0 for no limit)"), settings.m_lMaxClipSizeInBytes, _T(""), SettingMaxClipSize));
		
	AddTrueFalse(pGroupTest, _T("Maintain search view"), settings.GetMaintainSearchView(), SettingMaintainSearchView);

	AddTrueFalse(pGroupTest, _T("Move selection on open hot key"), settings.GetMoveSelectionOnOpenHotkey(), SettingMoveSelectionOnOpenHotkey);
	
	CString multiPasteSeparator = settings.GetMultiPasteSeparator(false);
	pGroupTest->AddSubItem( MakeGridProperty<CMFCPropertyGridProperty>(_T("Multi-paste clip separator ([LF] = line feed)"), multiPasteSeparator, _T(""), SettingClipSeparator));

	AddTrueFalse(pGroupTest, _T("Multi-paste in reverse order"), settings.m_bMultiPasteReverse, SettingMultipasteReverseOrder);

	AddTrueFalse(pGroupTest, _T("Open to group same as active exe"), settings.GetOpenToGroupByActiveExe(), SettingOpenToGroupAsActiveExe);

	static const TCHAR BASED_CODE szFilter[] = _T("Sounds(*.wav)|*.wav||");
	CMFCPropertyGridFileProperty* pFileProp = MakeGridProperty<CMFCPropertyGridFileProperty>(_T("On copy play the sound"), TRUE, settings.GetPlaySoundOnCopy(), _T("wav"), 0, szFilter, (LPCTSTR)0, SettingCopyPlaySound);
	pGroupTest->AddSubItem(pFileProp);

	static const TCHAR BASED_CODE szTextEditorFilter[] = _T("Applications(*.exe)|*.exe||");
	CMFCPropertyGridFileProperty* pTextEditorProp = MakeGridProperty<CMFCPropertyGridFileProperty>(_T("Text editor path (empty for system mapping)"), TRUE, settings.GetTextEditorPath(), _T("exe"), 0, szTextEditorFilter, (LPCTSTR)0, SettingTextEditorPath);
	pGroupTest->AddSubItem(pTextEditorProp);

	AddTrueFalse(pGroupTest, _T("Paste clip in active window after selection"), settings.GetSendPasteAfterSelection(), SettingPasteInActiveWindow);	

	AddTrueFalse(pGroupTest, _T("Prompt when deleting clips"), settings.GetPromptWhenDeletingClips(), SettingPromptOnDelete);

	AddTrueFalse(pGroupTest, _T("Revert to top level group on close"), settings.GetRevertToTopLevelGroup(), SettingRevertToTopLevelGroup);

	AddTrueFalse(pGroupTest, _T("Refresh view after paste"), settings.GetRefreshViewAfterPasting(), SettingRefreshViewAfterPaste);

	AddTrueFalse(pGroupTest, _T("Regex case insensitive search"), settings.GetRegexCaseInsensitive(), SettingRegexCaseInsensitive);

	static const TCHAR BASED_CODE szRTFEditorFilter[] = _T("Applications(*.exe)|*.exe||");
	CMFCPropertyGridFileProperty* pRTFEditorProp = MakeGridProperty<CMFCPropertyGridFileProperty>(_T("RTF editor path"), TRUE, settings.GetRTFEditorPath(), _T("exe"), 0, szRTFEditorFilter, (LPCTSTR)0, SettingRtfEditorPath);
	pGroupTest->AddSubItem(pRTFEditorProp);

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Save clipboard delay (ms, default: 100)"), (long)(settings.GetProcessDrawClipboardDelay()), _T(""), SettingClipboardSaveDelay));

	AddTrueFalse(pGroupTest, _T("Save multi-pastes"), settings.GetSaveMultiPaste(), SettingSaveMultiPaste);

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Selected index"), (long)(settings.SelectedIndex()+1), _T(""), SettingSelectedIndex));

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Send keys delay (ms)"), (long)settings.RealSendKeysDelay(), _T(""), SettingSendKeysDelay));


	AddTrueFalse(pGroupTest, _T("Show clips that are in groups in main list"), settings.GetShowAllClipsInMainList(), SettingShowGroupClipsInList);
	AddTrueFalse(pGroupTest, _T("Show leading whitespace"), settings.GetDescShowLeadingWhiteSpace(), SettingShowLeadingWhitespace);
	AddTrueFalse(pGroupTest, _T("Show in taskbar"), settings.GetShowInTaskBar(), SettingShowInTaskbar);
	AddTrueFalse(pGroupTest, _T("Hide taskbar icon when Ditto window closes"), settings.GetHideTaskbarIconOnClose(), SettingHideTaskbarIconOnClose);
	AddTrueFalse(pGroupTest, _T("Show indicator a clip has been pasted"), settings.GetShowIfClipWasPasted(), SettingShowClipPasted);

	AddTrueFalse(pGroupTest, _T("Show startup tooltip message"), settings.GetShowStartupMessage(), SettingShowStartupMessage);

	AddTrueFalse(pGroupTest, _T("Show text for first ten copy hot keys"), settings.GetShowTextForFirstTenHotKeys(), SettingTextFirstTen);
	AddTrueFalse(pGroupTest, _T("Show thumbnails(for CF_DIB and PNG types) (could increase memory usage and display speed)"), settings.GetDrawThumbnail(), SettingDrawThumbnails);
	
	CString slugifySeparator = settings.GetSlugifySeparator();
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Slugify Separator (default: -)"), slugifySeparator, _T(""), SettingSlugifySeparator));

	AddTrueFalse(pGroupTest, _T("Support all types ignoring supported type list (default: false))"), settings.GetSupportAllTypes(), SettingSupportAllTypes);

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Text lines per clip"), settings.GetLinesPerRow(), _T(""), SettingLinesPerRow));

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Tooltip display time(ms) max of 32000 (-1 default (5 seconds), 0 to turn off)"), settings.m_tooltipTimeout, _T(""), SettingTooltipTimeout));
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Tooltip maximum display lines"), (long)settings.GetMaxToolTipLines(), _T(""), SettingTooltipLines));
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Tooltip display characters"), (long)settings.GetMaxToolTipCharacters(), _T(""), SettingTooltipCharacters));

	AddTrueFalse(pGroupTest, _T("Transparency enabled"), settings.GetEnableTransparency(), SettingEnableTransparency);
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Transparency percentage"), settings.GetTransparencyPercent(), _T(""), SettingTransparency));
	AddTrueFalse(pGroupTest, _T("Update description on clip edit"), settings.GetUpdateDescWhenSavingClip(), SettingUpdateDescOnClipEdit);
	AddTrueFalse(pGroupTest, _T("Update clip order on paste"), settings.GetUpdateTimeOnPaste(), SettingUpdateOrderOnPaste);
	AddTrueFalse(pGroupTest, _T("Update clip Order on ctrl-c"), settings.GetUpdateClipOrderOnCtrlC(), SettingUpdateOrderOnCtrlC);

	AddTrueFalse(pGroupTest, _T("Write debug to file"), settings.GetEnableDebugLogging(), SettingDebugToFile);
	AddTrueFalse(pGroupTest, _T("Write debug to OutputDebugString"), settings.GetEnableDebugLogging(), SettingDebugToOutputString);

	CMFCPropertyGridProperty * regexFilterGroup = MakeGridProperty<CMFCPropertyGridProperty>(_T("Exclude clips by Regular Expressions"));
	m_propertyGrid.AddProperty(regexFilterGroup);

	CString processFilterDesc = _T("Process making the copy first must match this before the Regex will be applied (empty or * for all processes) (separate multiples by ;)");
	CString regexFilterDesc = _T("If copied text matches this regular expression then the clip will not be saved to Ditto");

	CString regexFilter1 = settings.GetRegexFilter(0);
	CString regexProcessName1 = settings.GetRegexFilterByProcessName(0);
	CString regexFilter2 = settings.GetRegexFilter(1);
	CString regexProcessName2 = settings.GetRegexFilterByProcessName(1);
	CString regexFilter3 = settings.GetRegexFilter(2);
	CString regexProcessName3 = settings.GetRegexFilterByProcessName(2);
	CString regexFilter4 = settings.GetRegexFilter(3);
	CString regexProcessName4 = settings.GetRegexFilterByProcessName(3);
	CString regexFilter5 = settings.GetRegexFilter(4);
	CString regexProcessName5 = settings.GetRegexFilterByProcessName(4);
	CString regexFilter6 = settings.GetRegexFilter(5);
	CString regexProcessName6 = settings.GetRegexFilterByProcessName(5);
	CString regexFilter7 = settings.GetRegexFilter(6);
	CString regexProcessName7 = settings.GetRegexFilterByProcessName(6);
	CString regexFilter8 = settings.GetRegexFilter(7);
	CString regexProcessName8 = settings.GetRegexFilterByProcessName(7);
	CString regexFilter9 = settings.GetRegexFilter(8);
	CString regexProcessName9 = settings.GetRegexFilterByProcessName(8);
	CString regexFilter10 = settings.GetRegexFilter(9);
	CString regexProcessName10 = settings.GetRegexFilterByProcessName(9);
	CString regexFilter11 = settings.GetRegexFilter(10);
	CString regexProcessName11 = settings.GetRegexFilterByProcessName(10);
	CString regexFilter12 = settings.GetRegexFilter(11);
	CString regexProcessName12 = settings.GetRegexFilterByProcessName(11);
	CString regexFilter13 = settings.GetRegexFilter(12);
	CString regexProcessName13 = settings.GetRegexFilterByProcessName(12);
	CString regexFilter14 = settings.GetRegexFilter(13);
	CString regexProcessName14 = settings.GetRegexFilterByProcessName(13);
	CString regexFilter15 = settings.GetRegexFilter(14);
	CString regexProcessName15 = settings.GetRegexFilterByProcessName(14);

	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("1 Regex"), regexFilter1, regexFilterDesc, SettingRegexFiltering1));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("1 Process Name"), regexProcessName1, processFilterDesc, SettingRegexFilteringByProcessName1));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("2 Regex"), regexFilter2, regexFilterDesc, SettingRegexFiltering2));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("2 Process Name"), regexProcessName2, processFilterDesc, SettingRegexFilteringByProcessName2));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("3 Regex"), regexFilter3, regexFilterDesc, SettingRegexFiltering3));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("3 Process Name"), regexProcessName3, processFilterDesc, SettingRegexFilteringByProcessName3));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("4 Regex"), regexFilter4, regexFilterDesc, SettingRegexFiltering4));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("4 Process Name"), regexProcessName4, processFilterDesc, SettingRegexFilteringByProcessName4));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("5 Regex"), regexFilter5, regexFilterDesc, SettingRegexFiltering5));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("5 Process Name"), regexProcessName5, processFilterDesc, SettingRegexFilteringByProcessName5));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("6 Regex"), regexFilter6, regexFilterDesc, SettingRegexFiltering6));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("6 Process Name"), regexProcessName6, processFilterDesc, SettingRegexFilteringByProcessName6));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("7 Regex"), regexFilter7, regexFilterDesc, SettingRegexFiltering7));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("7 Process Name"), regexProcessName7, processFilterDesc, SettingRegexFilteringByProcessName7));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("8 Regex"), regexFilter8, regexFilterDesc, SettingRegexFiltering8));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("8 Process Name"), regexProcessName8, processFilterDesc, SettingRegexFilteringByProcessName8));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("9 Regex"), regexFilter9, regexFilterDesc, SettingRegexFiltering9));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("9 Process Name"), regexProcessName9, processFilterDesc, SettingRegexFilteringByProcessName9));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("10 Regex"), regexFilter10, regexFilterDesc, SettingRegexFiltering10));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("10 Process Name"), regexProcessName10, processFilterDesc, SettingRegexFilteringByProcessName10));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("11 Regex"), regexFilter11, regexFilterDesc, SettingRegexFiltering11));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("11 Process Name"), regexProcessName11, processFilterDesc, SettingRegexFilteringByProcessName11));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("12 Regex"), regexFilter12, regexFilterDesc, SettingRegexFiltering12));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("12 Process Name"), regexProcessName12, processFilterDesc, SettingRegexFilteringByProcessName12));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("13 Regex"), regexFilter13, regexFilterDesc, SettingRegexFiltering13));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("13 Process Name"), regexProcessName13, processFilterDesc, SettingRegexFilteringByProcessName13));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("14 Regex"), regexFilter14, regexFilterDesc, SettingRegexFiltering14));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("14 Process Name"), regexProcessName14, processFilterDesc, SettingRegexFilteringByProcessName14));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("15 Regex"), regexFilter15, regexFilterDesc, SettingRegexFiltering15));
	regexFilterGroup->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("15 Process Name"), regexProcessName15, processFilterDesc, SettingRegexFilteringByProcessName15));

	regexFilterGroup->Expand(FALSE);

	return TRUE;
}

void CAdvGeneral::AddTrueFalse(CMFCPropertyGridProperty * pGroupTest, CString desc, BOOL value, int settingId)
{
	CString stringValue = _T("False");
	if(value)
	{
		stringValue = _T("True");
	}

	std::unique_ptr<CMFCPropertyGridProperty> pCombo{ std::make_unique<CMFCPropertyGridProperty>(desc, stringValue, _T(""), settingId) };
	pCombo->AddOption(_T("True"));
	pCombo->AddOption(_T("False"));
	pCombo->AllowEdit(FALSE);
	pGroupTest->AddSubItem(pCombo.release()); // ownership: the property grid
}

void CAdvGeneral::OnBnClickedOk()
{
	int topLevelCount = m_propertyGrid.GetPropertyCount();
	for (int topLevel = 0; topLevel < topLevelCount; topLevel++)
	{
		int count = m_propertyGrid.GetProperty(topLevel)->GetSubItemsCount();
		for (int row = 0; row < count; row++)
		{
			WriteSetting(m_propertyGrid.GetProperty(topLevel)->GetSubItem(row));
		}
	}
	CDialogEx::OnOK();
}

void CAdvGeneral::WriteSetting(CMFCPropertyGridProperty* prop)
{
	COleVariant i = prop->GetValue();
	LPVARIANT pNewValue = (LPVARIANT)i;

	COleVariant iOrig = prop->GetOriginalValue();
	LPVARIANT pOrigValue = (LPVARIANT)iOrig;

	const int id = (int)prop->GetData();
	CGetSetOptions& settings = Settings();

	if (const LongSetting* longSetting = FindSetting(s_longSettings, id))
	{
		if (pNewValue->lVal != pOrigValue->lVal)
		{
			longSetting->write(settings, pNewValue->lVal);
		}
	}
	else if (const BoolSetting* boolSetting = FindSetting(s_boolSettings, id))
	{
		if (wcscmp(pNewValue->bstrVal, pOrigValue->bstrVal) != 0)
		{
			BOOL val = wcscmp(pNewValue->bstrVal, L"True") == 0;
			boolSetting->write(settings, val);
		}
	}
	else if (const TextSetting* textSetting = FindSetting(s_textSettings, id))
	{
		if (wcscmp(pNewValue->bstrVal, pOrigValue->bstrVal) != 0)
		{
			textSetting->write(settings, pNewValue->bstrVal);
		}
	}
	else
	{
		WriteRegexSetting(settings, id, *pNewValue, *pOrigValue);
	}
}

void CAdvGeneral::WriteTransparencyPercent(CGetSetOptions& settings, long newValue)
{
	int value = 100;
	if (newValue <= 100 && newValue > 0)
	{
		value = newValue;
	}

	settings.SetTransparencyPercent(value);
}

void CAdvGeneral::WriteRegexSetting(CGetSetOptions& settings, int id,const VARIANT& newValue, const VARIANT& origValue)
{
	if (id >= SettingRegexFiltering1 && id <= SettingRegexFiltering15)
	{
		if (wcscmp(newValue.bstrVal, origValue.bstrVal) != 0)
		{
			settings.SetRegexFilter(newValue.bstrVal, (id - SettingRegexFiltering1));
		}
	}
	else if (id >= SettingRegexFilteringByProcessName1 && id <= SettingRegexFilteringByProcessName15)
	{
		if (wcscmp(newValue.bstrVal, origValue.bstrVal) != 0)
		{
			settings.SetRegexFilterByProcessName(newValue.bstrVal, (id - SettingRegexFilteringByProcessName1));
		}
	}
}

void CAdvGeneral::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	if (((GetKeyState(VK_LBUTTON) & 0x100) != 0) &&
		m_mouseDownOnCaption == false)
	{
		m_Resize.MoveControls(CSize(cx, cy));
	}
	else
	{
		m_Resize.SetParent(m_hWnd);
	}
}

void CAdvGeneral::OnBnClickedBtCompactAndRepair()
{
	auto msg = theApp.Services().Language().GetString("CompactRepairWarning", "Warning this can take quite a long time and require up to double the hard drive space as your current database size, Continue?");
	int ret = MessageBox(msg, _T("Ditto"), MB_OKCANCEL);

	if (ret == IDOK)
	{
		CWaitCursor wait;
		CDittoDb& database = theApp.Services().Database();

		try
		{
			try
			{
				for (int i = 0; i < 100; i++)
				{
					int toDeleteCount = database.execScalar(_T("SELECT COUNT(clipID) FROM MainDeletes"));
					if (toDeleteCount <= 0)
						break;

					CClipRetentionPolicy::RemoveOldEntries(Settings(), theApp.Services().IdleTime(), theApp.Services().Windows(), false);
				}
			}
			catch (CppSQLite3Exception& e)
			{
				CErrorReport::Show(CStringUtil::Format(_T("Compact and repair failed while removing deleted clips, the database was not compacted: %s"), e.errorMessage()));
				return;
			}

			database.execDML(_T("PRAGMA auto_vacuum = 1"));
			database.execQuery(_T("VACUUM"));
		}
		catch (CppSQLite3Exception& e)
		{
			CErrorReport::Show(CStringUtil::Format(_T("Compacting and repairing the clip database failed: %s"), e.errorMessage()));
			return;
		}
	}
}

void CAdvGeneral::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize.x = 450;
	lpMMI->ptMinTrackSize.y = 450;

	CDialogEx::OnGetMinMaxInfo(lpMMI);
}

void CAdvGeneral::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	m_mouseDownOnCaption = false;

	if (nHitTest == HTCAPTION)
	{
		m_mouseDownOnCaption = true;
	}

	CDialog::OnNcLButtonDown(nHitTest, point);
}

void CAdvGeneral::OnEnChangeAdvFilter()
{
	Search(false);
}

void CAdvGeneral::Search(bool fromSelection)
{
	CString filterText;
	m_editFilter.GetWindowText(filterText);
	filterText.MakeLower();

	if (filterText == _T(""))
	{
		m_propertyGrid.SetCurSel(m_propertyGrid.GetProperty(0));
		m_propertyGrid.EnsureVisible(m_propertyGrid.GetProperty(0), TRUE);
		return;
	}

	auto selection = m_propertyGrid.GetCurSel();
	bool foundSelection = false;

	for (int i = 0; i < m_propertyGrid.GetPropertyCount(); ++i)
	{
		CMFCPropertyGridProperty* pProp = m_propertyGrid.GetProperty(i);
		if (pProp != nullptr)
		{
			SearchGroup(pProp, filterText, fromSelection, selection, foundSelection);
		}
	}
}

void CAdvGeneral::SearchGroup(CMFCPropertyGridProperty* pProp, const CString& filterText, bool fromSelection, CMFCPropertyGridProperty* selection, bool& foundSelection)
{
	CString name = pProp->GetName();
	name.MakeLower();

	for (int row = 0; row < pProp->GetSubItemsCount(); ++row)
	{
		auto pSubItem = pProp->GetSubItem(row);
		if (pSubItem != nullptr)
		{
			if (fromSelection && selection != nullptr && foundSelection == false)
			{
				if (selection == pSubItem)
				{
					foundSelection = true;
				}
				continue;
			}

			CString subName = pSubItem->GetName();
			subName.MakeLower();
			if (subName.Find(filterText) >= 0)
			{
				ShowSearchMatch(pProp, row, pSubItem);
				break;
			}
		}
	}
}

void CAdvGeneral::ShowSearchMatch(CMFCPropertyGridProperty* pProp, int row, CMFCPropertyGridProperty* pSubItem)
{
	pSubItem->Show();
	m_propertyGrid.SetCurSel(pSubItem);

	//calling EnsureVisible mutliple times seemed to show it better otherwise it would randomly not work
	if (row > 2)
	{
		m_propertyGrid.EnsureVisible(pProp->GetSubItem(row - 2), TRUE);
		m_propertyGrid.EnsureVisible(pProp->GetSubItem(row - 2), TRUE);
		m_propertyGrid.EnsureVisible(pProp->GetSubItem(row - 2), TRUE);
	}
	else if (row > 1)
	{
		m_propertyGrid.EnsureVisible(pProp->GetSubItem(row - 1), TRUE);
		m_propertyGrid.EnsureVisible(pProp->GetSubItem(row - 1), TRUE);
		m_propertyGrid.EnsureVisible(pProp->GetSubItem(row - 1), TRUE);
	}
	else
	{
		m_propertyGrid.EnsureVisible(pSubItem, TRUE);
		m_propertyGrid.EnsureVisible(pSubItem, TRUE);
		m_propertyGrid.EnsureVisible(pSubItem, TRUE);
	}
}

BOOL CAdvGeneral::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		int idCtrl = this->GetFocus()->GetDlgCtrlID();
		if (idCtrl == IDC_EDIT_ADV_FILTER)
		{
			Search(true);
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CAdvGeneral::OnBnClickedButtonNextMatch()
{
	Search(true);	
}

void CAdvGeneral::OnBnClickedButtonCopyScripts2()
{
	CDimWnd dimmer(this);

	CMoveToGroupDlg dlg(this, _T("Select group to reset clip order"));

	const auto ret = dlg.DoModal();
	if (ret == IDOK)
	{
		CWaitCursor wait;

		const int groupID = dlg.GetSelectedGroup();

		CString reOrderSql = R"(

			WITH OrderedRows AS(
				SELECT
					rowid AS original_rowid,
					ROW_NUMBER() OVER(ORDER BY {orderField} ASC) AS rn
				FROM
					Main
				WHERE lParentID = {parentID}
			)
			--Update the main table using the CTE results
			UPDATE 
				Main
			SET {orderField} = (
					SELECT rn
					FROM OrderedRows
					WHERE OrderedRows.original_rowid = Main.rowid
				)
			WHERE lParentID = {parentID}
		)";

		if (groupID == -1)
		{
			reOrderSql.Replace(_T("{orderField}"), _T("clipOrder"));

			//reorder all clip
			reOrderSql.Replace(_T("WHERE lParentID = {parentID}"), _T(""));
		}
		else
		{
			reOrderSql.Replace(_T("{parentID}"), std::to_wstring(groupID).c_str());
			reOrderSql.Replace(_T("{orderField}"), _T("clipGroupOrder"));
		}

		try
		{
			theApp.Services().Database().execDML(reOrderSql);
		}
		catch (CppSQLite3Exception& e)
		{
			MessageBox(e.errorMessage());
		}
	}
}
