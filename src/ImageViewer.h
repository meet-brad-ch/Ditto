#pragma once

#include <memory>

#include "ScrollHelper.h"
#include "Clip.h"



class CImageViewer : public CWnd
{
	DECLARE_DYNAMIC(CImageViewer)

public:
	CImageViewer();
	virtual ~CImageViewer();

	/** @brief The image shown (owned); empty when there is none. */
	std::unique_ptr<Gdiplus::Bitmap> m_pGdiplusBitmap{};
	CScrollHelper m_scrollHelper;

	void UpdateBitmapSize(bool setScale);

	BOOL Create(CWnd* pParent);

	bool m_hoveringOverImage;


	CPoint m_ptFirst;
	CPoint m_ptSecond;
	DWORD m_dwArguments{};

	double m_scale;

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
public:
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg LRESULT OnGesture(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGestureNotify(WPARAM wParam, LPARAM lParam);

private:
	/** @brief Interprets one gesture.
	 *  @param gi Gesture information from GetGestureInfo.
	 *  @return TRUE for a known gesture (zoom, pan, rotate, two-finger tap, press and tap). */
	BOOL HandleGesture(const GESTUREINFO& gi);

	/** @brief Zoom gesture: tracks the zoom center and factor (debug output only).
	 *  @param gi Gesture information from GetGestureInfo. */
	void HandleZoomGesture(const GESTUREINFO& gi);

	/** @brief Pan gesture: scrolls the image by the finger movement.
	 *  @param gi Gesture information from GetGestureInfo. */
	void HandlePanGesture(const GESTUREINFO& gi);
};


