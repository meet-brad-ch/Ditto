#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include <array>


// CQuickPasteKeyboard dialog

class CQuickPasteKeyboard : public CPropertyPage
{
	DECLARE_DYNAMIC(CQuickPasteKeyboard)

public:
	CQuickPasteKeyboard();
	virtual ~CQuickPasteKeyboard();

	class KeyboardAB
	{
	public:
		KeyboardAB()
		{
			A = -1;
			B = -1;
			Dirty = false;
		}
		int A;
		int B;
		bool Dirty;
	};

	class KeyboardArray
	{
	public:
		KeyboardAB Array[10];
	};

// Dialog Data
	enum { IDD = IDD_OPTIONS_QUICK_PASTE_KEYBOARD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_list;
	virtual BOOL OnInitDialog();

protected:
	void InitListCtrlCols();
	void LoadItems();
	CString GetShortCutText(KeyboardArray ab);
	CString GetShortCutText(KeyboardAB ab);
	std::map<DWORD, KeyboardArray> m_map;
	void LoadHotKey(KeyboardAB ab);
	CString m_csTitle;
	int SelectedCommandId();
	int SelectedCommandShortCutId();
	int SelectedCommandRow();
	void SelectMouseTypeCombo(CComboBox &combo, int value);
	void SelectedRow(int row);

	/** @brief The keys that get the extended key flag when a shortcut is loaded into a hot key control. */
	static const std::array<BYTE, 12> s_extendedKeys;

	/** @brief A key with the extended key flag added when it is one of s_extendedKeys.
	@param key the key (virtual key in the low byte, modifiers in the high byte).
	@return the key, with HOTKEYF_EXT for an extended key. */
	static int WithExtendedKeyFlag(int key);
	/** @brief Is a key one of the mouse "keys" (click, double click, right click, middle click)?
	@param key the key (virtual key in the low byte).
	@return true for a mouse key. */
	static bool IsMouseKey(int key);
	/** @brief The list sort callback (CListCtrl::SortItems): orders the rows by their command text, ignoring case.
	@param lParam1 the item data of the first row.
	@param lParam2 the item data of the second row.
	@param lParamSort the list control (CListCtrl*).
	@return negative, zero or positive as for CString::CompareNoCase. */
	static int CALLBACK MyCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	/** @brief Shows the first press of a shortcut in the mouse or keyboard controls.
	@param a the first press key. */
	void LoadFirstPress(int a);
	/** @brief Shows the second press of a shortcut in the mouse or keyboard controls.
	@param b the second press key. */
	void LoadSecondPress(int b);
	/** @brief Checks the shift, control and alt check boxes of a modifier byte.
	@param mod the HOTKEYF_* modifiers.
	@param shiftId the shift check box.
	@param controlId the control check box.
	@param altId the alt check box. */
	void CheckModifierButtons(BYTE mod, int shiftId, int controlId, int altId);
	/** @brief Hides the controls of the second press. */
	void HideSecondPressControls();
	/** @brief Reads the first press from the keyboard or mouse controls into a shortcut.
	@param ab the shortcut; A is set when the keyboard or mouse radio is checked. */
	void ReadFirstPress(KeyboardAB &ab);
	/** @brief Reads the second press from the keyboard or mouse controls into a shortcut (0 when disabled).
	@param ab the shortcut; B is set. */
	void ReadSecondPress(KeyboardAB &ab);
	/** @brief The HOTKEYF_* modifiers of the checked shift, control and alt check boxes.
	@param shiftId the shift check box.
	@param controlId the control check box.
	@param altId the alt check box.
	@return the modifiers. */
	WORD CheckedModifiers(int shiftId, int controlId, int altId);

public:
	afx_msg void OnLvnItemActivateList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult);
	CHotKeyCtrl m_hotKey1;
	afx_msg void OnBnClickedAssign();
	CHotKeyCtrl m_hotKey2;
	virtual BOOL OnApply();
	CComboBox m_assignedCombo;
	afx_msg void OnCbnSelchangeComboAllAssigned();
	afx_msg void OnBnClickedButtonRemove();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnEnKillfocusEdit1();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedButtonEnter();
	afx_msg void OnBnClickedButtonEnter2();

	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	CComboBox m_mouseType1;
	CComboBox m_mouseType2;
	afx_msg void OnBnClickedButtonReset();
	afx_msg void OnBnClickedRadioKeyboard1();
	afx_msg void OnBnClickedRadioMouse1();
	afx_msg void OnBnClickedRadioKeyboard2();
	afx_msg void OnBnClickedRadioMouse2();
	afx_msg void OnBnClickedCheckEnableSecondPress();
};
