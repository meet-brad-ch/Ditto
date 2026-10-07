//
// GdipButton.h : Version 1.0 - see article at CodeProject.com
//
// Author:  Darren Sessions
//          
//
// Description:
//     GdipButton is a CButton derived control that uses GDI+ 
//     to support alternate image formats
//
// History
//     Version 1.0 - 2008 June 10
//     - Initial public release
//
// License:
//     This software is released under the Code Project Open License (CPOL),
//     which may be found here:  http://www.codeproject.com/info/eula.aspx
//     You are free to use this software in any way you like, except that you 
//     may not sell this source code.
//
//     This software is provided "as is" with no expressed or implied warranty.
//     I accept no liability for any damage or loss of business that this 
//     software may cause.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

// GdipButton.h : header file
//

#include <array>
#include <memory>
#include <span>

#include "CGdiPlusBitmap.h"
/////////////////////////////////////////////////////////////////////////////
// CGdipButton window

class CGdipButton : public CButton
{
public:

	CGdipButton();
	virtual ~CGdipButton();

	// image types
	enum	{
				STD_TYPE	= 0,
				ALT_TYPE,
				DIS_TYPE
			};

	// sets the image type
	void SetImage(int type);

	BOOL LoadAltImage(UINT id, LPCTSTR pType);
	BOOL LoadStdImage(UINT id, LPCTSTR pType);

	BOOL LoadStdImageDPI(int dpi, UINT id96, UINT id120, UINT id144, UINT id168, UINT id192, LPCTSTR pType, UINT id225 = 0, UINT id250 = 0, UINT id275 = 0, UINT id300 = 0, UINT id325 = 0, UINT id350 = 0);

	// if false, disables the press state and uses grayscale image if it exists
	void EnableButton(BOOL bEnable = TRUE) { m_bIsDisabled = !bEnable; }

	// in toggle mode each press toggles between std and alt images
	void EnableToggle(BOOL bEnable = TRUE);

	// return the enable/disable state
	BOOL IsDisabled(void) {return (m_bIsDisabled == TRUE); }

	void SetBkGnd(CDC* pDC);

	void SetToolTipText(CString spText, BOOL bActivate = TRUE);
	void SetToolTipText(UINT nId, BOOL bActivate = TRUE);
	void SetHorizontal(bool ImagesAreLaidOutHorizontally = FALSE);
	void DeleteToolTip();

	void Reset();


protected:

	void PaintBk(CDC* pDC);
	void PaintBtn(CDC* pDC);

	BOOL	m_bHaveAltImage;
	BOOL	m_bHaveBitmaps;

	BOOL	m_bIsDisabled;
	BOOL	m_bIsToggle;
	BOOL	m_bIsHovering;
	BOOL	m_bIsTracking;

	int		m_nCurType;

	/** @brief The alternate image (owned); empty until LoadAltImage. */
	std::unique_ptr<CGdiPlusBitmapResource> m_pAltImage{};
	/** @brief The standard image (owned); empty until LoadStdImage. */
	std::unique_ptr<CGdiPlusBitmapResource> m_pStdImage{};

	CString			m_tooltext;
	/** @brief The button's tooltip control (owned); empty until InitToolTip. */
	std::unique_ptr<CToolTipCtrl>	m_pToolTip{};
	
	void	InitToolTip();

	virtual void PreSubclassWindow();
	virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	//{{AFX_MSG(CGdipButton)
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg LRESULT OnMouseLeave(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnMouseHover(WPARAM wparam, LPARAM lparam) ;
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:

	/** @brief One candidate image of LoadStdImageDPI: used when the DPI is at least minDpi. */
	struct DpiImageChoice
	{
		/** @brief Lowest DPI that uses this image. */
		int minDpi{};
		/** @brief Resource id of the image. */
		UINT id{};
		/** @brief true if the image is optional: an id of 0 skips this entry. */
		bool optional{};
	};

	/** @brief Picks the resource id of the first matching DPI choice.
	 *  @param dpi Current DPI.
	 *  @param choices Candidates, largest DPI first; the last one must always match.
	 *  @return The chosen resource id (0 if no entry matches). */
	static UINT PickDpiImageId(int dpi, std::span<const DpiImageChoice> choices);

	/** @brief Copies the parent's background behind the button into m_dcBk.
	 *  @param rect Client rectangle of the button. */
	void CreateBackgroundDC(const CRect& rect);

	/** @brief Creates target as a DC with a bitmap copy of what pDC shows.
	 *  @param target DC to create.
	 *  @param pDC Memory DC to copy from.
	 *  @param rect Client rectangle of the button. */
	static void CaptureToDC(CDC& target, CDC* pDC, const CRect& rect);

	/** @brief Creates the standard, pressed, hot and grayscale DCs of the standard image.
	 *  @param pDC Memory DC to draw in.
	 *  @param graphics GDI+ graphics on pDC.
	 *  @param rect Client rectangle of the button. */
	void CreateStdImageDCs(CDC* pDC, Gdiplus::Graphics& graphics, const CRect& rect);

	/** @brief Creates the alternate, pressed and hot DCs of the alternate image.
	 *  @param pDC Memory DC to draw in.
	 *  @param graphics GDI+ graphics on pDC.
	 *  @param rect Client rectangle of the button. */
	void CreateAltImageDCs(CDC* pDC, Gdiplus::Graphics& graphics, const CRect& rect);

	/** @brief Points m_pCurBtn at the bitmap for the pressed, hot or normal state.
	 *  @param bIsPressed TRUE if the button is pressed. */
	void SelectCurBtn(BOOL bIsPressed);

	CDC		m_dcBk;			// button background
	
	CDC		m_dcStd;		// standard button
	CDC		m_dcStdP;		// standard button pressed
	CDC		m_dcStdH;		// standard button hot

	CDC		m_dcAlt;		// alternate button
	CDC		m_dcAltP;		// alternate button pressed
	CDC		m_dcAltH;		// alternate button hot

	CDC		m_dcGS;			// grayscale button (does not have a hot or pressed state)

	CDC*	m_pCurBtn;		// current pointer to one of the above

};
