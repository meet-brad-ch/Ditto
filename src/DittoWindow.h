#pragma once

#include <array>

#include "GdipButton.h"
#include "GdiImageDrawer.h"
#include "DPI.h"

class CDittoWindow
{
public:
	/** @brief The caption buttons: DoNcLButtonDown and DoNcLButtonUp return the pressed one (0: none). */
	enum : int
	{
		/** @brief The close button. */
		ButtonClose = 1,
		/** @brief The chevron (roll up / down) button. */
		ButtonChevron = 2,
		/** @brief The minimize button. */
		ButtonMinimize = 3,
		/** @brief The maximize button. */
		ButtonMaximize = 4,
	};

	/** @brief MinMaxWindow's options. */
	enum : int
	{
		/** @brief Rolls the window up when it is down, else down. */
		SwapMinMax = 1,
		/** @brief Rolls the window up (does nothing when it is up). */
		ForceMin = 2,
		/** @brief Rolls the window down (does nothing when it is down). */
		ForceMax = 3,
	};

	CDittoWindow(void);
	~CDittoWindow(void);

	void DoNcPaint(CWnd* pWnd);
	void DrawChevronBtn(CWindowDC& dc, CWnd* pWnd);
	void DrawCloseBtn(CWindowDC& dc, CWnd* pWnd);
	void DrawMaximizeBtn(CWindowDC& dc, CWnd* pWnd);
	void DrawMinimizeBtn(CWindowDC& dc, CWnd* pWnd);
	void DrawWindowIcon(CWindowDC& dc, CWnd* pWnd);

	void DoCreate(CWnd* pWnd);
	void DoNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp);
	UINT DoNcHitTest(CWnd* pWnd, CPoint point);
	long DoNcLButtonUp(CWnd* pWnd, UINT nHitTest, CPoint point);
	int DoNcLButtonDown(CWnd* pWnd, UINT nHitTest, CPoint point);
	bool DoPreTranslateMessage(MSG* pMsg);
	void SetCaptionOn(CWnd* pWnd, int nPos, bool bOnstartup, int captionSize, int captionFontSize);
	bool SetCaptionColors(COLORREF left, COLORREF right, COLORREF border);
	void SetCaptionTextColor(COLORREF color);
	void MinMaxWindow(CWnd* pWnd, long lOption);
	void SetTitleTextHeight(CWnd* pWnd);
	int IndexToPos(int index, bool horizontal);
	void OnDpiChanged(CWnd* pWnd, int dpi);

	bool m_bDrawClose;
	bool m_sendWMClose;
	bool m_bDrawChevron;
	bool m_bDrawMaximize;
	bool m_bDrawMinimize;

	CRect m_crCloseBT;
	CRect m_crChevronBT;
	CRect m_crMaximizeBT;
	CRect m_crMinimizeBT;
	CRect m_crWindowIconBT;

	CFont m_VertFont;
	CFont m_HorFont;

	bool m_bMinimized;

	bool m_bMouseDownOnChevron;
	bool m_bMouseOverChevron;
	bool m_bMouseDownOnClose;
	bool m_bMouseOverClose;
	bool m_bMouseDownOnMinimize;
	bool m_bMouseOverMinimize;
	bool m_bMouseDownOnMaximize;
	bool m_bMouseOverMaximize;

	COLORREF m_CaptionColorLeft;
	COLORREF m_CaptionColorRight;
	COLORREF m_CaptionTextColor;
	COLORREF m_border;

	CGdiImageDrawer m_closeButton;
	CGdiImageDrawer m_chevronRightButton;
	CGdiImageDrawer m_chevronLeftButton;
	CGdiImageDrawer m_maximizeButton;
	CGdiImageDrawer m_minimizeButton;
	//CGdiImageDrawer m_windowIcon;

	CString m_customWindowTitle;
	bool m_useCustomWindowTitle;

	int m_captionBorderWidth;
	int m_captionFontSize{};

	int m_captionPosition;
	int m_borderSize;

	int m_titleTextHeight{};

	bool m_buttonDownOnCaption;

	CRect m_crFullSizeWindow;
	COleDateTime m_TimeMinimized;
	COleDateTime m_TimeMaximized;


	CDPI m_dpi;

private:
	/** @brief The caption button offsets from the caption's start, by button slot (unscaled). */
	struct ButtonOffset
	{
		/** @brief The offset in a horizontal (top or bottom) caption. */
		int horizontal{};
		/** @brief The offset in a vertical (left or right) caption. */
		int vertical{};
	};

	/** @brief The slot of each caption button (in drawing order) and the number of slots used. */
	struct ButtonSlots
	{
		/** @brief The number of buttons drawn. */
		int count{};
		/** @brief The slot of the close button. */
		int close{};
		/** @brief The slot of the chevron button. */
		int chevron{};
		/** @brief The slot of the maximize button. */
		int maximize{};
		/** @brief The slot of the minimize button. */
		int minimize{};
	};

	/** @brief The caption's areas: the two coloured parts and the title text. */
	struct CaptionLayout
	{
		/** @brief The part of the caption filled with the left colour. */
		CRect leftRect{};
		/** @brief The part of the caption filled with the right colour (behind the buttons). */
		CRect rightRect{};
		/** @brief The title text area. */
		CRect textRect{};
		/** @brief Whether the caption is vertical (left or right). */
		BOOL vertical{ FALSE };
	};

	/** @brief A window position and size, as MoveWindow takes them. */
	struct WindowPlacement
	{
		/** @brief The left edge. */
		LONG x{};
		/** @brief The top edge. */
		LONG y{};
		/** @brief The width. */
		LONG width{};
		/** @brief The height. */
		LONG height{};
	};

	/** @brief The button offsets by slot (IndexToPos). */
	static const std::array<ButtonOffset, 5> s_buttonOffsets;

	/**
	 * @brief Whether a point is on one of the caption buttons.
	 * @param myLocal the point relative to the window.
	 * @return true when it is on the close, chevron, minimize or maximize button.
	 */
	bool IsOnCaptionButton(const CPoint& myLocal) const;
	/**
	 * @brief Hit tests the top corners of the window frame.
	 * @param crWindow the window rectangle.
	 * @param point the point in screen coordinates.
	 * @return HTTOPLEFT, HTTOPRIGHT, or UINT_MAX when the point is not on a top corner.
	 */
	UINT HitTestTopCorners(const CRect& crWindow, const CPoint& point) const;
	/**
	 * @brief Hit tests the bottom corners of the window frame.
	 * @param crWindow the window rectangle.
	 * @param point the point in screen coordinates.
	 * @return HTBOTTOMRIGHT, HTBOTTOMLEFT, or UINT_MAX when the point is not on a bottom corner.
	 */
	UINT HitTestBottomCorners(const CRect& crWindow, const CPoint& point) const;
	/**
	 * @brief Hit tests the top and bottom edges (not for a minimized window with a top or bottom caption).
	 * @param crWindow the window rectangle.
	 * @param point the point in screen coordinates.
	 * @return HTTOP, HTBOTTOM, or UINT_MAX when the point is not on these edges.
	 */
	UINT HitTestTopBottomEdges(const CRect& crWindow, const CPoint& point) const;
	/**
	 * @brief Hit tests the left and right edges (not for a minimized window with a left or right caption).
	 * @param crWindow the window rectangle.
	 * @param point the point in screen coordinates.
	 * @return HTRIGHT, HTLEFT, or UINT_MAX when the point is not on these edges.
	 */
	UINT HitTestLeftRightEdges(const CRect& crWindow, const CPoint& point) const;
	/**
	 * @brief Whether a point is in the caption band.
	 * @param crWindow the window rectangle.
	 * @param point the point in screen coordinates.
	 * @return true when the point is within the caption width from the caption's side.
	 */
	bool IsInCaption(const CRect& crWindow, const CPoint& point) const;

	/**
	 * @brief Draws the window border and shrinks the rectangle to its inside.
	 * @param dc the window DC.
	 * @param rcBorder the window rectangle; shrinks by the border width.
	 * @param border the border width.
	 */
	void DrawFrameBorder(CWindowDC& dc, CRect& rcBorder, int border);
	/**
	 * @brief Gives each drawn caption button a slot.
	 * @return the slots.
	 */
	ButtonSlots AssignButtonSlots() const;
	/**
	 * @brief Lays out a right caption and its buttons.
	 * @param rcBorder the inside of the window border.
	 * @param border the border width.
	 * @param widthHeight the button size.
	 * @param slots the button slots.
	 * @return the caption areas.
	 */
	CaptionLayout LayoutRightCaption(const CRect& rcBorder, int border, int widthHeight, const ButtonSlots& slots);
	/**
	 * @brief Lays out a left caption and its buttons.
	 * @param rcBorder the inside of the window border.
	 * @param border the border width.
	 * @param widthHeight the button size.
	 * @param slots the button slots.
	 * @return the caption areas.
	 */
	CaptionLayout LayoutLeftCaption(const CRect& rcBorder, int border, int widthHeight, const ButtonSlots& slots);
	/**
	 * @brief Lays out a top caption and its buttons.
	 * @param rcBorder the inside of the window border.
	 * @param widthHeight the button size.
	 * @param slots the button slots.
	 * @return the caption areas.
	 */
	CaptionLayout LayoutTopCaption(const CRect& rcBorder, int widthHeight, const ButtonSlots& slots);
	/**
	 * @brief Lays out a bottom caption and its buttons.
	 * @param rcBorder the inside of the window border.
	 * @param border the border width.
	 * @param widthHeight the button size.
	 * @param slots the button slots.
	 * @return the caption areas.
	 */
	CaptionLayout LayoutBottomCaption(const CRect& rcBorder, int border, int widthHeight, const ButtonSlots& slots);
	/**
	 * @brief Places the buttons of a vertical caption, one below the other.
	 * @param left the buttons' left edge.
	 * @param right the buttons' right edge.
	 * @param widthHeight the button size.
	 * @param slots the button slots.
	 */
	void SetVerticalButtonRects(int left, int right, int widthHeight, const ButtonSlots& slots);
	/**
	 * @brief Places the buttons of a horizontal caption, from the right edge leftwards.
	 * @param rightEdge the right edge of the caption.
	 * @param top the buttons' top edge.
	 * @param bottom the buttons' bottom edge.
	 * @param widthHeight the button size.
	 * @param slots the button slots.
	 */
	void SetHorizontalButtonRects(int rightEdge, int top, int bottom, int widthHeight, const ButtonSlots& slots);
	/**
	 * @brief Fills the two caption parts with the caption colours.
	 * @param dc the window DC.
	 * @param layout the caption areas.
	 */
	void FillCaption(CWindowDC& dc, const CaptionLayout& layout);
	/**
	 * @brief Draws the window title in the caption.
	 * @param dc the window DC.
	 * @param pWnd the window.
	 * @param layout the caption areas; the text area is adjusted to the title.
	 */
	void DrawCaptionText(CWindowDC& dc, CWnd* pWnd, CaptionLayout& layout);

	/**
	 * @brief Releases the close button: closes the window when the mouse is still on it.
	 * @param pWnd the window.
	 * @param localPoint the point relative to the window.
	 * @return ButtonClose when the button was clicked, else 0.
	 */
	long ReleaseCloseButton(CWnd* pWnd, const CPoint& localPoint);
	/**
	 * @brief Releases the chevron button.
	 * @param pWnd the window.
	 * @param localPoint the point relative to the window.
	 * @return ButtonChevron when the button was clicked, else 0.
	 */
	long ReleaseChevronButton(CWnd* pWnd, const CPoint& localPoint);
	/**
	 * @brief Releases the minimize button: minimizes the window when the mouse is still on it.
	 * @param pWnd the window.
	 * @param localPoint the point relative to the window.
	 * @return ButtonMinimize when the button was clicked, else 0.
	 */
	long ReleaseMinimizeButton(CWnd* pWnd, const CPoint& localPoint);
	/**
	 * @brief Releases the maximize button: maximizes or restores the window when the mouse is still on it.
	 * @param pWnd the window.
	 * @param localPoint the point relative to the window.
	 * @return ButtonMaximize when the button was clicked, else 0.
	 */
	long ReleaseMaximizeButton(CWnd* pWnd, const CPoint& localPoint);

	/**
	 * @brief Whether the caption position is one of the four sides.
	 * @return true for the four CGetSetOptions::CaptionOn* positions.
	 */
	bool IsKnownCaptionPosition() const;
	/**
	 * @brief Shrinks the window to its caption.
	 * @param pWnd the window.
	 */
	void MinimizeToCaption(CWnd* pWnd);
	/**
	 * @brief Restores the window shrunk to its caption.
	 * @param pWnd the window.
	 */
	void RestoreFromCaption(CWnd* pWnd);
	/**
	 * @brief Where the window goes when it shrinks to its caption.
	 * @return the placement within m_crFullSizeWindow, at the caption's side.
	 */
	WindowPlacement MinimizedPlacement() const;
	/**
	 * @brief Where the window goes when it is restored from its caption.
	 * @param cr the current (minimized) window rectangle.
	 * @return the placement with the full size, kept at the caption's side.
	 */
	WindowPlacement RestoredPlacement(const CRect& cr) const;
};
