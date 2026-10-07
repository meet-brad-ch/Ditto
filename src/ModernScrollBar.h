#pragma once

#include <afxwin.h>
#include "DPI.h"

// Scrollbar orientation
enum class ScrollBarOrientation
{
	Vertical,
	Horizontal
};

// Modern scrollbar overlay control with rounded corners
// Similar to Discord, Teams, GitHub Desktop style
class CModernScrollBar : public CWnd
{
	DECLARE_DYNAMIC(CModernScrollBar)

public:
	CModernScrollBar();
	virtual ~CModernScrollBar();

	// Create the scrollbar overlay
	BOOL Create(CWnd* pParentWnd, CListCtrl* pListCtrl, ScrollBarOrientation orientation = ScrollBarOrientation::Vertical);
	
	// Update scrollbar position and visibility based on list state
	void UpdateScrollBar();
	
	// Set scrollbar colors from theme
	void SetColors(COLORREF trackColor, COLORREF thumbColor, COLORREF thumbHoverColor);
	
	// Set rounded corner radius
	void SetCornerRadius(int radius) { m_cornerRadius = radius; }
	
	// Set scrollbar width/height (depending on orientation)
	void SetWidth(int width) { m_scrollBarWidth = width; }
	
	// Show/hide with fade animation
	void Show(bool animate = true);
	void Hide(bool animate = true);
	
	// Check if mouse is over scrollbar
	bool IsMouseOver() const { return m_isMouseOver; }
	
	// DPI awareness
	void SetDPI(CDPI* pDPI) { m_pDPI = pDPI; }
	
	// Get orientation
	ScrollBarOrientation GetOrientation() const { return m_orientation; }

protected:
	DECLARE_MESSAGE_MAP()
	
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnMouseHover(WPARAM wParam, LPARAM lParam);

	// Calculate thumb rectangle based on list scroll position
	CRect GetThumbRect();
	
	// Draw rounded rectangle with GDI+
	void DrawRoundedRect(CDC* pDC, CRect rect, int radius, COLORREF color);
	
	// Scroll list to position based on thumb drag
	void ScrollToPosition(int thumbPos);

private:
	/** @brief Native scroll bar type that matches the orientation.
	 *  @return SB_VERT for a vertical bar, SB_HORZ for a horizontal bar. */
	int GetScrollBarType() const;

	/** @brief Tells if the list control exists and has a window.
	 *  @return true if m_pListCtrl and its window handle are set. */
	bool HasListWindow() const;

	/** @brief Scales a pixel value for the DPI, if a DPI object is set.
	 *  @param value Value at 96 DPI.
	 *  @return The scaled value, or value when no DPI object is set. */
	int ScaleForDpi(int value) const;

	/** @brief Tells if the scroll info has a range and a page.
	 *  @param si Scroll info of the list control.
	 *  @return true if nMax > 0 and nPage > 0. */
	static bool HasScrollRange(const SCROLLINFO& si);

	/** @brief Track length along the orientation.
	 *  @param clientRect Client rectangle of the scroll bar.
	 *  @return Height for a vertical bar, width for a horizontal bar. */
	int GetTrackSize(const CRect& clientRect) const;

	/** @brief Thumb length, proportional to the page, not below the minimum thumb size.
	 *  @param si Scroll info of the list control.
	 *  @param totalRange nMax - nMin + 1.
	 *  @param trackSize Track length in pixels.
	 *  @return Thumb length in pixels. */
	int CalcThumbSize(const SCROLLINFO& si, int totalRange, int trackSize) const;

	/** @brief Thumb start position on the track for the current scroll position.
	 *  @param si Scroll info of the list control.
	 *  @param totalRange nMax - nMin + 1.
	 *  @param trackSize Track length in pixels.
	 *  @param thumbSize Thumb length in pixels.
	 *  @return Thumb position, clamped to the track. */
	static int CalcThumbPos(const SCROLLINFO& si, int totalRange, int trackSize, int thumbSize);

	/** @brief Builds the thumb rectangle for the orientation.
	 *  @param clientRect Client rectangle of the scroll bar.
	 *  @param thumbPos Thumb start position.
	 *  @param thumbSize Thumb length.
	 *  @return The thumb rectangle. */
	CRect MakeThumbRect(const CRect& clientRect, int thumbPos, int thumbSize) const;

	/** @brief Scroll bar rectangle in parent client coordinates.
	 *  @param listRectInParent List control rectangle in parent client coordinates.
	 *  @param parentClientRect Parent client rectangle.
	 *  @return The rectangle to move the scroll bar to. */
	CRect CalcScrollRect(const CRect& listRectInParent, const CRect& parentClientRect) const;

	/** @brief Converts a dragged thumb position into a list scroll position.
	 *  @param thumbPos Requested thumb position (clamped to the track).
	 *  @param trackSize Track length in pixels.
	 *  @param thumbSize Thumb length in pixels.
	 *  @param scrollableRange totalRange - nPage.
	 *  @return The new scroll position. */
	static int CalcScrollPosFromThumb(int thumbPos, int trackSize, int thumbSize, int scrollableRange);

	/** @brief Scrolls the list vertically to a row position.
	 *  @param si Scroll info of the list control (changed by the fallback path).
	 *  @param newPos New top row position. */
	void ScrollVerticalTo(SCROLLINFO& si, int newPos);

	/** @brief Scrolls the list horizontally to a pixel position.
	 *  @param newPos New horizontal scroll position. */
	void ScrollHorizontalTo(int newPos);


	CListCtrl* m_pListCtrl;
	CWnd* m_pParentWnd;
	CDPI* m_pDPI;
	ScrollBarOrientation m_orientation;
	
	// Colors
	COLORREF m_trackColor;
	COLORREF m_thumbColor;
	COLORREF m_thumbHoverColor;
	
	// Dimensions
	int m_scrollBarWidth;
	int m_scrollBarHoverWidth;
	int m_cornerRadius;
	int m_minThumbSize;  // Min thumb width or height depending on orientation
	
	// State
	bool m_isMouseOver;
	bool m_isDragging;
	int m_dragStartPos;  // X or Y depending on orientation
	int m_dragStartScrollPos;
	bool m_isVisible;
	bool m_trackingMouse;
	
	// Timer
	enum { TIMER_AUTO_HIDE = 1 };
};
