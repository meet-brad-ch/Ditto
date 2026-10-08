#pragma once

#include <memory>
#include <vector>

#include "DittoRulerRichEditCtrl.h"
#include "TabCtrl.h"
#include "ClipIds.h"
#include "DPI.h"

class CEditWnd : public CWnd
{
	DECLARE_DYNAMIC(CEditWnd)

public:
	CEditWnd();
	virtual ~CEditWnd();

	bool EditIds(CClipIDs& Ids);
	bool CloseEdits(bool bPrompt);

	void OnDpiChanged(CWnd* pParent, int dpi);

protected:
	DECLARE_MESSAGE_MAP()

	CTabCtrlEx m_tabControl;
	CToolBar m_toolBarControl;
	CToolTipCtrl m_toolTipControl;
	/** @brief The open edit controls (owned), one per tab in tab order. */
	std::vector<std::unique_ptr<CDittoRulerRichEditCtrl>> m_edits{};
	CButton m_updateDescriptionButton;
	CFont m_font;
	long m_lastSaveID;

	CDPI m_dpi;

protected:
	void MoveControls();
	int IsIDAlreadyInEdit(int id, bool bSetFocus);
	bool AddItem(int id);
	bool DoSave();
	bool DoSaveItem(int index);
	void LoadToolbarDPI();

public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSave();
	afx_msg void OnDestroy();
	afx_msg void OnSaveAll();
	afx_msg void OnClose();
	afx_msg void OnNew();
	afx_msg void OnSaveCloseClipboard();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

private:
	/** @brief Handles escape (close, Shift: save and close to the clipboard), Ctrl + S (save,
	 *  Shift: save all) and Ctrl + N (new).
	 *  @param key Virtual key code of the WM_KEYDOWN message.
	 *  @return true if the key was handled and must not be passed on (Ctrl + N is passed on). */
	bool HandleKeyDown(WPARAM key);
};
