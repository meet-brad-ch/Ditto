#pragma once

#include <vector>

class CMagneticWnd : public CWnd
{
public:
	CMagneticWnd(void);
	~CMagneticWnd(void);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
	afx_msg void OnMove(int x, int y);

	std::vector<CMagneticWnd*> m_SnapToWnds;
	std::vector<CMagneticWnd*> m_AttachedWnd;
	bool m_bMovedAttachedWnd;
	CRect m_crLastMove;
	bool m_bHandleWindowPosChanging;

private:
	/** @brief Tells if this window can snap to another window in this move.
	 *  @param pOtherWnd Window to snap to (may be NULL).
	 *  @param lpwndpos New position of this window.
	 *  @return true if both windows are visible and the position and size are all non-zero. */
	bool CanSnapTo(CMagneticWnd *pOtherWnd, const WINDOWPOS* lpwndpos);

	/** @brief Snaps the new position to another window and updates the attached state.
	 *  @param pOtherWnd Window to snap to.
	 *  @param lpwndpos New position of this window; changed by the snap. */
	void SnapToWindow(CMagneticWnd *pOtherWnd, WINDOWPOS* lpwndpos);

	/** @brief Moves the new position onto the edges of a rectangle that are within 15 pixels.
	 *  @param lpwndpos New position of this window; changed by the snap.
	 *  @param rectParent Window rectangle to snap to.
	 *  @return true if any edge snapped. */
	static bool SnapEdges(WINDOWPOS* lpwndpos, const CRect& rectParent);

public:
	void AddWindowToSnapTo(CMagneticWnd *pWnd)	{ m_SnapToWnds.push_back(pWnd); }
	void SetWindowAttached(CMagneticWnd *pOther, bool bAttache);
	bool IsWindowAttached(CMagneticWnd *pWnd);
	void MoveMagneticWWnd(LPCRECT lpRect, BOOL bRepaint = TRUE);

	void SetMoveAttachedWnds(bool bMove)	{ m_bMovedAttachedWnd = bMove; }
};
