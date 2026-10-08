// QuickPaste.h: interface for the CQuickPaste class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_QUICKPASTE_H__1B4A98E6_B719_402C_BDD4_7F3F97CD0EB0__INCLUDED_)
#define AFX_QUICKPASTE_H__1B4A98E6_B719_402C_BDD4_7F3F97CD0EB0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "QPasteWnd.h"
#include <memory>

class CAppState;
class CDittoDb;
class CGetSetOptions;
class ExternalWindowTracker;

class CQuickPaste
{
public:
	/** @brief Creates the quick paste controller.
	@param settings the application settings; must outlive this object.
	@param state the application state (whether the quick paste window shows); must outlive this object.
	@param database the clip database (closed and reopened on request); must outlive this object.
	@param activeWindow the tracker of the target window (the caret the window opens at); must outlive this object. */
	CQuickPaste(CGetSetOptions& settings, CAppState& state, CDittoDb& database, ExternalWindowTracker& activeWindow);
	virtual ~CQuickPaste();

	void ShowQPasteWnd(CWnd* pParent, bool bAtPrevPos, bool bFromKeyboard, BOOL bReFillList);
	void HideQPasteWnd();
	BOOL CloseQPasteWnd();
	BOOL IsWindowVisibleEx();
	void MoveSelection(bool down);
	void OnKeyStateUp();
	void SetKeyModiferState(bool bActive);
	bool IsWindowTopLevel();

	void UpdateFont()
	{
		if (m_pwndPaste) m_pwndPaste->UpdateFont();
	}

	void OnScreenResolutionChange();

	//protected:
	std::unique_ptr<CQPasteWnd> m_pwndPaste{}; // the quick paste window, owned; null until first shown

protected:
	/** @brief True: the next show moves and sizes the window again (set when the screen resolution changed). */
	bool m_forceResizeOnNextShow{};

private:
	/** @brief The application settings (owned by the application services). */
	CGetSetOptions& m_settings;
	/** @brief The application state (owned by the application services). */
	CAppState& m_state;
	/** @brief The clip database (owned by the application services). */
	CDittoDb& m_database;
	/** @brief The tracker of the target window (owned by the application services). */
	ExternalWindowTracker& m_activeWindow;

	/** @brief Is the "close the window and reopen the database" key combination (shift + control, not from the keyboard hot key) down?
	@param bFromKeyboard the window is shown from the keyboard hot key.
	@return true for the combination. */
	static bool IsReopenDatabaseRequested(bool bFromKeyboard);
	/** @brief Closes the quick paste window and reopens the database. */
	void CloseWndAndReopenDatabase();
	/** @brief Brings the always-on-top window to the front when it exists.
	@return true when it was shown. */
	bool ShowPersistentWnd();
	/** @brief The window size (and saved point) to start from: the current window, or the saved options.
	@param point the saved point, set when the options are used.
	@param csSize the size to set. */
	void GetInitialPointAndSize(CPoint& point, CSize& csSize);
	/** @brief The caret position, or the center of the active window or of the cursor's monitor when there is no caret.
	@param csSize the window size.
	@param point set to the cursor position when the monitor center is used.
	@return the point. */
	CPoint CaretOrCenterPoint(const CSize& csSize, CPoint& point);
	/** @brief Sets the window point (and for the previous position the size) by the position option.
	@param nPosition the POS_* option.
	@param bAtPrevPos show at the previous position.
	@param ptCaret the caret point.
	@param point the point to set.
	@param csSize the size, set for the previous position. */
	void ChooseWindowPoint(int nPosition, bool bAtPrevPos, const CPoint& ptCaret, CPoint& point, CSize& csSize);
	/** @brief Keeps the window rect on the screen and replaces an invalid size by 300x300 at the caret.
	@param crRect the window rect.
	@param ptCaret the caret point.
	@return true when the window must be moved. */
	bool FixInitialRect(CRect& crRect, const CPoint& ptCaret);
	/** @brief Creates the window when it does not exist yet.
	@param pParent the parent window (none when it shows in the task bar).
	@param crRect the window rect.
	@return true when the window was created (its rect must be scaled by the DPI). */
	bool CreateWndIfNeeded(CWnd* pParent, const CRect& crRect);
	/** @brief Must the window be moved to the new rect?
	@param nPosition the POS_* option.
	@param bAtPrevPos show at the previous position.
	@param forceMoveWindow the rect was changed.
	@return true to move it. */
	static bool ShouldMoveWindow(int nPosition, bool bAtPrevPos, bool forceMoveWindow);
	/** @brief Moves the window to the rect, scaled by the DPI after a create.
	@param crRect the window rect, scaled in place.
	@param adjustRect the window was just created. */
	void MoveQPasteWnd(CRect& crRect, bool adjustRect);
};

#endif // !defined(AFX_QUICKPASTE_H__1B4A98E6_B719_402C_BDD4_7F3F97CD0EB0__INCLUDED_)
