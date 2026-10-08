// DeleteClipData.cpp : implementation file
//

#include "stdafx.h"
#include "FileDialogPath.h"
#include "CP_Main.h"
#include "DeleteClipData.h"
#include "ClipboardFormatError.h"
#include "ControlTextBuffer.h"
#include "ErrorReport.h"
#include "DittoDbTransaction.h"
#include "afxdialogex.h"
#include "Misc.h"
#include "ProgressWnd.h"
#include <algorithm>
#include <memory>
#include "../Shared/TextConvert.h"
#include "../resource.h"
#include "CopyProperties.h"
#include "DimWnd.h"

// CDeleteClipData dialog

IMPLEMENT_DYNAMIC(CDeleteClipData, CDialog)

CDeleteClipData::CDeleteClipData(CWnd* pParent /*=NULL*/) :
	CDialog(CDeleteClipData::IDD, pParent),
	m_showTaskbar(theApp.Services().Windows(), theApp.Services().State()),
	m_pDescriptionWindow(nullptr),
	m_clipTitle(_T("")),
	m_filterByClipTitle(FALSE),
	m_filterByCreatedDate(FALSE),
	m_filterByLastUsedDate(FALSE),
	m_filterByClipboardFormat(FALSE),
	m_createdDateStart(COleDateTime::GetCurrentTime()),
	m_createdDateEnd(COleDateTime::GetCurrentTime()),
	m_createdTimeStart(COleDateTime::GetCurrentTime()),
	m_createdTimeEnd(COleDateTime::GetCurrentTime()),
	m_usedTimeStart(COleDateTime::GetCurrentTime()),
	m_usedTimeEnd(COleDateTime::GetCurrentTime()),
	m_usedDateStart(COleDateTime::GetCurrentTime()),
	m_usedDateEnd(COleDateTime::GetCurrentTime()),
	m_databaseSize(_T("")),
	m_selectedSize(_T("")),
	m_selectedCount(_T(""))
{
	m_applyingDelete = false;
	m_cancelDelete = false;
}

CDeleteClipData::~CDeleteClipData()
{
}

void CDeleteClipData::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST2, m_clipList);
	DDX_Text(pDX, IDC_EDIT_CLIP_TITLE, m_clipTitle);
	DDX_Check(pDX, IDC_CHECK_CLIP_TITLE, m_filterByClipTitle);
	DDX_Check(pDX, IDC_CHECK_CREATE_DATE, m_filterByCreatedDate);
	DDX_Check(pDX, IDC_CHECK_LAST_USE_DATE, m_filterByLastUsedDate);
	DDX_Check(pDX, IDC_CHECK_DATA_FORMAT, m_filterByClipboardFormat);
	DDX_Control(pDX, IDC_COMBO_DATA_FORMAT, m_clipboardFomatCombo);
	DDX_DateTimeCtrl(pDX, IDC_DATE_CREATE_START, m_createdDateStart);
	DDX_DateTimeCtrl(pDX, IDC_DATE_CREATE_END, m_createdDateEnd);
	DDX_DateTimeCtrl(pDX, IDC_TIME_CREATE_START, m_createdTimeStart);
	DDX_DateTimeCtrl(pDX, IDC_TIME_CREATE_END, m_createdTimeEnd);
	DDX_DateTimeCtrl(pDX, IDC_TIME_USE_START, m_usedTimeStart);
	DDX_DateTimeCtrl(pDX, IDC_TIME_USE_END, m_usedTimeEnd);
	DDX_DateTimeCtrl(pDX, IDC_DATE_USE_START, m_usedDateStart);
	DDX_DateTimeCtrl(pDX, IDC_DATE_USE_END, m_usedDateEnd);
	DDX_Text(pDX, IDC_STATIC_DB_SIZE, m_databaseSize);
	DDX_Text(pDX, IDC_STATIC_SELECTED_SIZE, m_selectedSize);
	DDX_Text(pDX, IDC_STATIC_SELECTED_COUNT, m_selectedCount);
}


BEGIN_MESSAGE_MAP(CDeleteClipData, CDialog)
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_WM_NCDESTROY()
	ON_BN_CLICKED(IDC_BUTTON_SEARCH, &CDeleteClipData::OnBnClickedButtonSearch)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST2, &CDeleteClipData::OnLvnKeydownList2)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST2, &CDeleteClipData::OnLvnItemchangedList2)
	ON_NOTIFY(HDN_GETDISPINFO, 0, &CDeleteClipData::OnHdnGetdispinfoList2)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_LIST2, &CDeleteClipData::OnLvnGetdispinfoList2)
	ON_BN_CLICKED(IDC_CHECK_CLIP_TITLE, &CDeleteClipData::OnBnClickedCheckClipTitle)
	ON_BN_CLICKED(IDC_BUTTON_APPLY, &CDeleteClipData::OnBnClickedButtonApply)
	ON_BN_CLICKED(IDCLOSE, &CDeleteClipData::OnBnClickedClose)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CHECK_CREATE_DATE, &CDeleteClipData::OnBnClickedCheckCreateDate)
	ON_BN_CLICKED(IDC_CHECK_LAST_USE_DATE, &CDeleteClipData::OnBnClickedCheckLastUseDate)
	ON_BN_CLICKED(IDC_CHECK_DATA_FORMAT, &CDeleteClipData::OnBnClickedCheckDataFormat)
	ON_NOTIFY(HDN_ITEMCLICK, 0, &CDeleteClipData::OnLvnColumnclickList2)

	ON_WM_CONTEXTMENU()
	ON_BN_CLICKED(IDC_BT_COMPACT_AND_REPAIR, &CDeleteClipData::OnBnClickedBtCompactAndRepair)
END_MESSAGE_MAP()

BOOL CDeleteClipData::OnInitDialog()
{
	CDialog::OnInitDialog();

	theApp.Services().Language().UpdateDeleteClipData(this);

	m_Resize.SetParent(m_hWnd);
	m_Resize.AddControl(IDC_LIST2, CDialogResizer::SizeHeight | CDialogResizer::SizeWidth);
	m_Resize.AddControl(IDCLOSE, CDialogResizer::MoveTop | CDialogResizer::MoveLeft);
	m_Resize.AddControl(IDC_BUTTON_APPLY, CDialogResizer::MoveTop | CDialogResizer::MoveLeft);
	m_Resize.AddControl(IDC_STATIC_TO_DELETE_TEXT, CDialogResizer::MoveTop);
	m_Resize.AddControl(IDC_STATIC_TO_DELETE_SIZE, CDialogResizer::MoveTop);
	m_Resize.AddControl(IDC_STATIC_SELECTED_SIZE, CDialogResizer::MoveTop);
	m_Resize.AddControl(IDC_STATIC_SELECTED_SIZE_TEXT, CDialogResizer::MoveTop);
	m_Resize.AddControl(IDC_STATIC_DB_SIZE, CDialogResizer::MoveTop);
	m_Resize.AddControl(IDC_STATIC_DB_SIZE_TEXT, CDialogResizer::MoveTop);
	m_Resize.AddControl(IDC_BUTTON_SEARCH, CDialogResizer::MoveLeft);
	m_Resize.AddControl(IDC_STATIC_GROUP_SEARCH, CDialogResizer::SizeWidth);
	m_Resize.AddControl(IDC_BT_COMPACT_AND_REPAIR, CDialogResizer::MoveTop | CDialogResizer::MoveLeft);

	InitListCtrlCols();

	SetTimer(1, 500, 0);

	SetDbSize();

	return TRUE;
}

void CDeleteClipData::SetDbSize()
{
	__int64 size = CFileSystem::FileSize(CDatabaseManager::GetDBName(theApp.Services().Settings()));

	const int MAX_FILE_SIZE_BUFFER = 255;
	TCHAR szFileSize[MAX_FILE_SIZE_BUFFER];
	StrFormatByteSize(size, szFileSize, MAX_FILE_SIZE_BUFFER);

	m_databaseSize = szFileSize;
	UpdateData(0);
}

void CDeleteClipData::InitListCtrlCols()
{
	m_clipList.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	m_clipList.InsertColumn(0, theApp.Services().Language().GetDeleteClipDataString("ID", "ID"), LVCFMT_LEFT, 50);
	m_clipList.InsertColumn(1, theApp.Services().Language().GetDeleteClipDataString("Title", "Title"), LVCFMT_LEFT, 350);
	m_clipList.InsertColumn(2, theApp.Services().Language().GetDeleteClipDataString("QuickPasteText", "Quick Paste Text"), LVCFMT_LEFT, 200);
	m_clipList.InsertColumn(3, theApp.Services().Language().GetDeleteClipDataString("Created", "Created"), LVCFMT_LEFT, 150);
	m_clipList.InsertColumn(4, theApp.Services().Language().GetDeleteClipDataString("LastUsed", "Last Used"), LVCFMT_LEFT, 150);
	m_clipList.InsertColumn(5, theApp.Services().Language().GetDeleteClipDataString("Format", "Format"), LVCFMT_LEFT, 150);
	m_clipList.InsertColumn(6, theApp.Services().Language().GetDeleteClipDataString("DataSize", "Data Size"), LVCFMT_LEFT, 100);
}

void CDeleteClipData::LoadItems()
{
	CWaitCursor wait;
	m_data.clear();
	m_filteredOut.clear();

	if (m_clipboardFomatCombo.GetCount() == 0)
	{
		CppSQLite3Query qFormats = theApp.Services().Database().execQueryEx(_T("select DISTINCT(strClipBoardFormat) from Data"));
		while (qFormats.eof() == false)
		{
			CString format = qFormats.getStringField(_T("strClipBoardFormat"));
			m_clipboardFomatCombo.AddString(format);

			qFormats.nextRow();
		}
	}

	CppSQLite3Query q = theApp.Services().Database().execQueryEx(_T("SELECT Main.lID, Main.mText, Main.lDate, Main.lastPasteDate, Main.QuickPasteText, Data.lID AS DataID, Data.strClipBoardFormat, length(Data.ooData) AS DataLength ")
																 _T("FROM Data ")
																 _T("INNER JOIN Main on Main.lID = Data.lParentID ")
																 _T("ORDER BY length(ooData) DESC"));

	int row = 0;
	while (q.eof() == false)
	{
		CDeleteData data;
		data.m_lID = q.getIntField(_T("lID"));
		data.m_Desc = q.getStringField(_T("mText"));
		data.m_createdDateTime = q.getInt64Field(_T("lDate"));
		data.m_lastUsedDateTime = q.getInt64Field(_T("lastPasteDate"));
		data.m_clipboardFormat = q.getStringField(_T("strClipBoardFormat"));
		data.m_dataSize = q.getIntField(_T("DataLength"));
		data.m_DatalID = q.getIntField(_T("DataID"));
		data.m_quickPasteText = q.getStringField(_T("QuickPasteText"));

		m_data.push_back(data);

		row++;

		q.nextRow();
	}

	m_clipList.SetItemCountEx(row, 0);
}

void CDeleteClipData::SetNotifyWnd(HWND hWnd)
{
	m_hWndParent = hWnd;
}

void CDeleteClipData::OnClose()
{
	if (m_applyingDelete)
	{
		return;
	}

	CloseDescriptionWindow();
	DestroyWindow();
}

void CDeleteClipData::CloseDescriptionWindow()
{
	if (IsDescriptionWindowValid())
	{
		m_pDescriptionWindow->CloseWindow();
		// DestroyWindow deletes the object too (CToolTipEx::PostNcDestroy)
		m_pDescriptionWindow->DestroyWindow();
	}
	m_pDescriptionWindow = nullptr;
	m_descriptionWindowHwnd = NULL;
}

bool CDeleteClipData::IsDescriptionWindowValid() const
{
	return m_pDescriptionWindow != nullptr &&
		   ::IsWindow(m_descriptionWindowHwnd) &&
		   CWnd::FromHandlePermanent(m_descriptionWindowHwnd) == m_pDescriptionWindow;
}

void CDeleteClipData::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	m_Resize.MoveControls(CSize(cx, cy));
}

void CDeleteClipData::OnNcDestroy()
{
	CDialog::OnNcDestroy();
	::PostMessage(m_hWndParent, CDittoMessage::DeleteClipsClosed, 0, 0);
}

// CDeleteClipData message handlers


void CDeleteClipData::OnBnClickedButtonSearch()
{
	FilterItems();
}

void CDeleteClipData::FilterItems()
{
	if (IsDescriptionWindowValid())
	{
		m_pDescriptionWindow->Hide();
	}

	UpdateData();

	//First search the already filtered text, see if we need to add them back in
	std::vector<int> filteredRowsToDelete;
	std::vector<CDeleteData> addBackIn;
	FindFilteredOutMatches(addBackIn, filteredRowsToDelete);

	//next search the main list
	std::vector<int> rowsToDelete;
	INT_PTR count = m_data.size();
	for (int i = 0; i < count; i++)
	{
		CDeleteData data = m_data[i];

		if (MatchesFilter(&data) == false)
		{
			m_filteredOut.push_back(data);
			rowsToDelete.push_back(i);
		}
	}

	//Add back in the filtered out ones that now match
	count = addBackIn.size();
	for (int i = 0; i < count; i++)
	{
		CDeleteData data = addBackIn[i];
		m_data.push_back(data);
	}

	int toSelect = -1;

	//Remove from the main list the ones that don't match
	count = rowsToDelete.size();
	for (INT_PTR i = count - 1; i >= 0; i--)
	{
		int row = rowsToDelete[i];
		toSelect = row;

		m_data.erase(m_data.begin() + row);
	}

	//Remove the rows that were filtered out but now match
	count = filteredRowsToDelete.size();
	for (INT_PTR i = count - 1; i >= 0; i--)
	{
		int row = filteredRowsToDelete[i];
		m_filteredOut.erase(m_filteredOut.begin() + row);
	}

	if (toSelect > -1)
	{
		m_clipList.SetItemState(toSelect, LVIS_SELECTED, LVIS_SELECTED);
	}

	m_clipList.SetItemCountEx((int)m_data.size(), 0);
}

void CDeleteClipData::FindFilteredOutMatches(std::vector<CDeleteData>& addBackIn, std::vector<int>& filteredRowsToDelete)
{
	INT_PTR count = m_filteredOut.size();
	for (int i = 0; i < count; i++)
	{
		CDeleteData data = m_filteredOut[i];

		if (MatchesFilter(&data))
		{
			addBackIn.push_back(data);
			filteredRowsToDelete.push_back(i);
		}
	}
}

bool CDeleteClipData::MatchesFilter(CDeleteData* pdata)
{
	if (IsRejectedByTitle(pdata))
	{
		return false;
	}

	if (m_filterByCreatedDate)
	{
		return IsInDateRange(pdata->m_createdDateTime, m_createdDateStart, m_createdTimeStart, m_createdDateEnd, m_createdTimeEnd);
	}

	if (m_filterByLastUsedDate)
	{
		return IsInDateRange(pdata->m_lastUsedDateTime, m_usedDateStart, m_usedTimeStart, m_usedDateEnd, m_usedTimeEnd);
	}

	if (m_filterByClipboardFormat)
	{
		return MatchesSelectedFormat(pdata);
	}


	return true;
}

bool CDeleteClipData::IsRejectedByTitle(const CDeleteData* pdata) const
{
	if (m_filterByClipTitle &&
		m_clipTitle != _T("") &&
		pdata->m_Desc != _T(""))
	{
		// compare lower-case copies: the item's description and the typed title keep their case
		CString description{ pdata->m_Desc };
		CString title{ m_clipTitle };
		if (description.MakeLower().Find(title.MakeLower()) == -1)
		{
			return true;
		}
	}

	return false;
}

bool CDeleteClipData::IsInDateRange(const CTime& value, const COleDateTime& startDate, const COleDateTime& startTime, const COleDateTime& endDate, const COleDateTime& endTime)
{
	CTime dateStart = CTime(startDate.GetYear(), startDate.GetMonth(), startDate.GetDay(), startTime.GetHour(), startTime.GetMinute(), startTime.GetSecond());
	CTime dateEnd = CTime(endDate.GetYear(), endDate.GetMonth(), endDate.GetDay(), endTime.GetHour(), endTime.GetMinute(), endTime.GetSecond());

	return value >= dateStart && value <= dateEnd;
}

bool CDeleteClipData::MatchesSelectedFormat(const CDeleteData* pdata)
{
	const int selection{ m_clipboardFomatCombo.GetCurSel() };
	if (selection == CB_ERR)
	{
		// no format chosen yet: no item has the (empty) selected format
		return false;
	}

	CString selectedFormat;
	m_clipboardFomatCombo.GetLBText(selection, selectedFormat);

	return pdata->m_clipboardFormat == selectedFormat;
}

void CDeleteClipData::OnLvnKeydownList2(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here

	switch (pLVKeyDow->wVKey)
	{
	case VK_DELETE:
		this->ApplyDelete();
		break;
	case VK_RETURN:
	{
		if (GetKeyState(VK_MENU) & 0x8000) // Check if Alt is also pressed
		{
			ShowClipPropertiesWindow();
			*pResult = 1;
		}
	}
	break;
	case VK_F3:
	{
		CreateAndShowDescriptionWindow();
		*pResult = 1;
	}
	break;
	case 'N':
	{
		SelectNextRowAndDescribe();
		*pResult = 1;
	}
	break;
	case 'P':
	{
		SelectPreviousRowAndDescribe();
		*pResult = 1;
	}
	break;
	default:
		*pResult = 0;
		break;
	}
}

void CDeleteClipData::SelectNextRowAndDescribe()
{
	int nSelItem = m_clipList.GetNextItem(-1, LVNI_SELECTED);
	if (nSelItem != -1 && nSelItem < m_clipList.GetItemCount() - 1)
	{
		SelectRow(nSelItem + 1);
		CreateAndShowDescriptionWindow();
	}
}

void CDeleteClipData::SelectPreviousRowAndDescribe()
{
	int nSelItem = m_clipList.GetNextItem(-1, LVNI_SELECTED);
	if (nSelItem != -1 && nSelItem > 0)
	{
		SelectRow(nSelItem - 1);
		CreateAndShowDescriptionWindow();
	}
}

void CDeleteClipData::OnLvnItemchangedList2(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	POSITION pos = m_clipList.GetFirstSelectedItemPosition();
	__int64 selectedDataSize = 0;
	int selectedCount = 0;
	bool setDescriptionWindowText = false;

	int nCaretItem = m_clipList.GetNextItem(-1, LVNI_FOCUSED);

	if (pos != nullptr)
	{
		while (pos)
		{
			INT_PTR row = m_clipList.GetNextSelectedItem(pos);

			if (row >= 0 && row < (INT_PTR)m_data.size())
			{
				selectedDataSize += m_data[row].m_dataSize;
				selectedCount++;

				if (row == nCaretItem &&
					setDescriptionWindowText == false &&
					IsDescriptionWindowValid() &&
					m_pDescriptionWindow->IsWindowVisible())
				{
					SetDescriptionWindowText(row);

					CRect r;
					m_pDescriptionWindow->GetWindowRectEx(r);
					CPoint pt;
					pt = r.TopLeft();

					m_pDescriptionWindow->Show(pt);

					setDescriptionWindowText = true;
				}
			}
		}
	}

	const int MAX_FILE_SIZE_BUFFER = 255;
	TCHAR szFileSize[MAX_FILE_SIZE_BUFFER];
	StrFormatByteSize(selectedDataSize, szFileSize, MAX_FILE_SIZE_BUFFER);

	m_selectedSize = szFileSize;

	CString count;
	count.Format(_T("%d"), selectedCount);

	m_selectedCount = count;

	UpdateData(0);

	*pResult = 0;
}


void CDeleteClipData::OnHdnGetdispinfoList2(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMHDDISPINFO pDispInfo = reinterpret_cast<LPNMHDDISPINFO>(pNMHDR);

	if (pDispInfo->mask & LVIF_TEXT)
	{
		switch (pDispInfo->iItem)
		{
		case 0:
			break;
		}
	}

	*pResult = 0;
}


void CDeleteClipData::OnLvnGetdispinfoList2(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVDISPINFO* pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	if (pDispInfo->item.mask & LVIF_TEXT)
	{
		if (pDispInfo->item.iItem >= 0 && static_cast<size_t>(pDispInfo->item.iItem) < m_data.size())
		{
			CopyColumnText(pDispInfo->item);
		}
	}
	*pResult = 0;
}

void CDeleteClipData::CopyColumnText(LVITEM& item)
{
	switch (item.iSubItem)
	{
	case 0:
	{
		CopyDisplayText(item, CStringUtil::Format(_T("%d"), m_data[item.iItem].m_lID));
	}
	break;
	case 1:
	{
		CopyDisplayText(item, m_data[item.iItem].m_Desc);
	}
	break;
	case 2:
	{
		CopyDisplayText(item, m_data[item.iItem].m_quickPasteText);
	}
	break;
	case 3:
	{
		COleDateTime dtTime(m_data[item.iItem].m_createdDateTime.GetTime());
		CopyDisplayText(item, dtTime.Format());
	}
	break;
	case 4:
	{
		COleDateTime dtTime(m_data[item.iItem].m_lastUsedDateTime.GetTime());
		CopyDisplayText(item, dtTime.Format());
	}
	break;
	case 5:
	{
		CopyDisplayText(item, m_data[item.iItem].m_clipboardFormat);
	}
	break;
	case 6:
	{
		const int MAX_FILE_SIZE_BUFFER = 255;
		TCHAR szFileSize[MAX_FILE_SIZE_BUFFER];
		StrFormatByteSize(m_data[item.iItem].m_dataSize, szFileSize, MAX_FILE_SIZE_BUFFER);

		CopyDisplayText(item, szFileSize);
	}
	break;
	}
}

void CDeleteClipData::CopyDisplayText(LVITEM& item, LPCTSTR text)
{
	CControlTextBuffer::CopyCut(item.pszText, item.cchTextMax, text);
}


void CDeleteClipData::OnBnClickedCheckClipTitle()
{
	UpdateData();
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_CLIP_TITLE), m_filterByClipTitle);
	::SetFocus(::GetDlgItem(m_hWnd, IDC_EDIT_CLIP_TITLE));
}


void CDeleteClipData::OnBnClickedButtonApply()
{
	ApplyDelete();
}

void CDeleteClipData::ApplyDelete()
{
	if (m_applyingDelete)
		return;

	if (MessageBox(_T("Delete selected items?  This cannot be undone!"), _T(""), MB_OKCANCEL | MB_ICONWARNING) == IDOK)
	{
		m_clipList.EnableWindow(FALSE);
		m_applyingDelete = true;
		m_cancelDelete = false;

		CWaitCursor wait;

		try
		{
			POSITION pos = m_clipList.GetFirstSelectedItemPosition();
			std::vector<int> rowsToDelete;

			if (pos != nullptr)
			{
				while (pos)
				{
					int row = m_clipList.GetNextSelectedItem(pos);
					rowsToDelete.push_back(row);
				}
			}

			CProgressWnd progress;
			progress.Create(this, _T("Deleting clip items"), TRUE);
			progress.SetRange(0, (int)rowsToDelete.size() + 2);
			progress.SetText(_T("Deleting selected items"));
			progress.SetStep(1);

			DeleteRows(rowsToDelete, progress);

			progress.StepIt();
			progress.SetText(_T("Refreshing database size"));
			SetDbSize();

			progress.StepIt();
			progress.SetText(_T("Applying filter"));
			FilterItems();

			m_clipList.SetItemCountEx((int)m_data.size(), 0);

			POSITION selectedPos = m_clipList.GetFirstSelectedItemPosition();
			if (selectedPos != nullptr)
			{
				INT_PTR row = m_clipList.GetNextSelectedItem(selectedPos);
				SelectRow((int)row);
			}
		}
		catch (CppSQLite3Exception& e)
		{
			// the operation stops here; the lines below only re-enable the dialog
			CErrorReport::Show(CStringUtil::Format(_T("Deleting the selected clip items failed: %s"), e.errorMessage()));
		}

		m_applyingDelete = false;
		m_clipList.EnableWindow();
		m_clipList.SetFocus();
	}
}

void CDeleteClipData::DeleteRows(const std::vector<int>& rowsToDelete, CProgressWnd& progress)
{
	INT_PTR count{ static_cast<INT_PTR>(rowsToDelete.size()) };
	for (INT_PTR i = count - 1; i >= 0; i--)
	{
		progress.PeekAndPump();
		if (m_cancelDelete || progress.Cancelled())
		{
			break;
		}
		progress.StepIt();

		int row{ rowsToDelete[i] };

		CDeleteData data{ m_data[row] };
		try
		{
			// one transaction per item: upstream ran the two deletes without one, so a failed second
			// delete left the item half deleted (rolled back now, and the item stays in the list)
			CDittoDbTransaction transaction(theApp.Services().Database());
			theApp.Services().Database().execDMLEx(_T("DELETE FROM Data where lID = %d"), data.m_DatalID);

			//If there are no more children for this clip then delete the parent
			theApp.Services().Database().execDMLEx(_T("DELETE FROM Main where lID IN ")
												   _T("(")
												   _T("SELECT Main.lID ")
												   _T("FROM Main ")
												   _T("LEFT OUTER JOIN Data on Data.lParentID = Main.lID ")
												   _T("WHERE bIsGroup = 0 AND Main.lID = %d ")
												   _T("Group by Main.lID ")
												   _T("having Count(Data.lID) = 0 ")
												   _T(")"),
												   data.m_lID);
			transaction.Commit();

			m_data.erase(m_data.begin() + row);
		}
		catch (CppSQLite3Exception& e)
		{
			CErrorReport::Show(CStringUtil::Format(_T("Deleting clip data id %ld (clip id %ld) failed, the remaining items were not deleted: %s"), data.m_DatalID, data.m_lID, e.errorMessage()));
			// stop deleting; the refresh below still shows the items deleted so far
			break;
		}
	}
}

void CDeleteClipData::OnBnClickedClose()
{
	if (m_applyingDelete)
	{
		m_cancelDelete = true;
		return;
	}
	CloseDescriptionWindow();
	DestroyWindow();
}

void CDeleteClipData::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case 1:
		LoadItems();
		KillTimer(1);
		break;
	}

	CDialog::OnTimer(nIDEvent);
}


void CDeleteClipData::OnBnClickedCheckCreateDate()
{
	UpdateData();
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_DATE_CREATE_START), m_filterByCreatedDate);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_TIME_CREATE_START), m_filterByCreatedDate);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_DATE_CREATE_END), m_filterByCreatedDate);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_TIME_CREATE_END), m_filterByCreatedDate);
	::SetFocus(::GetDlgItem(m_hWnd, IDC_DATE_CREATE_START));
}


void CDeleteClipData::OnBnClickedCheckLastUseDate()
{
	UpdateData();
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_DATE_USE_START), m_filterByLastUsedDate);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_TIME_USE_START), m_filterByLastUsedDate);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_DATE_USE_END), m_filterByLastUsedDate);
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_TIME_USE_END), m_filterByLastUsedDate);
	::SetFocus(::GetDlgItem(m_hWnd, IDC_DATE_USE_START));
}


void CDeleteClipData::OnBnClickedCheckDataFormat()
{
	UpdateData();
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_COMBO_DATA_FORMAT), m_filterByClipboardFormat);
	::SetFocus(::GetDlgItem(m_hWnd, IDC_COMBO_DATA_FORMAT));
}

bool CDeleteClipData::SortByIDDesc(const CDeleteData& a1, const CDeleteData& a2)
{
	return a1.m_lID > a2.m_lID;
}
bool CDeleteClipData::SortByIDAsc(const CDeleteData& a1, const CDeleteData& a2)
{
	return a1.m_lID < a2.m_lID;
}


bool CDeleteClipData::SortByTitleDesc(const CDeleteData& a1, const CDeleteData& a2)
{
	return a1.m_Desc > a2.m_Desc;
}
bool CDeleteClipData::SortByTitleAsc(const CDeleteData& a1, const CDeleteData& a2)
{
	return a1.m_Desc < a2.m_Desc;
}


bool CDeleteClipData::SortByQuickPaste(const CDeleteData& a1, const CDeleteData& a2)
{
	return a1.m_quickPasteText > a2.m_quickPasteText;
}

bool CDeleteClipData::SortByCreatedDateDesc(const CDeleteData& a1, const CDeleteData& a2)
{
	return a1.m_createdDateTime > a2.m_createdDateTime;
}

bool CDeleteClipData::SortByLastUsedDateDesc(const CDeleteData& a1, const CDeleteData& a2)
{
	return a1.m_lastUsedDateTime > a2.m_lastUsedDateTime;
}

bool CDeleteClipData::SortByFormatDesc(const CDeleteData& a1, const CDeleteData& a2)
{
	return a1.m_clipboardFormat > a2.m_clipboardFormat;
}

bool CDeleteClipData::SortByDataSizeDesc(const CDeleteData& a1, const CDeleteData& a2)
{
	return a1.m_dataSize > a2.m_dataSize;
}


bool CDeleteClipData::SortByCreatedDateAsc(const CDeleteData& a1, const CDeleteData& a2)
{
	return a1.m_createdDateTime < a2.m_createdDateTime;
}

bool CDeleteClipData::SortByLastUsedDateAsc(const CDeleteData& a1, const CDeleteData& a2)
{
	return a1.m_lastUsedDateTime < a2.m_lastUsedDateTime;
}

bool CDeleteClipData::SortByFormatAsc(const CDeleteData& a1, const CDeleteData& a2)
{
	return a1.m_clipboardFormat < a2.m_clipboardFormat;
}

bool CDeleteClipData::SortByDataSizeAsc(const CDeleteData& a1, const CDeleteData& a2)
{
	return a1.m_dataSize < a2.m_dataSize;
}

// the quick paste column sorts descending both ways (as before)
const std::array<CDeleteClipData::ColumnSort, 7> CDeleteClipData::s_columnSorts{ {
	{ SortByIDDesc, SortByIDAsc },
	{ SortByTitleDesc, SortByTitleAsc },
	{ SortByQuickPaste, SortByQuickPaste },
	{ SortByCreatedDateDesc, SortByCreatedDateAsc },
	{ SortByLastUsedDateDesc, SortByLastUsedDateAsc },
	{ SortByFormatDesc, SortByFormatAsc },
	{ SortByDataSizeDesc, SortByDataSizeAsc },
} };

void CDeleteClipData::OnLvnColumnclickList2(NMHDR* pNMHDR, LRESULT* pResult)
{
	HD_NOTIFY* phdn = (HD_NOTIFY*)pNMHDR;

	if (phdn->iItem >= 0 && static_cast<size_t>(phdn->iItem) < s_columnSorts.size())
	{
		const ColumnSort& sort{ s_columnSorts[static_cast<size_t>(phdn->iItem)] };
		std::sort(m_data.begin(), m_data.end(), m_sortDescending ? sort.descending : sort.ascending);
	}

	m_sortDescending = !m_sortDescending;

	m_clipList.SetItemCountEx((int)m_data.size(), 0);

	*pResult = 0;
}


BOOL CDeleteClipData::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN)
		{
			FilterItems();
			return TRUE; // Do not process further
		}
		else if (pMsg->wParam == VK_ESCAPE)
		{
			if (IsDescriptionWindowValid())
			{
				m_pDescriptionWindow->Hide();
				return TRUE;
			}
		}
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CDeleteClipData::SelectRow(int selectedRow)
{
	RemoveAllSelection();
	SetCaret(selectedRow);
	SetSelection(selectedRow);
	ListView_SetSelectionMark(m_clipList.GetSafeHwnd(), selectedRow);
	m_clipList.EnsureVisible(selectedRow, FALSE);
}

void CDeleteClipData::RemoveAllSelection()
{
	POSITION pos = m_clipList.GetFirstSelectedItemPosition();
	while (pos)
	{
		SetSelection(m_clipList.GetNextSelectedItem(pos), FALSE);
	}
}

BOOL CDeleteClipData::SetCaret(int nRow, BOOL bFocus)
{
	if (bFocus)
		return m_clipList.SetItemState(nRow, LVIS_FOCUSED, LVIS_FOCUSED);
	else
		return m_clipList.SetItemState(nRow, ~static_cast<UINT>(LVIS_FOCUSED), LVIS_FOCUSED);
}

BOOL CDeleteClipData::SetSelection(int nRow, BOOL bSelect)
{
	if (bSelect)
		return m_clipList.SetItemState(nRow, LVIS_SELECTED, LVIS_SELECTED);
	else
		return m_clipList.SetItemState(nRow, ~static_cast<UINT>(LVIS_SELECTED), LVIS_SELECTED);
}

void CDeleteClipData::CreateAndShowDescriptionWindow()
{
	if (IsDescriptionWindowValid() == false)
	{
		// a self-deleting window: CWnd::CreateEx calls PostNcDestroy on failure too, so the
		// window owns the object from the Create call on
		CToolTipEx* pWindow{ std::make_unique<CToolTipEx>().release() }; // ownership: the window (PostNcDestroy deletes it, also when Create fails)
		if (pWindow->Create(this) == FALSE)
		{
			AfxThrowResourceException();
		}
		m_pDescriptionWindow = pWindow;
		m_descriptionWindowHwnd = m_pDescriptionWindow->GetSafeHwnd();
		m_pDescriptionWindow->SetNotifyWnd(GetParent());
	}

	POSITION pos = m_clipList.GetFirstSelectedItemPosition();
	if (pos != nullptr)
	{
		INT_PTR row = m_clipList.GetNextSelectedItem(pos);
		if (row >= 0 && row < (INT_PTR)m_data.size())
		{
			SetDescriptionWindowText(row);

			CRect rc;
			this->GetWindowRect(rc);

			CPoint pt;
			pt = CPoint(rc.right, rc.top);

			m_pDescriptionWindow->Show(pt);
		}
	}
}

void CDeleteClipData::SetDescriptionWindowText(INT_PTR row)
{
	m_pDescriptionWindow->SetGdiplusBitmap(nullptr);
	m_pDescriptionWindow->SetRTFText("");
	m_pDescriptionWindow->SetToolTipText(_T(""));
	m_pDescriptionWindow->SetFolderPath(_T(""));

	m_pDescriptionWindow->SetToolTipText(m_data[row].m_Desc);

	CClip selectedClip(theApp.Services().ClipContext());
	// a clip that does not load (a database error is shown by the load) is not described;
	// upstream described the empty clip as if it had loaded
	if (selectedClip.LoadMainTable(m_data[row].m_lID) == FALSE ||
		selectedClip.LoadFormats(m_data[row].m_lID, false, false, m_data[row].m_DatalID) == false)
	{
		return;
	}

	CString clipData = DescribeClip(selectedClip);

	int parentId = selectedClip.m_parentId;
	if (parentId > 0)
	{
		CString folder = CClipDatabase::FolderPath(theApp.Services().Database(), parentId);

		m_pDescriptionWindow->SetFolderPath(folder);
	}

	m_pDescriptionWindow->SetClipData(clipData);

	SetDescriptionWindowContent(selectedClip);
}

CString CDeleteClipData::DescribeClip(CClip& selectedClip)
{
	CString clipData;
	COleDateTime time(selectedClip.m_Time.GetTime());
	clipData += "Added: " + time.Format();

	COleDateTime modified(selectedClip.m_lastPasteDate.GetTime());
	clipData += _T(" | Last Used: ") + modified.Format();

	if (selectedClip.m_dontAutoDelete > 0)
	{
		clipData += _T(" | Never Auto Delete");
	}

	CString csQuickPaste = selectedClip.m_csQuickPaste;
	if (csQuickPaste.IsEmpty() == FALSE)
	{
		clipData += _T(" | Quick Paste = ");
		clipData += csQuickPaste;
	}

	int shortCut = selectedClip.m_shortCut;
	if (shortCut > 0)
	{
		clipData += _T(" | ");
		clipData += CHotKey::GetHotKeyDisplayStatic(shortCut);

		BOOL globalShortCut = selectedClip.m_globalShortCut;
		if (globalShortCut)
		{
			clipData += _T(" - Global Shortcut Key");
		}
	}

	if (theApp.Services().State().m_GroupID > 0)
	{
		double sticky = selectedClip.m_stickyClipGroupOrder;
		if (sticky != CClip::InvalidSticky)
		{
			clipData += _T(" | ");
			clipData += _T(" - Sticky In Group");
		}
	}
	else
	{
		double sticky = selectedClip.m_stickyClipOrder;
		if (sticky != CClip::InvalidSticky)
		{
			clipData += _T(" | ");
			clipData += _T(" - Sticky");
		}
	}

	return clipData;
}

void CDeleteClipData::SetDescriptionWindowContent(CClip& selectedClip)
{
	// the first format found is shown: text, else RTF, else HTML, else the image
	// (upstream declared a new format variable in the RTF and HTML steps, so every later step ran too)
	IClipFormat* format = SetDescriptionWindowPlainText(selectedClip);

	if (format == nullptr)
	{
		format = selectedClip.Clips()->FindFormatEx(CClipboardFormats::GetFormatID(CF_RTF));
		if (format != nullptr)
		{
			m_pDescriptionWindow->SetRTFText(format->GetAsCStringA());
		}
	}

	if (format == nullptr)
	{
		format = selectedClip.Clips()->FindFormatEx(CClipboardFormats::GetFormatID(_T("HTML Format")));
		if (format != nullptr)
		{
			// show the HTML source as plain text; this fork has no HTML renderer
			CString html = CTextConvert::Utf8ToUnicode(format->GetAsCStringA());
			m_pDescriptionWindow->SetToolTipText(html);
		}
	}

	if (format == nullptr)
	{
		SetDescriptionWindowImage(selectedClip);
	}
}

IClipFormat* CDeleteClipData::SetDescriptionWindowPlainText(CClip& selectedClip)
{
	IClipFormat* format = selectedClip.Clips()->FindFormatEx(CF_UNICODETEXT);
	if (format != nullptr)
	{
		m_pDescriptionWindow->SetToolTipText(format->GetAsCString());
	}

	if (format == NULL)
	{
		format = selectedClip.Clips()->FindFormatEx(CF_TEXT);
		if (format != nullptr)
		{
			CString cs(format->GetAsCStringA());
			m_pDescriptionWindow->SetToolTipText(cs);
		}
	}

	return format;
}

void CDeleteClipData::SetDescriptionWindowImage(CClip& selectedClip)
{
	try
	{
		// PNG is closer to the original, so it replaces the DIB when the clip has both
		for (const CLIPFORMAT cfType : { (CLIPFORMAT)CF_DIB, theApp.Services().ClipboardFormats().Png() })
		{
			CClipFormat* format = selectedClip.m_Formats.FindFormat(cfType);
			if (format != nullptr)
			{
				m_pDescriptionWindow->SetGdiplusBitmap(format->LoadGdiplusBitmap(theApp.Services().ClipboardFormats().Png()));
			}
		}
	}
	catch (const DittoCore::ClipboardFormatError& error)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Ditto cannot show the clip's image: the image data is malformed (%s)."), CString(error.what()).GetString()));
	}
}

void CDeleteClipData::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CMenu menu;
	menu.LoadMenu(IDR_MENU_DELETE_CLIP_DATA); // Load your context menu from resource

	CMenu* pContextMenu = menu.GetSubMenu(0); // Get the first submenu

	if (pContextMenu != nullptr)
	{
		int nID = pContextMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, point.x, point.y, this);

		switch (nID)
		{
		case ID__VIEWFULLDESCRIPTION:
		{
			CreateAndShowDescriptionWindow();
		}
		break;
		case ID__SAVETOFILE:
		{
			int row = m_clipList.GetNextItem(-1, LVNI_SELECTED);
			if (row >= 0 && row < (INT_PTR)m_data.size())
			{
				SaveClipDataItemToFile(m_data[row]);
			}
		}
		break;
		case ID__PROPERTIES:
		{
			ShowClipPropertiesWindow();
		}
		break;
		}
	}
}

void CDeleteClipData::ShowClipPropertiesWindow()
{
	int row = m_clipList.GetNextItem(-1, LVNI_SELECTED);
	if (row >= 0 && row < (INT_PTR)m_data.size())
	{
		CDimWnd dimmer(this);

		CCopyProperties props(m_data[row].m_lID, this);
		props.DoModal();
	}
}

void CDeleteClipData::SaveClipDataItemToFile(CDeleteData item)
{
	// The filter is a list of strings ending in an empty one, so it stays a literal: a CString
	// would end it at the first \0. The default extension has no period.
	const SaveFileType* saveFileType{ FindSaveFileType(item.m_clipboardFormat) };
	if (saveFileType == nullptr)
	{
		return;
	}
	const TCHAR* extension{ saveFileType->extension };
	const TCHAR* filter{ saveFileType->filter };

	OPENFILENAME ofn{};
	TCHAR szFile[400]{};
	const CString csInitialDir = theApp.Services().Settings().GetLastImportDir();

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrFilter = filter;
	ofn.nFilterIndex = 1;
	ofn.lpstrInitialDir = csInitialDir.GetString();
	ofn.lpstrDefExt = extension;
	// a save dialog: the file may be new (no OFN_FILEMUSTEXIST)
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

	if (GetSaveFileName(&ofn))
	{
		CClip selectedClip(theApp.Services().ClipContext());
		// nothing is written when the format does not load (a database error is shown by the load)
		if (selectedClip.LoadFormats(item.m_lID, false, false, item.m_DatalID))
		{
			WriteClipDataItem(selectedClip, item, ofn);
		}
	}
}

const std::array<CDeleteClipData::SaveFileType, 5> CDeleteClipData::s_saveFileTypes{ {
	{ _T("PNG"), _T("png"), _T("PNG Files (*.png)\0*.png\0") },
	{ _T("CF_DIB"), _T("bmp"), _T("Bitmap Files (*.bmp)\0*.bmp\0") },
	{ _T("CF_UNICODETEXT"), _T("txt"), _T("Text Files (*.txt)\0*.txt\0") },
	{ _T("CF_TEXT"), _T("txt"), _T("Text Files (*.txt)\0*.txt\0") },
	{ _T("Rich Text Format"), _T("rtf"), _T("Rich Text Files (*.rtf)\0*.rtf\0") },
} };

const CDeleteClipData::SaveFileType* CDeleteClipData::FindSaveFileType(const CString& format)
{
	for (const SaveFileType& type : s_saveFileTypes)
	{
		if (format == type.format)
		{
			return &type;
		}
	}
	return nullptr;
}

void CDeleteClipData::WriteClipDataItem(CClip& selectedClip, const CDeleteData& item, const OPENFILENAME& ofn)
{
	const CString path{ CFileDialogPath::From(ofn) };
	BOOL written{ TRUE };
	if (item.m_clipboardFormat == _T("PNG") || item.m_clipboardFormat == _T("CF_DIB"))
	{
		// reports its own failure, naming the file
		selectedClip.WriteImageToFileOrReport(path, _T("save"));
	}
	else if (item.m_clipboardFormat == _T("CF_UNICODETEXT"))
	{
		written = selectedClip.WriteTextToFile(path, TRUE, FALSE, FALSE);
	}
	else if (item.m_clipboardFormat == _T("CF_TEXT"))
	{
		written = selectedClip.WriteTextToFile(path, FALSE, TRUE, FALSE);
	}
	else if (item.m_clipboardFormat == _T("Rich Text Format"))
	{
		written = selectedClip.WriteTextToFile(path, FALSE, FALSE, TRUE);
	}

	if (written == FALSE)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Saving the clip's %s data to %s failed."), item.m_clipboardFormat.GetString(), path.GetString()));
	}
}
void CDeleteClipData::OnCancel()
{
	//don't close on escape key
}

void CDeleteClipData::OnBnClickedBtCompactAndRepair()
{
	auto msg = theApp.Services().Language().GetString("CompactRepairWarning", "Warning this can take quite a long time and require up to double the hard drive space as your current database size, Continue?");
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
					int toDeleteCount = theApp.Services().Database().execScalar(_T("SELECT COUNT(clipID) FROM MainDeletes"));
					if (toDeleteCount <= 0)
						break;

					// a failed purge (shown) stops before VACUUM; upstream retried it up to 100 times
					if (CClipRetentionPolicy::RemoveOldEntries(theApp.Services().Settings(), theApp.Services().IdleTime(), theApp.Services().Windows(), false) == FALSE)
					{
						return;
					}
				}
			}
			catch (CppSQLite3Exception& e)
			{
				CErrorReport::Show(CStringUtil::Format(_T("Compact and repair failed while removing deleted clips, the database was not compacted: %s"), e.errorMessage()));
				return;
			}

			theApp.Services().Database().execDML(_T("PRAGMA auto_vacuum = 1"));
			theApp.Services().Database().execQuery(_T("VACUUM"));
			SetDbSize();
		}
		catch (CppSQLite3Exception& e)
		{
			CErrorReport::Show(CStringUtil::Format(_T("Compacting and repairing the clip database failed: %s"), e.errorMessage()));
			return;
		}
	}
}
