#pragma once

#include "DialogResizer.h"
#include "ShowTaskBarIcon.h"
#include "afxwin.h"
#include "ATLComTime.h"
#include <array>
#include <vector>
#include "afxcmn.h"

class CProgressWnd;

// CDeleteClipData dialog

class CClip;
class IClipFormat;

class CDeleteData
{
public:
	CDeleteData() :
		m_lID(-1),
		m_dataSize(0)
	{
	}

	long m_lID;
	long m_DatalID{ -1 };
	CString m_Desc;
	CTime m_createdDateTime;
	CTime m_lastUsedDateTime;
	CString m_clipboardFormat;
	DWORD m_dataSize;
	CString m_quickPasteText;
};


class CDeleteClipData : public CDialog
{
	DECLARE_DYNAMIC(CDeleteClipData)

public:
	CDeleteClipData(CWnd* pParent = NULL); // standard constructor
	virtual ~CDeleteClipData();

	void SetNotifyWnd(HWND hWnd);

	// Dialog Data
	enum
	{
		IDD = IDD_DELETE_CLIP_DATA
	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support

	DECLARE_MESSAGE_MAP()


	CDialogResizer m_Resize;
	CListCtrl m_clipList;
	HWND m_hWndParent{};
	CShowTaskBarIcon m_showTaskbar;
	std::vector<CDeleteData> m_data;
	std::vector<CDeleteData> m_toDelete;
	std::vector<CDeleteData> m_filteredOut;
	bool m_applyingDelete;
	bool m_cancelDelete;

	/** @brief The description window (non-owning: the window deletes itself in PostNcDestroy). */
	CToolTipEx* m_pDescriptionWindow;
	/** @brief The description window's handle, read instead of the object until the window is known to be alive. */
	HWND m_descriptionWindowHwnd{};

	/**
	 * @brief Whether the description window exists: m_pDescriptionWindow is set and its window is alive
	 *        (the user can close it, which deletes the object).
	 * @return true when m_descriptionWindowHwnd is a window and that window is m_pDescriptionWindow.
	 */
	bool IsDescriptionWindowValid() const;


	void InitListCtrlCols();
	virtual BOOL OnInitDialog();

	void SetDbSize();

	afx_msg void OnClose();
	void CloseDescriptionWindow();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNcDestroy();
	void LoadItems();
	void FilterItems();
	bool MatchesFilter(CDeleteData* pdata);
	void ApplyDelete();
	void RemoveAllSelection();
	BOOL SetCaret(int nRow, BOOL bFocus = 1);
	BOOL SetSelection(int nRow, BOOL bSelect = 1);
	void SelectRow(int selectedRow);
	void CreateAndShowDescriptionWindow();
	void SetDescriptionWindowText(INT_PTR row);
	// Shows the clip's image in the description window; reports a malformed image.
	void SetDescriptionWindowImage(CClip& selectedClip);
	void SaveClipDataItemToFile(CDeleteData item);
	// Copies text into the list view's display buffer, cut to the buffer size.
	void CopyDisplayText(LVITEM& item, LPCTSTR text);

public:
	CString m_clipTitle;
	BOOL m_filterByClipTitle;
	BOOL m_filterByCreatedDate;
	BOOL m_filterByLastUsedDate;
	BOOL m_filterByClipboardFormat;
	CComboBox m_clipboardFomatCombo;
	COleDateTime m_createdDateStart;
	COleDateTime m_createdDateEnd;
	COleDateTime m_createdTimeStart;
	COleDateTime m_createdTimeEnd;
	COleDateTime m_usedTimeStart;
	COleDateTime m_usedTimeEnd;
	COleDateTime m_usedDateStart;
	COleDateTime m_usedDateEnd;
	afx_msg void OnBnClickedButtonSearch();
	afx_msg void OnLvnKeydownList2(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnItemchangedList2(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHdnGetdispinfoList2(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnGetdispinfoList2(NMHDR* pNMHDR, LRESULT* pResult);
	CString m_databaseSize;
	CString m_selectedSize;
	CString m_selectedCount;
	CString m_toDeleteSize;
	afx_msg void OnBnClickedCheckClipTitle();
	afx_msg void OnBnClickedButtonApply();
	afx_msg void OnBnClickedClose();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedCheckCreateDate();
	afx_msg void OnBnClickedCheckLastUseDate();
	afx_msg void OnBnClickedCheckDataFormat();
	afx_msg void OnLvnColumnclickList2(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	void ShowClipPropertiesWindow();
	virtual void OnCancel();
	afx_msg void OnBnClickedBtCompactAndRepair();

private:
	// ApplyDelete's delete loop: deletes the rows' data items from the last row back, stepping the
	// progress; stops on cancel, or after reporting the first item that fails to delete
	void DeleteRows(const std::vector<int>& rowsToDelete, CProgressWnd& progress);

	/**
	 * @brief FilterItems' first step: finds the filtered-out items that match the filter again.
	 * @param addBackIn Receives copies of the items that match again, in order.
	 * @param filteredRowsToDelete Receives their indexes in m_filteredOut, ascending.
	 */
	void FindFilteredOutMatches(std::vector<CDeleteData>& addBackIn, std::vector<int>& filteredRowsToDelete);

	/**
	 * @brief MatchesFilter's title step: whether the title filter rejects an item.
	 * @param pdata The item (unchanged: the search is case-insensitive on copies).
	 * @return True when the title filter is on, both texts are set and the title is not found.
	 */
	bool IsRejectedByTitle(const CDeleteData* pdata) const;

	/**
	 * @brief Whether a time lies within a range given as separate date and time controls (ends included).
	 * @param value The time to check.
	 * @param startDate The date of the range start.
	 * @param startTime The time of day of the range start.
	 * @param endDate The date of the range end.
	 * @param endTime The time of day of the range end.
	 * @return True when start <= value <= end.
	 */
	static bool IsInDateRange(const CTime& value, const COleDateTime& startDate, const COleDateTime& startTime, const COleDateTime& endDate, const COleDateTime& endTime);

	/**
	 * @brief MatchesFilter's format step: whether an item has the format selected in the combo box.
	 * @param pdata The item.
	 * @return True when the item's clipboard format equals the selected one; false when no format is selected.
	 */
	bool MatchesSelectedFormat(const CDeleteData* pdata);

	/**
	 * @brief OnLvnKeydownList2's 'N' key: selects the next row and shows its description.
	 */
	void SelectNextRowAndDescribe();

	/**
	 * @brief OnLvnKeydownList2's 'P' key: selects the previous row and shows its description.
	 */
	void SelectPreviousRowAndDescribe();

	/**
	 * @brief OnLvnGetdispinfoList2's text step: copies the text of the item's column into its display buffer.
	 * @param item The list view item; iItem must be a valid row of m_data. Unknown columns get no text.
	 */
	void CopyColumnText(LVITEM& item);

	/** @brief The two orders a list column sorts in. */
	struct ColumnSort
	{
		/** @brief The comparison for the descending order. */
		bool (*descending)(const CDeleteData&, const CDeleteData&) = nullptr;
		/** @brief The comparison for the ascending order. */
		bool (*ascending)(const CDeleteData&, const CDeleteData&) = nullptr;
	};

	/** @brief The sort orders of the list columns, by column index. */
	static const std::array<ColumnSort, 7> s_columnSorts;

	/** @brief The direction of the next column sort: true for descending; every column click toggles it. */
	bool m_sortDescending{ true };

	/**
	 * @brief Orders by clip id, highest first.
	 * @param a1 The first item.
	 * @param a2 The second item.
	 * @return True when a1 goes before a2.
	 */
	static bool SortByIDDesc(const CDeleteData& a1, const CDeleteData& a2);
	/**
	 * @brief Orders by clip id, lowest first.
	 * @param a1 The first item.
	 * @param a2 The second item.
	 * @return True when a1 goes before a2.
	 */
	static bool SortByIDAsc(const CDeleteData& a1, const CDeleteData& a2);
	/**
	 * @brief Orders by title, descending.
	 * @param a1 The first item.
	 * @param a2 The second item.
	 * @return True when a1 goes before a2.
	 */
	static bool SortByTitleDesc(const CDeleteData& a1, const CDeleteData& a2);
	/**
	 * @brief Orders by title, ascending.
	 * @param a1 The first item.
	 * @param a2 The second item.
	 * @return True when a1 goes before a2.
	 */
	static bool SortByTitleAsc(const CDeleteData& a1, const CDeleteData& a2);
	/**
	 * @brief Orders by quick paste text, descending (used for both directions).
	 * @param a1 The first item.
	 * @param a2 The second item.
	 * @return True when a1 goes before a2.
	 */
	static bool SortByQuickPaste(const CDeleteData& a1, const CDeleteData& a2);
	/**
	 * @brief Orders by created date, newest first.
	 * @param a1 The first item.
	 * @param a2 The second item.
	 * @return True when a1 goes before a2.
	 */
	static bool SortByCreatedDateDesc(const CDeleteData& a1, const CDeleteData& a2);
	/**
	 * @brief Orders by last used date, newest first.
	 * @param a1 The first item.
	 * @param a2 The second item.
	 * @return True when a1 goes before a2.
	 */
	static bool SortByLastUsedDateDesc(const CDeleteData& a1, const CDeleteData& a2);
	/**
	 * @brief Orders by clipboard format name, descending.
	 * @param a1 The first item.
	 * @param a2 The second item.
	 * @return True when a1 goes before a2.
	 */
	static bool SortByFormatDesc(const CDeleteData& a1, const CDeleteData& a2);
	/**
	 * @brief Orders by data size, largest first.
	 * @param a1 The first item.
	 * @param a2 The second item.
	 * @return True when a1 goes before a2.
	 */
	static bool SortByDataSizeDesc(const CDeleteData& a1, const CDeleteData& a2);
	/**
	 * @brief Orders by created date, oldest first.
	 * @param a1 The first item.
	 * @param a2 The second item.
	 * @return True when a1 goes before a2.
	 */
	static bool SortByCreatedDateAsc(const CDeleteData& a1, const CDeleteData& a2);
	/**
	 * @brief Orders by last used date, oldest first.
	 * @param a1 The first item.
	 * @param a2 The second item.
	 * @return True when a1 goes before a2.
	 */
	static bool SortByLastUsedDateAsc(const CDeleteData& a1, const CDeleteData& a2);
	/**
	 * @brief Orders by clipboard format name, ascending.
	 * @param a1 The first item.
	 * @param a2 The second item.
	 * @return True when a1 goes before a2.
	 */
	static bool SortByFormatAsc(const CDeleteData& a1, const CDeleteData& a2);
	/**
	 * @brief Orders by data size, smallest first.
	 * @param a1 The first item.
	 * @param a2 The second item.
	 * @return True when a1 goes before a2.
	 */
	static bool SortByDataSizeAsc(const CDeleteData& a1, const CDeleteData& a2);

	/**
	 * @brief SetDescriptionWindowText's details line: dates, auto delete, quick paste, shortcut, sticky.
	 * @param selectedClip The loaded clip.
	 * @return The text for the description window's clip data.
	 */
	static CString DescribeClip(CClip& selectedClip);

	/**
	 * @brief SetDescriptionWindowText's content step: shows the clip's text, else RTF, else HTML, else image.
	 * @param selectedClip The loaded clip.
	 */
	void SetDescriptionWindowContent(CClip& selectedClip);

	/**
	 * @brief Shows the clip's unicode text, or else its CF_TEXT, in the description window.
	 * @param selectedClip The loaded clip.
	 * @return The text format shown; null when the clip has neither.
	 */
	IClipFormat* SetDescriptionWindowPlainText(CClip& selectedClip);

	/** @brief A clipboard format that can be saved to a file, with the save dialog's settings. */
	struct SaveFileType
	{
		/** @brief The stored clipboard format name. */
		const TCHAR* format{};
		/** @brief The default extension, without a period. */
		const TCHAR* extension{};
		/** @brief The save dialog's filter: a list of strings ending in an empty one. */
		const TCHAR* filter{};
	};

	/** @brief The clipboard formats SaveClipDataItemToFile can save. */
	static const std::array<SaveFileType, 5> s_saveFileTypes;

	/**
	 * @brief The save settings of a clipboard format.
	 * @param format The stored clipboard format name.
	 * @return The entry of s_saveFileTypes; null when the format cannot be saved.
	 */
	static const SaveFileType* FindSaveFileType(const CString& format);

	/**
	 * @brief SaveClipDataItemToFile's write step: writes the item's format to the chosen file.
	 * @param selectedClip The clip, loaded with the item's data.
	 * @param item The data item.
	 * @param ofn The completed save dialog.
	 */
	static void WriteClipDataItem(CClip& selectedClip, const CDeleteData& item, const OPENFILENAME& ofn);
};
