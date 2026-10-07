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
	{ SettingDescSize, [](long value) { CGetSetOptions::SetDescTextSize(value); } },
	{ SettingSelectedIndex, [](long value) { CGetSetOptions::SetSelectedIndex(max((value - 1), 0)); } },
	{ SettingClipboardSaveDelay, [](long value) { CGetSetOptions::SetProcessDrawClipboardDelay(max(value, 0)); } },
	{ SettingMaxClipSize, [](long value) { CGetSetOptions::SetMaxClipSizeInBytes(value); } },
	{ SettingLinesPerRow, [](long value) { CGetSetOptions::SetLinesPerRow(value); } },
	{ SettingTransparency, &CAdvGeneral::WriteTransparencyPercent },
	{ SettingTooltipTimeout, [](long value) { CGetSetOptions::SetToolTipTimeout(value); } },
	{ SettingTooltipLines, [](long value) { CGetSetOptions::SetMaxToolTipLines(value); } },
	{ SettingTooltipCharacters, [](long value) { CGetSetOptions::SetMaxToolTipCharacters(value); } },
	{ SettingActivateWindowDelay, [](long value) { CGetSetOptions::SetSendKeysDelay(value); } },
	{ SettingSendKeysDelay, [](long value) { CGetSetOptions::SetRealSendKeysDelay(value); } },
	{ SettingClipboardRestoreAfterCopyBufferDelay, [](long value) { CGetSetOptions::SetDittoRestoreClipboardDelay(value); } },
	{ SettingDoubleKeystrokeTimeout, [](long value) { CGetSetOptions::SetDoubleKeyStrokeTimeout(value); } },
	{ SettingFirstTenHotkeysStart, [](long value) { CGetSetOptions::SetFirstTenHotKeysStart(value); } },
	{ SettingFirstTenHotkeysFontSize, [](long value) { CGetSetOptions::SetFirstTenHotKeysFontSize(value); } },
	{ SettingCopySaveDelay, [](long value) { CGetSetOptions::SetCopyAndSveDelay(value); } },
	{ SettingEditorFontSize, [](long value) { CGetSetOptions::SetEditorDefaultFontSize(value); } },
	{ SettingIgnoreFalseCopiesDelay, [](long value) { CGetSetOptions::SetSaveClipDelay(value); } },
	{ SettingClipEditSaveDelayAfterLoad, [](long value) { CGetSetOptions::SetClipEditSaveDelayAfterLoadSeconds(value); } },
	{ SettingClipEditSaveDelayAfterSave, [](long value) { CGetSetOptions::SetClipEditSaveDelayAfterSaveSeconds(value); } },
} };

const std::array<CAdvGeneral::BoolSetting, 43> CAdvGeneral::s_boolSettings{ {
	{ SettingShowTaskbarIcon, [](BOOL value) { CGetSetOptions::SetShowIconInSysTray(value); } },
	{ SettingSaveMultiPaste, [](BOOL value) { CGetSetOptions::SetSaveMultiPaste(value); } },
	{ SettingHideOnHotkeyIfVisible, [](BOOL value) { CGetSetOptions::SetHideDittoOnHotKeyIfAlreadyShown(value); } },
	{ SettingPasteInActiveWindow, [](BOOL value) { CGetSetOptions::SetSendPasteAfterSelection(value); } },
	{ SettingEnsureConnected, [](BOOL value) { CGetSetOptions::SetEnsureConnectToClipboard(value); } },
	{ SettingTextFirstTen, [](BOOL value) { CGetSetOptions::SetShowTextForFirstTenHotKeys(value); } },
	{ SettingShowLeadingWhitespace, [](BOOL value) { CGetSetOptions::SetDescShowLeadingWhiteSpace(value); } },
	{ SettingEnableTransparency, [](BOOL value) { CGetSetOptions::SetEnableTransparency(value); } },
	{ SettingDrawThumbnails, [](BOOL value) { CGetSetOptions::SetDrawThumbnail(value); } },
	{ SettingFastThumbnailMode, [](BOOL value) { CGetSetOptions::SetFastThumbnailMode(value); } },
	{ SettingDrawRtf, [](BOOL value) { CGetSetOptions::SetDrawRTF(value); } },
	{ SettingFindAsType, [](BOOL value) { CGetSetOptions::SetFindAsYouType(value); } },
	{ SettingEnsureWindowIsVisible, [](BOOL value) { CGetSetOptions::SetEnsureEntireWindowCanBeSeen(value); } },
	{ SettingShowGroupClipsInList, [](BOOL value) { CGetSetOptions::SetShowAllClipsInMainList(value); } },
	{ SettingPromptOnDelete, [](BOOL value) { CGetSetOptions::SetPromptWhenDeletingClips(value); } },
	{ SettingAlwaysShowScrollBar, [](BOOL value) { CGetSetOptions::SetShowScrollBar(value); } },
	{ SettingUseModernScrollbar, [](BOOL value) { CGetSetOptions::SetUseModernScrollBar(value); } },
	{ SettingPasteAsAdmin, [](BOOL value) { CGetSetOptions::SetPasteAsAdmin(value); } },
	{ SettingShowInTaskbar, [](BOOL value) { CGetSetOptions::SetShowInTaskBar(value); } },
	{ SettingShowClipPasted, [](BOOL value) { CGetSetOptions::SetShowIfClipWasPasted(value); } },
	{ SettingUpdateOrderOnPaste, [](BOOL value) { CGetSetOptions::SetUpdateTimeOnPaste(value); } },
	{ SettingUpdateOrderOnCtrlC, [](BOOL value) { CGetSetOptions::SetUpdateClipOrderOnCtrlC(value); } },
	{ SettingMultipasteReverseOrder, [](BOOL value) { CGetSetOptions::SetMultiPasteReverse(value); } },
	{ SettingAllowDuplicates, [](BOOL value) { CGetSetOptions::SetAllowDuplicates(value); } },
	{ SettingAllowBackToBackDuplicates, [](BOOL value) { CGetSetOptions::SetAllowBackToBackDuplicates(value); } },
	{ SettingShowStartupMessage, [](BOOL value) { CGetSetOptions::SetShowStartupMessage(value); } },
	{ SettingRevertToTopLevelGroup, [](BOOL value) { CGetSetOptions::SetRevertToTopLevelGroup(value); } },
	{ SettingOpenToGroupAsActiveExe, [](BOOL value) { CGetSetOptions::SetOpenToGroupByActiveExe(value); } },
	{ SettingAddCfHdropOnDrag, [](BOOL value) { CGetSetOptions::SetAddCFHDROP_OnDrag(value); } },
	{ SettingMoveSelectionOnOpenHotkey, [](BOOL value) { CGetSetOptions::SetMoveSelectionOnOpenHotkey(value); } },
	{ SettingMaintainSearchView, [](BOOL value) { CGetSetOptions::SetMaintainSearchView(value); } },
	{ SettingDebugToFile, [](BOOL value) { CGetSetOptions::SetEnableDebugLogging(value); } },
	{ SettingDebugToOutputString, [](BOOL value) { CGetSetOptions::SetEnableOutputDebugStringLogging(value); } },
	{ SettingRefreshViewAfterPaste, [](BOOL value) { CGetSetOptions::SetRefreshViewAfterPasting(value); } },
	{ SettingSupportAllTypes, [](BOOL value) { CGetSetOptions::SetSupportAllTypes(value); } },
	{ SettingRegexCaseInsensitive, [](BOOL value) { CGetSetOptions::SetRegexCaseInsensitive(value); } },
	{ SettingDrawCopiedColorCode, [](BOOL value) { CGetSetOptions::SetDrawCopiedColorCode(value); } },
	{ SettingCenterWindowBelowCursorCaret, [](BOOL value) { CGetSetOptions::SetCenterWindowBelowCursorOrCaret(value); } },
	{ SettingUpdateDescOnClipEdit, [](BOOL value) { CGetSetOptions::SetUpdateDescWhenSavingClip(value); } },
	{ SettingUseUtf8ForDiff, [](BOOL value) { CGetSetOptions::SetPreferUtf8ForCompare(value); } },
	{ SettingDoNotHideOnDeactivate, [](BOOL value) { CGetSetOptions::SetDoNotHideOnDeactivate(value); } },
	{ SettingHideTaskbarIconOnClose, [](BOOL value) { CGetSetOptions::SetHideTaskbarIconOnClose(value); } },
	{ SettingEnforceClipboardIgnoreFormats, [](BOOL value) { CGetSetOptions::SetEnforceClipboardIgnoreFormats(value); } },
} };

const std::array<CAdvGeneral::TextSetting, 11> CAdvGeneral::s_textSettings{ {
	{ SettingClipSeparator, [](LPCTSTR value) { CGetSetOptions::SetMultiPasteSeparator(value); } },
	{ SettingCopyPlaySound, [](LPCTSTR value) { CGetSetOptions::SetPlaySoundOnCopy(value); } },
	{ SettingDiffApp, [](LPCTSTR value) { CGetSetOptions::SetDiffApp(value); } },
	{ SettingDefaultPasteString, [](LPCTSTR value) { CGetSetOptions::SetDefaultPasteString(value); } },
	{ SettingDefaultCopyString, [](LPCTSTR value) { CGetSetOptions::SetDefaultCopyString(value); } },
	{ SettingDefaultCutString, [](LPCTSTR value) { CGetSetOptions::SetDefaultCutString(value); } },
	{ SettingSlugifySeparator, [](LPCTSTR value) { CGetSetOptions::SetSlugifySeparator(value); } },
	{ SettingIgnoreAnnoyingCfDib, [](LPCTSTR value) { CGetSetOptions::SetIgnoreAnnoyingCFDIB(value); } },
	{ SettingTextEditorPath, [](LPCTSTR value) { CGetSetOptions::SetTextEditorPath(value); } },
	{ SettingImageEditorPath, [](LPCTSTR value) { CGetSetOptions::SetImageEditorPath(value); } },
	{ SettingRtfEditorPath, [](LPCTSTR value) { CGetSetOptions::SetRTFEditorPath(value); } },
} };

BOOL CAdvGeneral::OnInitDialog()
{
	CDialogEx::OnInitDialog();

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

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Activate window delay (100ms default)"), (long)CGetSetOptions::SendKeysDelay(), _T(""), SettingActivateWindowDelay));

	AddTrueFalse(pGroupTest, _T("Add file drop when dragging clips"), CGetSetOptions::GetAddCFHDROP_OnDrag(), SettingAddCfHdropOnDrag);

	AddTrueFalse(pGroupTest, _T("Allow duplicates"), CGetSetOptions::GetAllowDuplicates(), SettingAllowDuplicates);
	AddTrueFalse(pGroupTest, _T("Allow back to back duplicates (if allowing duplicates)"), CGetSetOptions::GetAllowBackToBackDuplicates(), SettingAllowBackToBackDuplicates);

	AddTrueFalse(pGroupTest, _T("Always show scroll bar"), CGetSetOptions::GetShowScrollBar(), SettingAlwaysShowScrollBar);
	AddTrueFalse(pGroupTest, _T("Use modern scroll bar"), CGetSetOptions::GetUseModernScrollBar(), SettingUseModernScrollbar);
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Amount of text to save for description"), CGetSetOptions::m_bDescTextSize, _T(""), SettingDescSize));
	AddTrueFalse(pGroupTest, _T("Center window below cursor or caret"), CGetSetOptions::GetCenterWindowBelowCursorOrCaret(), SettingCenterWindowBelowCursorCaret);
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Copy and save clipboard delay (ms)"), (long)CGetSetOptions::GetCopyAndSveDelay(), _T(""), SettingCopySaveDelay));

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Clip edit save delay after load"), (long)(CGetSetOptions::GetClipEditSaveDelayAfterLoadSeconds()), _T(""), SettingClipEditSaveDelayAfterLoad));
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Clip edit save delay after Save"), (long)(CGetSetOptions::GetClipEditSaveDelayAfterSaveSeconds()), _T(""), SettingClipEditSaveDelayAfterSave));

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Clipboard restore delay after copy buffer sent paste (ms, default: 750)"), (long)(CGetSetOptions::GetDittoRestoreClipboardDelay()), _T(""), SettingClipboardRestoreAfterCopyBufferDelay));

	CString defaultPasteString = CGetSetOptions::GetDefaultPasteString();
	CString defaultCopyString = CGetSetOptions::GetDefaultCopyString();
	CString defaultCutString = CGetSetOptions::GetDefaultCutString();
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Default paste string"), defaultPasteString, _T(""), SettingDefaultPasteString));
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Default copy string"), defaultCopyString, _T(""), SettingDefaultCopyString));
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Default cut string"), defaultCutString, _T(""), SettingDefaultCutString));
	
	static const TCHAR BASED_CODE szDiffFilter[] = _T("Diff Applications(*.exe)|*.exe||");
	CMFCPropertyGridFileProperty* pDiffProp = MakeGridProperty<CMFCPropertyGridFileProperty>(_T("Diff application path"), TRUE, CGetSetOptions::GetDiffApp(), _T("exe"), 0, szDiffFilter, (LPCTSTR)0, SettingDiffApp);
	pGroupTest->AddSubItem(pDiffProp);

	AddTrueFalse(pGroupTest, _T("Diff save compare files as utf8"), CGetSetOptions::GetPreferUtf8ForCompare(), SettingUseUtf8ForDiff);

	AddTrueFalse(pGroupTest, _T("Display icon in system tray"), CGetSetOptions::GetShowIconInSysTray(), SettingShowTaskbarIcon);

	AddTrueFalse(pGroupTest, _T("Do not hide Ditto window on deactivate"), CGetSetOptions::GetDoNotHideOnDeactivate(), SettingDoNotHideOnDeactivate);

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Double shortcut keystroke timeout)"), (long)CGetSetOptions::GetDoubleKeyStrokeTimeout(), _T(""), SettingDoubleKeystrokeTimeout));

	AddTrueFalse(pGroupTest, _T("Draw swatch for hex, RGB, and HSL colors"), CGetSetOptions::GetDrawCopiedColorCode(), SettingDrawCopiedColorCode);

	AddTrueFalse(pGroupTest, _T("Draw RTF text in list (for RTF types) (could increase memory usage an display speed)"), CGetSetOptions::GetDrawRTF(), SettingDrawRtf);
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Editor default font size"), (long)CGetSetOptions::GetEditorDefaultFontSize(), _T(""), SettingEditorFontSize));
	AddTrueFalse(pGroupTest, _T("Enforce clipboard ignore formats"), CGetSetOptions::GetEnforceClipboardIgnoreFormats(), SettingEnforceClipboardIgnoreFormats);
	AddTrueFalse(pGroupTest, _T("Elevated privileges to paste into elevated apps"), CGetSetOptions::GetPasteAsAdmin(), SettingPasteAsAdmin);
	AddTrueFalse(pGroupTest, _T("Ensure Ditto is always connected to the clipboard"), CGetSetOptions::GetEnsureConnectToClipboard(), SettingEnsureConnected);
	AddTrueFalse(pGroupTest, _T("Ensure entire window is visible"), CGetSetOptions::GetEnsureEntireWindowCanBeSeen(), SettingEnsureWindowIsVisible);

	AddTrueFalse(pGroupTest, _T("Fast thumbnails (True = fast / low quality (default). False = slow / high quality)"), CGetSetOptions::GetFastThumbnailMode(), SettingFastThumbnailMode);

	AddTrueFalse(pGroupTest, _T("Find as you type"), CGetSetOptions::GetFindAsYouType(), SettingFindAsType);

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("First ten hot keys start index"), (long)CGetSetOptions::GetFirstTenHotKeysStart(), _T(""), SettingFirstTenHotkeysStart));
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("First ten hot keys font size"), (long)CGetSetOptions::GetFirstTenHotKeysFontSize(), _T(""), SettingFirstTenHotkeysFontSize));

	AddTrueFalse(pGroupTest, _T("Hide Ditto on hot key if Ditto is visible"), CGetSetOptions::GetHideDittoOnHotKeyIfAlreadyShown(), SettingHideOnHotkeyIfVisible);

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Ignore copies faster than (ms) (default: 500)"), (long)CGetSetOptions::GetSaveClipDelay(), _T(""), SettingIgnoreFalseCopiesDelay));
	CString ignoreAnnoyingCFDIB = CGetSetOptions::GetIgnoreAnnoyingCFDIB();
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Ignore CF_DIB when a clip is detected as text content"), ignoreAnnoyingCFDIB, _T("Case insensitive. Recommended option is \"excel.exe; onenote.exe; powerpnt.exe\" "), SettingIgnoreAnnoyingCfDib));

	static const TCHAR BASED_CODE szImageEditorFilter[] = _T("Applications(*.exe)|*.exe||");
	CMFCPropertyGridFileProperty* pImageEditorProp = MakeGridProperty<CMFCPropertyGridFileProperty>(_T("Image editor path (empty for system mapping)"), TRUE, CGetSetOptions::GetImageEditorPath(), _T("exe"), 0, szImageEditorFilter, (LPCTSTR)0, SettingImageEditorPath);
	pGroupTest->AddSubItem(pImageEditorProp);

	pGroupTest->AddSubItem( MakeGridProperty<CMFCPropertyGridProperty>(_T("Maximum clip size in bytes (0 for no limit)"), CGetSetOptions::m_lMaxClipSizeInBytes, _T(""), SettingMaxClipSize));
		
	AddTrueFalse(pGroupTest, _T("Maintain search view"), CGetSetOptions::GetMaintainSearchView(), SettingMaintainSearchView);

	AddTrueFalse(pGroupTest, _T("Move selection on open hot key"), CGetSetOptions::GetMoveSelectionOnOpenHotkey(), SettingMoveSelectionOnOpenHotkey);
	
	CString multiPasteSeparator = CGetSetOptions::GetMultiPasteSeparator(false);
	pGroupTest->AddSubItem( MakeGridProperty<CMFCPropertyGridProperty>(_T("Multi-paste clip separator ([LF] = line feed)"), multiPasteSeparator, _T(""), SettingClipSeparator));

	AddTrueFalse(pGroupTest, _T("Multi-paste in reverse order"), CGetSetOptions::m_bMultiPasteReverse, SettingMultipasteReverseOrder);

	AddTrueFalse(pGroupTest, _T("Open to group same as active exe"), CGetSetOptions::GetOpenToGroupByActiveExe(), SettingOpenToGroupAsActiveExe);

	static const TCHAR BASED_CODE szFilter[] = _T("Sounds(*.wav)|*.wav||");
	CMFCPropertyGridFileProperty* pFileProp = MakeGridProperty<CMFCPropertyGridFileProperty>(_T("On copy play the sound"), TRUE, CGetSetOptions::GetPlaySoundOnCopy(), _T("wav"), 0, szFilter, (LPCTSTR)0, SettingCopyPlaySound);
	pGroupTest->AddSubItem(pFileProp);

	static const TCHAR BASED_CODE szTextEditorFilter[] = _T("Applications(*.exe)|*.exe||");
	CMFCPropertyGridFileProperty* pTextEditorProp = MakeGridProperty<CMFCPropertyGridFileProperty>(_T("Text editor path (empty for system mapping)"), TRUE, CGetSetOptions::GetTextEditorPath(), _T("exe"), 0, szTextEditorFilter, (LPCTSTR)0, SettingTextEditorPath);
	pGroupTest->AddSubItem(pTextEditorProp);

	AddTrueFalse(pGroupTest, _T("Paste clip in active window after selection"), CGetSetOptions::GetSendPasteAfterSelection(), SettingPasteInActiveWindow);	

	AddTrueFalse(pGroupTest, _T("Prompt when deleting clips"), CGetSetOptions::GetPromptWhenDeletingClips(), SettingPromptOnDelete);

	AddTrueFalse(pGroupTest, _T("Revert to top level group on close"), CGetSetOptions::GetRevertToTopLevelGroup(), SettingRevertToTopLevelGroup);

	AddTrueFalse(pGroupTest, _T("Refresh view after paste"), CGetSetOptions::GetRefreshViewAfterPasting(), SettingRefreshViewAfterPaste);

	AddTrueFalse(pGroupTest, _T("Regex case insensitive search"), CGetSetOptions::GetRegexCaseInsensitive(), SettingRegexCaseInsensitive);

	static const TCHAR BASED_CODE szRTFEditorFilter[] = _T("Applications(*.exe)|*.exe||");
	CMFCPropertyGridFileProperty* pRTFEditorProp = MakeGridProperty<CMFCPropertyGridFileProperty>(_T("RTF editor path"), TRUE, CGetSetOptions::GetRTFEditorPath(), _T("exe"), 0, szRTFEditorFilter, (LPCTSTR)0, SettingRtfEditorPath);
	pGroupTest->AddSubItem(pRTFEditorProp);

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Save clipboard delay (ms, default: 100)"), (long)(CGetSetOptions::GetProcessDrawClipboardDelay()), _T(""), SettingClipboardSaveDelay));

	AddTrueFalse(pGroupTest, _T("Save multi-pastes"), CGetSetOptions::GetSaveMultiPaste(), SettingSaveMultiPaste);

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Selected index"), (long)(CGetSetOptions::SelectedIndex()+1), _T(""), SettingSelectedIndex));

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Send keys delay (ms)"), (long)CGetSetOptions::RealSendKeysDelay(), _T(""), SettingSendKeysDelay));


	AddTrueFalse(pGroupTest, _T("Show clips that are in groups in main list"), CGetSetOptions::GetShowAllClipsInMainList(), SettingShowGroupClipsInList);
	AddTrueFalse(pGroupTest, _T("Show leading whitespace"), CGetSetOptions::GetDescShowLeadingWhiteSpace(), SettingShowLeadingWhitespace);
	AddTrueFalse(pGroupTest, _T("Show in taskbar"), CGetSetOptions::GetShowInTaskBar(), SettingShowInTaskbar);
	AddTrueFalse(pGroupTest, _T("Hide taskbar icon when Ditto window closes"), CGetSetOptions::GetHideTaskbarIconOnClose(), SettingHideTaskbarIconOnClose);
	AddTrueFalse(pGroupTest, _T("Show indicator a clip has been pasted"), CGetSetOptions::GetShowIfClipWasPasted(), SettingShowClipPasted);

	AddTrueFalse(pGroupTest, _T("Show startup tooltip message"), CGetSetOptions::GetShowStartupMessage(), SettingShowStartupMessage);

	AddTrueFalse(pGroupTest, _T("Show text for first ten copy hot keys"), CGetSetOptions::GetShowTextForFirstTenHotKeys(), SettingTextFirstTen);
	AddTrueFalse(pGroupTest, _T("Show thumbnails(for CF_DIB and PNG types) (could increase memory usage and display speed)"), CGetSetOptions::GetDrawThumbnail(), SettingDrawThumbnails);
	
	CString slugifySeparator = CGetSetOptions::GetSlugifySeparator();
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Slugify Separator (default: -)"), slugifySeparator, _T(""), SettingSlugifySeparator));

	AddTrueFalse(pGroupTest, _T("Support all types ignoring supported type list (default: false))"), CGetSetOptions::GetSupportAllTypes(), SettingSupportAllTypes);

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Text lines per clip"), CGetSetOptions::GetLinesPerRow(), _T(""), SettingLinesPerRow));

	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Tooltip display time(ms) max of 32000 (-1 default (5 seconds), 0 to turn off)"), CGetSetOptions::m_tooltipTimeout, _T(""), SettingTooltipTimeout));
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Tooltip maximum display lines"), (long)CGetSetOptions::GetMaxToolTipLines(), _T(""), SettingTooltipLines));
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Tooltip display characters"), (long)CGetSetOptions::GetMaxToolTipCharacters(), _T(""), SettingTooltipCharacters));

	AddTrueFalse(pGroupTest, _T("Transparency enabled"), CGetSetOptions::GetEnableTransparency(), SettingEnableTransparency);
	pGroupTest->AddSubItem(MakeGridProperty<CMFCPropertyGridProperty>(_T("Transparency percentage"), CGetSetOptions::GetTransparencyPercent(), _T(""), SettingTransparency));
	AddTrueFalse(pGroupTest, _T("Update description on clip edit"), CGetSetOptions::GetUpdateDescWhenSavingClip(), SettingUpdateDescOnClipEdit);
	AddTrueFalse(pGroupTest, _T("Update clip order on paste"), CGetSetOptions::GetUpdateTimeOnPaste(), SettingUpdateOrderOnPaste);
	AddTrueFalse(pGroupTest, _T("Update clip Order on ctrl-c"), CGetSetOptions::GetUpdateClipOrderOnCtrlC(), SettingUpdateOrderOnCtrlC);

	AddTrueFalse(pGroupTest, _T("Write debug to file"), CGetSetOptions::GetEnableDebugLogging(), SettingDebugToFile);
	AddTrueFalse(pGroupTest, _T("Write debug to OutputDebugString"), CGetSetOptions::GetEnableDebugLogging(), SettingDebugToOutputString);

	CMFCPropertyGridProperty * regexFilterGroup = MakeGridProperty<CMFCPropertyGridProperty>(_T("Exclude clips by Regular Expressions"));
	m_propertyGrid.AddProperty(regexFilterGroup);

	CString processFilterDesc = _T("Process making the copy first must match this before the Regex will be applied (empty or * for all processes) (separate multiples by ;)");
	CString regexFilterDesc = _T("If copied text matches this regular expression then the clip will not be saved to Ditto");

	CString regexFilter1 = CGetSetOptions::GetRegexFilter(0);
	CString regexProcessName1 = CGetSetOptions::GetRegexFilterByProcessName(0);
	CString regexFilter2 = CGetSetOptions::GetRegexFilter(1);
	CString regexProcessName2 = CGetSetOptions::GetRegexFilterByProcessName(1);
	CString regexFilter3 = CGetSetOptions::GetRegexFilter(2);
	CString regexProcessName3 = CGetSetOptions::GetRegexFilterByProcessName(2);
	CString regexFilter4 = CGetSetOptions::GetRegexFilter(3);
	CString regexProcessName4 = CGetSetOptions::GetRegexFilterByProcessName(3);
	CString regexFilter5 = CGetSetOptions::GetRegexFilter(4);
	CString regexProcessName5 = CGetSetOptions::GetRegexFilterByProcessName(4);
	CString regexFilter6 = CGetSetOptions::GetRegexFilter(5);
	CString regexProcessName6 = CGetSetOptions::GetRegexFilterByProcessName(5);
	CString regexFilter7 = CGetSetOptions::GetRegexFilter(6);
	CString regexProcessName7 = CGetSetOptions::GetRegexFilterByProcessName(6);
	CString regexFilter8 = CGetSetOptions::GetRegexFilter(7);
	CString regexProcessName8 = CGetSetOptions::GetRegexFilterByProcessName(7);
	CString regexFilter9 = CGetSetOptions::GetRegexFilter(8);
	CString regexProcessName9 = CGetSetOptions::GetRegexFilterByProcessName(8);
	CString regexFilter10 = CGetSetOptions::GetRegexFilter(9);
	CString regexProcessName10 = CGetSetOptions::GetRegexFilterByProcessName(9);
	CString regexFilter11 = CGetSetOptions::GetRegexFilter(10);
	CString regexProcessName11 = CGetSetOptions::GetRegexFilterByProcessName(10);
	CString regexFilter12 = CGetSetOptions::GetRegexFilter(11);
	CString regexProcessName12 = CGetSetOptions::GetRegexFilterByProcessName(11);
	CString regexFilter13 = CGetSetOptions::GetRegexFilter(12);
	CString regexProcessName13 = CGetSetOptions::GetRegexFilterByProcessName(12);
	CString regexFilter14 = CGetSetOptions::GetRegexFilter(13);
	CString regexProcessName14 = CGetSetOptions::GetRegexFilterByProcessName(13);
	CString regexFilter15 = CGetSetOptions::GetRegexFilter(14);
	CString regexProcessName15 = CGetSetOptions::GetRegexFilterByProcessName(14);

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

	if (const LongSetting* longSetting = FindSetting(s_longSettings, id))
	{
		if (pNewValue->lVal != pOrigValue->lVal)
		{
			longSetting->write(pNewValue->lVal);
		}
	}
	else if (const BoolSetting* boolSetting = FindSetting(s_boolSettings, id))
	{
		if (wcscmp(pNewValue->bstrVal, pOrigValue->bstrVal) != 0)
		{
			BOOL val = wcscmp(pNewValue->bstrVal, L"True") == 0;
			boolSetting->write(val);
		}
	}
	else if (const TextSetting* textSetting = FindSetting(s_textSettings, id))
	{
		if (wcscmp(pNewValue->bstrVal, pOrigValue->bstrVal) != 0)
		{
			textSetting->write(pNewValue->bstrVal);
		}
	}
	else
	{
		WriteRegexSetting(id, *pNewValue, *pOrigValue);
	}
}

void CAdvGeneral::WriteTransparencyPercent(long newValue)
{
	int value = 100;
	if (newValue <= 100 && newValue > 0)
	{
		value = newValue;
	}

	CGetSetOptions::SetTransparencyPercent(value);
}

void CAdvGeneral::WriteRegexSetting(int id, const VARIANT& newValue, const VARIANT& origValue)
{
	if (id >= SettingRegexFiltering1 && id <= SettingRegexFiltering15)
	{
		if (wcscmp(newValue.bstrVal, origValue.bstrVal) != 0)
		{
			CGetSetOptions::SetRegexFilter(newValue.bstrVal, (id - SettingRegexFiltering1));
		}
	}
	else if (id >= SettingRegexFilteringByProcessName1 && id <= SettingRegexFilteringByProcessName15)
	{
		if (wcscmp(newValue.bstrVal, origValue.bstrVal) != 0)
		{
			CGetSetOptions::SetRegexFilterByProcessName(newValue.bstrVal, (id - SettingRegexFilteringByProcessName1));
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
	auto msg = theApp.m_Language.GetString("CompactRepairWarning", "Warning this can take quite a long time and require up to double the hard drive space as your current database size, Continue?");
	int ret = MessageBox(msg, _T("Ditto"), MB_OKCANCEL);

	if (ret == IDOK)
	{
		CWaitCursor wait;

		try
		{
			try
			{
				for (int i = 0; i < 100; i++)
				{
					int toDeleteCount = theApp.m_db.execScalar(_T("SELECT COUNT(clipID) FROM MainDeletes"));
					if (toDeleteCount <= 0)
						break;

					CClipRetentionPolicy::RemoveOldEntries(false);
				}
			}
			catch (CppSQLite3Exception& e)
			{
				CErrorReport::Show(CStringUtil::Format(_T("Compact and repair failed while removing deleted clips, the database was not compacted: %s"), e.errorMessage()));
				return;
			}

			theApp.m_db.execDML(_T("PRAGMA auto_vacuum = 1"));
			theApp.m_db.execQuery(_T("VACUUM"));
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
			theApp.m_db.execDML(reOrderSql);
		}
		catch (CppSQLite3Exception& e)
		{
			MessageBox(e.errorMessage());
		}
	}
}
