/* ==========================================================================
	File :			RuleRichEditCtrl.cpp

	Class :			CRulerRichEditCtrl

	Author :		Johan Rosengren, Abstrakt Mekanik AB
					Iain Clarke

	Date :			2004-04-17

	Purpose :		"CRulerRichEditCtrl" is a "CWnd" derived class containing an 
					embedded RTF-control, a ruler-control with dragable tab-
					positions and a formatting toolbar. The class can be used 
					to - for example - add a complete mini-editor to a modal 
					or modeless dialog box. 

	Description :	The class mainly handles mouse messages. The mouse
					messages are sent from the ruler control, and are 
					button down, where the a check is made for the cursor 
					located on one of the tab-markers, mouse move, where an 
					XORed line is drawn across the RTF-control and button up, 
					where a new tab position is set. The class also handles 
					the toolbar buttons, setting styles as 
					appropriate for the selected text.

	Usage :			Add a "CRulerRichEditCtrl"-member to the parent class. 
					Call Create to create the control. "GetRichEditCtrl" can 
					be used to access the embedded RTF-control. Remember to 
					call "AfxInitRichEdit(2)"!

					The contents can be saved to disk by calling "Save", and 
					loaded from disk by calling "Load". The two functions 
					will automatically display a file dialog if the file 
					name parameter of the calls are left empty.

					"GetRTF" and "SetRTF" can be used to get and set the 
					contents of the embedded RTF-control as RTF 
					respectively.

					The ruler measures can be displayed as inches or 
					centimeters, by calling "SetMode". "GetMode" will get the 
					current mode.

   ========================================================================*/

#include "stdafx.h"
#include "StdGrfx.h"
#include "RulerRichEditCtrl.h"
#include "RichEditStringSink.h"
#include "RichEditUtf8Source.h"
#include "..\Options.h"
#include "..\CP_Main.h"
#include "..\Misc.h"
#include ".\rulerricheditctrl.h"
#include "..\..\resource.h"

#include <vector>



/////////////////////////////////////////////////////////////////////////////
// Registered messages for ruler/CRulerRichEditCtrl communication

const UINT& CRulerRichEditCtrl::SetCurrentFontNameMessage()
{
	static const UINT message{ ::RegisterWindowMessage( _T( "_RULERRICHEDITCTRL_SET_CURRENT_FONT_NAME" ) ) };
	return message;
}

const UINT& CRulerRichEditCtrl::SetCurrentFontSizeMessage()
{
	static const UINT message{ ::RegisterWindowMessage( _T( "_RULERRICHEDITCTRL_SET_CURRENT_FONT_SIZE" ) ) };
	return message;
}

const UINT& CRulerRichEditCtrl::SetCurrentFontColorMessage()
{
	static const UINT message{ ::RegisterWindowMessage( _T( "_RULERRICHEDITCTRL_SET_CURRENT_FONT_COLOR" ) ) };
	return message;
}


/////////////////////////////////////////////////////////////////////////////
// CRulerRichEditCtrl

CRulerRichEditCtrl::CRulerRichEditCtrl()
/* ============================================================
	Function :		CRulerRichEditCtrl::CRulerRichEditCtrl
	Description :	constructor
	Access :		Public
					
	Return :		void
	Parameters :	none

	Usage :			

   ============================================================*/
{
	m_margin = 0;
	m_movingtab = -1;
	m_offset = 0;
	m_readOnly = FALSE;
	m_bInWrapMode = theApp.Services().Settings().GetEditWordWrap();
	ShowToolbar();
}

CRulerRichEditCtrl::~CRulerRichEditCtrl()
/* ============================================================
	Function :		CRulerRichEditCtrl::~CRulerRichEditCtrl
	Description :	destructor
	Access :		Public
					
	Return :		void
	Parameters :	none

	Usage :			

   ============================================================*/
{	
}


BOOL CRulerRichEditCtrl::Create( DWORD dwStyle, const RECT &rect, CWnd* pParentWnd, UINT nID, BOOL autohscroll )
/* ============================================================
	Function :		CRulerRichEditCtrl::Create
	Description :	Creates the control and sub controls.
	Access :		Public
					
	Return :		BOOL				-	"TRUE" if created OK.
	Parameters :	DWORD dwStyle		-	Style of the control, 
											normally "WS_CHILD" 
											and "WS_VISIBLE".
					const RECT &rect	-	Placement rectangle.
					CWnd* pParentWnd	-	Parent window.
					UINT nID			-	Control ID
					BOOL autohscroll	-	"TRUE" if the RTF-control
											should have the 
											"ES_AUTOHSCROLL" style
											set.

	Usage :			Call to create the control.

   ============================================================*/
{	
	BOOL result = CWnd::Create(NULL, _T( "" ), dwStyle, rect, pParentWnd, nID);
	if ( result )
	{
		result = FALSE;
		// Save screen resolution for
		// later on.
		CClientDC dc( this );
		m_physicalInch = dc.GetDeviceCaps( LOGPIXELSX );

		// Create sub-controls
		if( CreateRTFControl( autohscroll ) )
		{
			CreateMargins();
			if( CreateToolbar() )
			{
				UpdateToolbarButtons();
				result = TRUE;				
			}

			//Do wrap will reverse the saved option so initially set it as opposite of what it's saved as
			m_bInWrapMode = !m_bInWrapMode;
			DoWrap();
		}

		m_dpi.SetHwnd(m_hWndOwner);
	}

	return result;
}

void CRulerRichEditCtrl::OnDpiChanged(CWnd* /*pParent*/, int dpi)
{
	m_dpi.Update(dpi);
	
	m_toolbar.DestroyWindow();
	CreateToolbar();

	UpdateToolbarButtons();

	m_toolbar.Invalidate();
	m_toolbar.RedrawWindow();

	CRect rect;
	GetClientRect(rect);
	LayoutControls(rect.Width(), rect.Height());
}

BOOL CRulerRichEditCtrl::CreateToolbar()
/* ============================================================
	Function :		CRulerRichEditCtrl::CreateToolbar
	Description :	Creates the toolbar control
	Access :		Private

	Return :		BOOL	-	"TRUE" if the toolbar was created ok.
	Parameters :	none

	Usage :			Called during control creation

   ============================================================*/
{

	CRect rect;
	GetClientRect( rect );

	CRect toolbarRect( 0, 0, rect.right, m_dpi.Scale(s_toolbarHeight));
	return m_toolbar.Create( this, toolbarRect, ToolbarIdPerDPI());
}

BOOL CRulerRichEditCtrl::CreateRTFControl( BOOL autohscroll )
/* ============================================================
	Function :		CRulerRichEditCtrl::CreateRTFControl
	Description :	Creates the embedded RTF-control.
	Access :		Private
					
	Return :		BOOL				-	"TRUE" if created ok.
	Parameters :	BOOL autohscroll	-	"TRUE" if the RTF-control
											should have the
											"ES_AUTOHSCROLL" style
											set.

	Usage :			Called during control creation

   ============================================================*/
{
	BOOL result = FALSE;

	CRect rect;
	GetClientRect( rect );

	int top = s_toolbarHeight;
	CRect rtfRect( 0, top, rect.right, rect.bottom );
	DWORD style = ES_NOHIDESEL|WS_CHILD|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL|ES_WANTRETURN|ES_MULTILINE;
	if( autohscroll )
		style |= ES_AUTOHSCROLL;

	if( m_rtf.Create( style, rtfRect, this ) )
	{
		// Setting up default tab stops
 		ParaFormat para( PFM_TABSTOPS );
 		para.cTabCount = MAX_TAB_STOPS;
 		for( int t = 0; t < MAX_TAB_STOPS ; t++ )
 			para.rgxTabs[ t ] = 640 * ( t + 1 );
 
 		m_rtf.SetParaFormat( para );
 
 		// Setting default character format
 		CharFormat	cf;
 		cf.dwMask = CFM_SIZE | CFM_FACE | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_LINK;
 		cf.yHeight = theApp.Services().Settings().GetEditorDefaultFontSize() * 20;
 		cf.dwEffects = 0;
 		lstrcpy( cf.szFaceName, _T( "Segoe UI" ) );
 		m_rtf.SendMessage(EM_SETCHARFORMAT, 0, (LPARAM)&cf);

		DWORD editStyle = s_sesHyperlinkTooltips | s_sesNoFocusLinkNotify;
		m_rtf.SendMessage(EM_SETEDITSTYLE, editStyle, editStyle);

		m_rtf.SendMessage(EM_AUTOURLDETECT, TRUE, 0);
 
 		// Set the internal tabs array
 		SetTabStops( ( LPLONG ) ( para.rgxTabs ), MAX_TAB_STOPS );
 
 		m_rtf.SetEventMask( m_rtf.GetEventMask() | ENM_SELCHANGE | ENM_SCROLL | ENM_CHANGE );
 		SetReadOnly( GetReadOnly() ); 

		result = TRUE;
	}

	return result;
}

void CRulerRichEditCtrl::CreateMargins()
/* ============================================================
	Function :		CRulerRichEditCtrl::CreateMargins
	Description :	Sets the margins for the subcontrols and 
					the RTF-control edit rect.
	Access :		Private

	Return :		void
	Parameters :	none

	Usage :			Called during control creation.

   ============================================================*/
{
	// Set up edit rect margins
	int scmargin = 4;
	CRect rc;
	m_rtf.GetClientRect( rc );

	rc.top = scmargin;
	rc.left = scmargin * 2;
	rc.right -= scmargin * 2;

	m_rtf.SetRect( rc );

	// Get the diff between the window- and client 
	// rect of the RTF-control. This gives the actual 
	// size of the RTF-control border.
	CRect	r1;
	CRect	r2;

	m_rtf.GetWindowRect( r1 );
	m_rtf.GetClientRect( r2 );
	m_rtf.ClientToScreen( r2 );

	// Create the margin for the toolbar 
	// controls and the ruler.
	m_margin = scmargin * 2 + r2.left - r1.left;
	//m_ruler.SetMargin( m_margin );
}

BEGIN_MESSAGE_MAP(CRulerRichEditCtrl, CWnd)
	//{{AFX_MSG_MAP(CRulerRichEditCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_MESSAGE( WM_SETTEXT, OnSetText )
	ON_MESSAGE( WM_GETTEXT, OnGetText  )
	ON_MESSAGE( WM_GETTEXTLENGTH, OnGetTextLength )
	ON_BN_CLICKED(BUTTON_FONT, OnButtonFont)
	ON_BN_CLICKED(BUTTON_COLOR, OnButtonColor)
	ON_BN_CLICKED(BUTTON_BOLD, OnButtonBold)
	ON_BN_CLICKED(BUTTON_ITALIC, OnButtonItalic)
	ON_BN_CLICKED(BUTTON_UNDERLINE, OnButtonUnderline)
	ON_BN_CLICKED(BUTTON_LEFTALIGN, OnButtonLeftAlign)
	ON_BN_CLICKED(BUTTON_CENTERALIGN, OnButtonCenterAlign)
	ON_BN_CLICKED(BUTTON_RIGHTALIGN, OnButtonRightAlign)
	ON_BN_CLICKED(BUTTON_INDENT, OnButtonIndent)
	ON_BN_CLICKED(BUTTON_OUTDENT, OnButtonOutdent)
	ON_BN_CLICKED(BUTTON_BULLET, OnButtonBullet)
	ON_BN_CLICKED(ID_BUTTONWRAP, OnButtonWrap)
	ON_WM_SETFOCUS()
	ON_REGISTERED_MESSAGE(SetCurrentFontNameMessage(), OnSetCurrentFontName)
	ON_REGISTERED_MESSAGE(SetCurrentFontSizeMessage(), OnSetCurrentFontSize)
	ON_REGISTERED_MESSAGE(SetCurrentFontColorMessage(), OnSetCurrentFontColor)
	//}}AFX_MSG_MAP
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CRulerRichEditCtrl message handlers

void CRulerRichEditCtrl::OnPaint() 
/* ============================================================
	Function :		CRulerRichEditCtrl::OnPaint
	Description :	Paints the ruler.
	Access :		Protected
					
	Return :		void
	Parameters :	none

	Usage :			Called from MFC. 

   ============================================================*/
{

	CPaintDC mainDC(this);
	UpdateTabStops();

}

BOOL CRulerRichEditCtrl::OnEraseBkgnd( CDC* /*pDC*/ ) 
/* ============================================================
	Function :		CRulerRichEditCtrl::OnEraseBkgnd
	Description :	Returns "TRUE" to avoid flicker.
	Access :		Protected
					
	Return :		BOOL		-	Always "TRUE",
	Parameters :	CDC* pDC	-	Not used
					
	Usage :			Called from MFC. 

   ============================================================*/
{
	
	return TRUE;

}

BOOL CRulerRichEditCtrl::OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult ) 
/* ============================================================
	Function :		CRulerRichEditCtrl::OnNotify
	Description :	Called as the RTF-control is updated or 
					the selection changes.
	Access :		Protected
					
	Return :		BOOL				-	From base class
	Parameters :	WPARAM wParam		-	Control ID
					LPARAM lParam		-	Not interested
					LRESULT* pResult	-	Not interested
					
	Usage :			Called from MFC. We must check the control 
					every time the selection changes or the 
					contents are changed, as the cursor might 
					have entered a new paragraph, with new tab 
					and/or font settings.

   ============================================================*/
{

	if( wParam == CRulerRichEdit::s_controlId )
	{

		// Update the toolbar
		UpdateToolbarButtons();

		// Update ruler
		CRect rect;
		GetClientRect( rect );
		rect.top = s_toolbarHeight;
		rect.bottom = rect.top;

		RedrawWindow( rect );

	}
	
	return CWnd::OnNotify( wParam, lParam, pResult );

}

void CRulerRichEditCtrl::OnSize( UINT nType, int cx, int cy ) 
/* ============================================================
	Function :		CRulerRichEditCtrl::OnSize
	Description :	We resize the embedded RTF-control.
	Access :		Protected
					
	Return :		void
	Parameters :	UINT nType	-	Not interested
					int cx		-	New width
					int cy		-	New height
					
	Usage :			Called from MFC. 

   ============================================================*/
{

	CWnd::OnSize( nType, cx, cy );
	
	if( m_rtf.m_hWnd )
	{

		LayoutControls( cx, cy );

	}
	
}

void CRulerRichEditCtrl::OnSetFocus( CWnd* pOldWnd ) 
/* ============================================================
	Function :		CRulerRichEditCtrl::OnSetFocus
	Description :	We handle over the focus to the embedded 
					RTF-control.
	Access :		Protected
					
	Return :		void
	Parameters :	CWnd* pOldWnd	-	Not used
					
	Usage :			Called from MFC. 

   ============================================================*/
{

	CWnd::OnSetFocus( pOldWnd );
	m_rtf.SetFocus();
	
}

LRESULT CRulerRichEditCtrl::OnSetText( WPARAM wParam, LPARAM lParam )
/* ============================================================
	Function :		CRulerRichEditCtrl::OnSetText
	Description :	The function handles the "WM_SETTEXT" 
					message. The handler sets the text in the 
					RTF-control
	Access :		Protected
										
	Return :		LRESULT			-	From the control
	Parameters :	WPARAM wParam	-	Passed on
					LPARAM lParam	-	Passed on
					
	Usage :			Called from MFC.

   ============================================================*/
{
	
	return m_rtf.SendMessage( WM_SETTEXT, wParam, lParam );

}

LRESULT CRulerRichEditCtrl::OnGetText( WPARAM wParam, LPARAM lParam )
/* ============================================================
	Function :		CRulerRichEditCtrl::OnGetText
	Description :	The function handles the "WM_GETTEXT" 
					message. The handler gets the text from the 
					RTF-control
	Access :		Protected
										
	Return :		LRESULT			-	From the control
	Parameters :	WPARAM wParam	-	Passed on
					LPARAM lParam	-	Passed on
					
	Usage :			Called from MFC.

   ============================================================*/
{

	return m_rtf.SendMessage( WM_GETTEXT, wParam, lParam );

}

LRESULT CRulerRichEditCtrl::OnGetTextLength( WPARAM /*wParam*/, LPARAM /*lParam*/ )
/* ============================================================
	Function :		CRulerRichEditCtrl::OnGetTextLength
	Description :	The function handles the "WM_GETTEXTLENGTH" 
					message. The handler gets the length of 
					the text in the RTF-control
	Access :		Protected
										
	Return :		LRESULT			-	From the control
	Parameters :	WPARAM wParam	-	Passed on
					LPARAM lParam	-	Passed on
					
	Usage :			Called from MFC.

   ============================================================*/
{

	return m_rtf.GetTextLength();

}

/////////////////////////////////////////////////////////////////////////////
// CRulerRichEditCtrl public implementation

/**
 * @brief Returns the contents of the control as RTF.
 * @return The RTF contents, streamed out through CRichEditStringSink.
 */
CString CRulerRichEditCtrl::GetRTF()
{
	CRichEditStringSink sink;
	EDITSTREAM stream{ sink.Stream() };
	m_rtf.StreamOut( SF_RTF, stream );
	return sink.Text();
}

/**
 * @brief Replaces the contents of the control with RTF.
 * @param rtf The RTF contents, streamed in as UTF-8 through CRichEditUtf8Source.
 */
void CRulerRichEditCtrl::SetRTF( const CString& rtf )
{
	CRichEditUtf8Source source{ rtf };
	EDITSTREAM stream{ source.Stream() };
	m_rtf.StreamIn( SF_RTF, stream );
}

/**
 * @brief Replaces the selection with plain text (EM_SETTEXTEX, UTF-16, undo kept).
 * @param sText The text to insert.
 */
void CRulerRichEditCtrl::SetText(CString sText)
{
	SETTEXTEX stex{};
	stex.flags = ST_SELECTION | ST_KEEPUNDO;
	stex.codepage = 1200;  // Unicode code page (see SETTEXTEX documentation)
	m_rtf.SendMessage(EM_SETTEXTEX, (WPARAM)&stex, (LPARAM)sText.GetString());
}

/**
 * @brief Returns the contents of the control as plain text with CRLF line ends.
 * @return The text (EM_GETTEXTEX, UTF-16).
 */
CString CRulerRichEditCtrl::GetText()
{
	// room for the text plus CRLF expansion, as UTF-16
	std::vector<wchar_t> text(static_cast<std::size_t>(m_rtf.GetTextLength()) + 50, L'\0');

	GETTEXTEX stex{};
	stex.codepage = 1200;  // Unicode code page (see GETTEXTEX documentation)
	stex.flags = GT_USECRLF;
	stex.cb = static_cast<DWORD>(text.size() * sizeof(wchar_t));
	m_rtf.SendMessage(EM_GETTEXTEX, (WPARAM)&stex, (LPARAM)text.data());

	return CString(text.data());
}

void CRulerRichEditCtrl::SetMode( int /*mode*/ )
/* ============================================================
	Function :		CRulerRichEditCtrl::SetMode
	Description :	Sets the internal mode, that is, if the 
					ruler should display inches or centimeters.
	Access :		Public
					
	Return :		void
	Parameters :	int mode	-	Mode to use, "MODE_INCH" or 
									"MODE_METRIC" (default)
					
	Usage :			Call to change the mode.

   ============================================================*/
{

	//m_ruler.SetMode( mode );

}

int CRulerRichEditCtrl::GetMode() const
/* ============================================================
	Function :		CRulerRichEditCtrl::GetMode
	Description :	Gets the mode, that is, either "MODE_INCH" or 
					"MODE_METRIC", that is used to draw the ruler.
	Access :		Public         
					
	Return :		int		-	The mode, either "MODE_INCH" or 
								"MODE_METRIC"
	Parameters :	none

	Usage :			Call to get the current mode.

   ============================================================*/
{

	return 0;
	//return m_ruler.GetMode();

}

CRichEditCtrl& CRulerRichEditCtrl::GetRichEditCtrl()
/* ============================================================
	Function :		CRulerRichEditCtrl::GetRichEditCtrl
	Description :	Returns an alias to the embedded RTF-control.
	Access :		Public
					
	Return :		CRichEditCtrl&	-	An alias to the rtf-control
	Parameters :	none

	Usage :			Call to access the RTF-control directly.

   ============================================================*/
{

	return m_rtf;

}

/////////////////////////////////////////////////////////////////////////////
// CRulerRichEditCtrl toolbar button handlers

void CRulerRichEditCtrl::OnButtonFont() 
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonFont
	Description :	Button handler for the Font button
	Access :		Protected
					
	Return :		void
	Parameters :	none

	Usage :			Called from MFC

   ============================================================*/
{

	DoFont();


}

void CRulerRichEditCtrl::OnButtonColor() 
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonColor
	Description :	Button handler for the Color button
	Access :		Protected
					
	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoColor();

}

void CRulerRichEditCtrl::OnButtonBold() 
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonBold
	Description :	Button handler for the Bold button
	Access :		Protected
					
	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{
	DoBold();
}

int CRulerRichEditCtrl::ToolbarIdPerDPI()
{
	int scale = m_dpi.Scale(100);

	if (scale >= 225)
	{
		return IDR_EDIT_WND_FORMAT_225;
	}
	else if (scale >= 200)
	{
		return IDR_EDIT_WND_FORMAT_200;
	}
	else if (scale >= 175)
	{
		return IDR_EDIT_WND_FORMAT_175;
	}
	else if (scale >= 150)
	{
		return IDR_EDIT_WND_FORMAT_150;
	}
	else if (scale >= 125)
	{
		return IDR_EDIT_WND_FORMAT_125;
	}
	
	return IDR_EDIT_WND_FORMAT;	
}

void CRulerRichEditCtrl::OnButtonWrap()
{
	DoWrap();
}

void CRulerRichEditCtrl::OnButtonItalic() 
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonItalic
	Description :	Button handler for the Italic button
	Access :		Protected
					
	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoItalic();

}

void CRulerRichEditCtrl::OnButtonUnderline() 
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonUnderline
	Description :	Button handler for the Underline button
	Access :		Protected
					
	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoUnderline();

}

void CRulerRichEditCtrl::OnButtonLeftAlign() 
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonLeftAlign
	Description :	Button handler for the Left aligned button
	Access :		Protected
					
	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoLeftAlign();

}

void CRulerRichEditCtrl::OnButtonCenterAlign() 
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonCenterAlign
	Description :	Button handler for the Center button
	Access :		Protected
					
	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoCenterAlign();

}

void CRulerRichEditCtrl::OnButtonRightAlign() 
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonRightAlign
	Description :	Button handler for the Right-aligned button
	Access :		Protected
					
	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoRightAlign();

}

void CRulerRichEditCtrl::OnButtonIndent() 
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonIndent
	Description :	Button handler for the Indent button
	Access :		Protected
					
	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoIndent();

}

void CRulerRichEditCtrl::OnButtonOutdent() 
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonOutdent
	Description :	Button handler for the outdent button
	Access :		Protected
					
	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoOutdent();

}

void CRulerRichEditCtrl::OnButtonBullet() 
/* ============================================================
	Function :		CRulerRichEditCtrl::OnButtonBullet
	Description :	Button handler for the Bullet button
	Access :		Protected
					
	Return :		void
	Parameters :	none

	Usage :			Called from MFC.

   ============================================================*/
{

	DoBullet();

}

/////////////////////////////////////////////////////////////////////////////
// CRulerRichEditCtrl private helpers

void CRulerRichEditCtrl::SetTabStops( LPLONG tabs, int size )
/* ============================================================
	Function :		CRulerRichEditCtrl::SetTabStops
	Description :	Set the tab stops in the internal tab stop 
					list from the RTF-control, converting the 
					twip values to physical pixels.
	Access :		Private
					
	Return :		void
	Parameters :	LPLONG tabs	-	A pointer to an array of 
									"LONG" twip values.
					int size	-	The size of "tabs"
					
	Usage :			Call to set the tab list.

   ============================================================*/
{

	m_tabs.RemoveAll();

	double twip = ( double )m_physicalInch / 1440;
	for( int t = 0 ; t < size ; t++ )
	{
		// Convert from twips to pixels
		int tabpos = *( tabs + t );
		tabpos = ( int ) ( ( double ) tabpos * twip +.5 );
		m_tabs.Add( tabpos );

	}

	//m_ruler.SetTabStops( m_tabs );
}

void CRulerRichEditCtrl::UpdateTabStops()
/* ============================================================
	Function :		CRulerRichEditCtrl::UpdateTabStops
	Description :	Sets the tabs in the internal tab stop 
					list, converting the twip physical (pixel) 
					position to twip values.
	Access :		Private
					
	Return :		void
	Parameters :	none

	Usage :			Call to refresh the tab list from the RTF-
					control. Called from the "OnPaint" handler.

   ============================================================*/
{

	ParaFormat para( PFM_TABSTOPS );
	m_rtf.GetParaFormat( para );
	SetTabStops( (LPLONG)(para.rgxTabs), MAX_TAB_STOPS );

}

void CRulerRichEditCtrl::UpdateToolbarButtons()
/* ============================================================
	Function :		CRulerRichEditCtrl::UpdateToolbarButtons
	Description :	Updates the toolbar button, by getting 
					formatting information from the currently 
					selected text in the embedded RTF-control.
	Access :		Private
					
	Return :		void
	Parameters :	none

	Usage :			Call as the selection changes in the 
					RTF-control

   ============================================================*/
{
	if( m_showToolbar && m_toolbar.m_hWnd )
	{
		CharFormat	cf;
		cf.dwMask = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;
		m_rtf.SendMessage( EM_GETCHARFORMAT, SCF_SELECTION, ( LPARAM ) &cf );

		ParaFormat para( PFM_ALIGNMENT | PFM_NUMBERING );
		m_rtf.GetParaFormat( para );

		// Style
		m_toolbar.SetState( BUTTON_BOLD, ToolbarButtonState( ( cf.dwEffects & CFE_BOLD ) != 0 ) );
		m_toolbar.SetState( BUTTON_ITALIC, ToolbarButtonState( ( cf.dwEffects & CFE_ITALIC ) != 0 ) );
		m_toolbar.SetState( BUTTON_UNDERLINE, ToolbarButtonState( ( cf.dwEffects & CFM_UNDERLINE ) != 0 ) );
		m_toolbar.SetState( BUTTON_LEFTALIGN, ToolbarButtonState( para.wAlignment == PFA_LEFT ) );
		m_toolbar.SetState( BUTTON_CENTERALIGN, ToolbarButtonState( para.wAlignment == PFA_CENTER ) );
		m_toolbar.SetState( BUTTON_RIGHTALIGN, ToolbarButtonState( para.wAlignment == PFA_RIGHT ) );
		m_toolbar.SetState( BUTTON_BULLET, ToolbarButtonState( para.wNumbering != 0 ) );
		m_toolbar.SetState( ID_BUTTONWRAP, ToolbarButtonState( m_bInWrapMode != FALSE ) );

		UpdateToolbarFont( cf );
	}
}

UINT CRulerRichEditCtrl::ToolbarButtonState( bool checked )
{
	return TBSTATE_ENABLED | ( checked ? TBSTATE_CHECKED : 0 );
}

void CRulerRichEditCtrl::UpdateToolbarFont( const CharFormat& cf )
{
	if( cf.dwMask & CFM_FACE )
		m_toolbar.SetFontName( CString( cf.szFaceName ) );

	if( cf.dwMask & CFM_SIZE )
		m_toolbar.SetFontSize( cf.yHeight / 20 );

	if( cf.dwMask & CFM_COLOR )
		m_toolbar.SetFontColor( cf.crTextColor );
}

void CRulerRichEditCtrl::SetEffect( int mask, int effect )
/* ============================================================
	Function :		CRulerRichEditCtrl::SetEffect
	Description :	Sets the effect (bold, italic and/or 
					underline) for the currently selected text 
					in the embedded RTF-control.
	Access :		Private
					
	Return :		void
	Parameters :	int mask	-	What effects are valid. See 
									the documentation for 
									"CHARFORMAT".
					int effect	-	What effects to set. See the 
									documentation for "CHARFORMAT".
					
	Usage :			Called internally from button handlers

   ============================================================*/
{

	CharFormat cf;
	cf.dwMask = mask;
	cf.dwEffects = effect;

	m_rtf.SendMessage( EM_SETCHARFORMAT, SCF_SELECTION, ( LPARAM ) &cf );
	m_rtf.SetFocus();

}

void CRulerRichEditCtrl::SetAlignment( int alignment )
/* ============================================================
	Function :		CRulerRichEditCtrl::SetAlignment
	Description :	Sets the alignment for the currently 
					selected text in the embedded RTF-control.
	Access :		Private
					
	Return :		void
	Parameters :	int alignment	-	Alignment to set. See
										documentation for
										"PARAFORMAT"
					
	Usage :			Called internally from button handlers

   ============================================================*/
{

	ParaFormat	para( PFM_ALIGNMENT );
	para.wAlignment = ( WORD ) alignment;

	m_rtf.SetParaFormat( para );
	UpdateToolbarButtons();
	m_rtf.SetFocus();

}

// Virtual interface
void CRulerRichEditCtrl::DoFont()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoFont
	Description :	Externally accessible member to set the
					font of the control
	Access :		Public
					
	Return :		void
	Parameters :	none
					
	Usage :			Call to set the font of the selected text.

   ============================================================*/
{

	// Get the current font
	LOGFONT	lf;
	ZeroMemory( &lf, sizeof( LOGFONT ) );
	CharFormat	cf;
	m_rtf.SendMessage( EM_GETCHARFORMAT, SCF_SELECTION, ( LPARAM ) &cf );

	// Creating a LOGFONT from the current font settings
	CharFormatToLogFont( cf, lf );

	// Show font dialog
	CFontDialog	dlg(&lf);
	if(dlg.DoModal() == IDOK)
	{
		// Apply new font
		FontDialogToCharFormat( dlg, cf );

		m_rtf.SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf);

		m_rtf.SetFocus();
	}
}

void CRulerRichEditCtrl::CharFormatToLogFont( const CharFormat& cf, LOGFONT& lf ) const
{
	int height{};

	// Font
	if( cf.dwMask & CFM_FACE )
		lstrcpy( lf.lfFaceName, cf.szFaceName );

	if( cf.dwMask & CFM_SIZE )
	{
		double twip = ( double )m_physicalInch / 1440;
		height = cf.yHeight;
		height = -( int ) ( ( double ) height * twip +.5 );
		lf.lfHeight = height;

	}

	// Effects
	CharEffectsToLogFont( cf, lf );
}

void CRulerRichEditCtrl::CharEffectsToLogFont( const CharFormat& cf, LOGFONT& lf )
{
	if( cf.dwMask & CFM_BOLD )
	{
		if( cf.dwEffects & CFE_BOLD )
			lf.lfWeight = FW_BOLD;
		else
			lf.lfWeight = FW_NORMAL;
	}

	if( cf.dwMask & CFM_ITALIC )
		if( cf.dwEffects & CFE_ITALIC )
			lf.lfItalic = TRUE;

	if( cf.dwMask & CFM_UNDERLINE )
		if( cf.dwEffects & CFE_UNDERLINE )
			lf.lfUnderline = TRUE;
}

void CRulerRichEditCtrl::FontDialogToCharFormat( CFontDialog& dlg, CharFormat& cf )
{
	cf.yHeight = dlg.GetSize() * 2;
	lstrcpy(cf.szFaceName, dlg.GetFaceName());

	cf.dwMask = CFM_FACE | CFM_SIZE;
	cf.dwEffects = 0;

	if( dlg.IsBold() )
	{
		cf.dwMask |= CFM_BOLD;
		cf.dwEffects |= CFE_BOLD;
	}

	if( dlg.IsItalic() )
	{
		cf.dwMask |= CFM_ITALIC;
		cf.dwEffects |= CFE_ITALIC;
	}

	if( dlg.IsUnderline() )
	{
		cf.dwMask |= CFM_UNDERLINE;
		cf.dwEffects |= CFE_UNDERLINE;
	}
}

void CRulerRichEditCtrl::SetCurrentFontName( const CString& font )
/* ============================================================
	Function :		CRulerRichEditCtrl::SetCurrentFontName
	Description :	Changes the font of the selected text in 
					the editor to "font".
	Access :		Public
					
	Return :		void
	Parameters :	const CString& font	-	Font name of font 
											to change to.

	Usage :			Call to set the font of the selected text 
					in the editor.

   ============================================================*/
{
	CharFormat	cf;
	cf.dwMask = CFM_FACE;

	lstrcpy( cf.szFaceName, font );

	m_rtf.SendMessage( EM_SETCHARFORMAT, SCF_SELECTION, ( LPARAM ) &cf );
}

void CRulerRichEditCtrl::SetCurrentFontSize( int size )
/* ============================================================
	Function :		CRulerRichEditCtrl::SetCurrentFontSize
	Description :	Changes the size of the selected text in 
					the editor to "size" (measured in 
					typographical points).
	Access :		Public
					
	Return :		void
	Parameters :	int size	-	New size in typographical 
									points

	Usage :			Call to change the size of the selected 
					text.

   ============================================================*/
{
	CharFormat	cf;
	cf.dwMask = CFM_SIZE;
	cf.yHeight = size * 20;

	m_rtf.SendMessage( EM_SETCHARFORMAT, SCF_SELECTION, ( LPARAM ) &cf );
}

void CRulerRichEditCtrl::SetCurrentFontColor( COLORREF color )
/* ============================================================
	Function :		CRulerRichEditCtrl::SetCurrentFontSize
	Description :	Changes the color of the selected text in 
					the editor to "color".
	Access :		Public
					
	Return :		void
	Parameters :	COLORREF color	-	New color

	Usage :			Call to change the color of the selected 
					text.

   ============================================================*/
{

	CharFormat	cf;
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = color;

	m_rtf.SendMessage( EM_SETCHARFORMAT, SCF_SELECTION, ( LPARAM ) &cf );

}

LRESULT CRulerRichEditCtrl::OnSetCurrentFontName( WPARAM font, LPARAM )
/* ============================================================
	Function :		CRulerRichEditCtrl::OnSetCurrentFontName
	Description :	Handler for the registered message 
					"SetCurrentFontNameMessage()", called when the
					font name is changed from the toolbar.
	Access :		Protected

	Return :		LRESULT		-	Not used
	Parameters :	WPARAM font	-	Pointer to the new font name
					LPARAM		-	Not used

	Usage :			Called from MFC

   ============================================================*/
{

	CString fnt( ( LPCTSTR ) font );
	SetCurrentFontName( fnt );

	return 0;
	
}

LRESULT CRulerRichEditCtrl::OnSetCurrentFontSize(WPARAM, LPARAM size)
/* ============================================================
	Function :		CRulerRichEditCtrl::OnSetCurrentFontSize
	Description :	Handler for the registered message 
					"SetCurrentFontSizeMessage()", called when the
					font size is changed from the toolbar.
	Access :		Protected

	Return :		LRESULT		-	Not used
	Parameters :	WPARAM		-	Not used
					LPARAM size	-	New font size in typographical 
									points of the selected text

	Usage :			Called from MFC

   ============================================================*/
{

	SetCurrentFontSize((int)size);
	return 0;
	
}

LRESULT CRulerRichEditCtrl::OnSetCurrentFontColor(WPARAM, LPARAM color)
/* ============================================================
	Function :		CRulerRichEditCtrl::OnSetCurrentFontColor
	Description :	Handler for the registered message 
					"SetCurrentFontColorMessage()", called when the
					font color is changed from the toolbar.
	Access :		Protected

	Return :		LRESULT		-	Not used
	Parameters :	WPARAM		-	Not used
					LPARAM		-	New color of the selected 
									text

	Usage :			Called from MFC

   ============================================================*/
{

	SetCurrentFontColor( ( COLORREF ) color );
	return 0;
	
}

void CRulerRichEditCtrl::DoColor()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoColor
	Description :	Externally accessible member to set the
					color of the selected text
	Access :		Public
					
	Return :		void
	Parameters :	none
					
	Usage :			Call to set the color of the selected text.

   ============================================================*/
{

	// Get the current color
	COLORREF	clr( RGB( 0, 0, 0 ) );
	CharFormat	cf;
	m_rtf.SendMessage( EM_GETCHARFORMAT, SCF_SELECTION, ( LPARAM ) &cf );
	if( cf.dwMask & CFM_COLOR )
		clr = cf.crTextColor;

	// Display color selection dialog
	CColorDialog dlg( clr );
	if( dlg.DoModal() == IDOK )
	{
		// Apply new color
		cf.dwMask = CFM_COLOR;
		cf.dwEffects = 0;
		cf.crTextColor = dlg.GetColor();

		m_rtf.SendMessage( EM_SETCHARFORMAT, SCF_SELECTION, ( LPARAM ) &cf );

	}

	m_rtf.SetFocus();

}

void CRulerRichEditCtrl::DoWrap()
{
	if(m_bInWrapMode)
	{
		// Turn off word wrap.
		m_rtf.SetTargetDevice(NULL, 1);
		m_bInWrapMode = false;
	}
	else
	{
		// Turn on word wrap.
		m_rtf.SetTargetDevice(NULL, 0); 
		m_bInWrapMode = true;
	}

	theApp.Services().Settings().SetEditWordWrap(m_bInWrapMode);

	m_toolbar.CheckButton(ID_BUTTONWRAP, m_bInWrapMode);
}

void CRulerRichEditCtrl::DoBold()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoBold
	Description :	Externally accessible member to set/unset
					the selected text to/from bold
	Access :		Public
					
	Return :		void
	Parameters :	none
					
	Usage :			Call to toggle the selected text to/from 
					bold.

   ============================================================*/
{
	m_toolbar.CheckButton( BUTTON_BOLD, !m_toolbar.IsButtonChecked( BUTTON_BOLD ) );

	int effect = 0;
	if( m_toolbar.IsButtonChecked( BUTTON_BOLD ) )
		effect = CFE_BOLD;

	SetEffect( CFM_BOLD, effect );
}

void CRulerRichEditCtrl::DoItalic()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoItalic
	Description :	Externally accessible member to set/unset
					the selected text to/from italic
	Access :		Public
					
	Return :		void
	Parameters :	none
					
	Usage :			Call to toggle the selected text to/from 
					italic.

   ===========================================================  =*/
{

	m_toolbar.CheckButton( BUTTON_ITALIC, !m_toolbar.IsButtonChecked( BUTTON_ITALIC ) );

	int effect = 0;
	if( m_toolbar.IsButtonChecked( BUTTON_ITALIC ) )
		effect = CFE_ITALIC;

	SetEffect( CFM_ITALIC, effect );
}

void CRulerRichEditCtrl::DoUnderline()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoUnderline
	Description :	Externally accessible member to set/unset
					the selected text to/from underline
	Access :		Public
					
	Return :		void
	Parameters :	none
					
	Usage :			Call to toggle the selected text to/from 
					underlined.

   ============================================================*/
{

	m_toolbar.CheckButton( BUTTON_UNDERLINE, !m_toolbar.IsButtonChecked( BUTTON_UNDERLINE ) );

	int effect = 0;
	if( m_toolbar.IsButtonChecked( BUTTON_UNDERLINE ) )
		effect = CFE_UNDERLINE;

	SetEffect( CFM_UNDERLINE, effect );
}

void CRulerRichEditCtrl::DoLeftAlign()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoLeftAlign
	Description :	Externally accessible member to set the
					selected text to left aligned.
	Access :		Public
					
	Return :		void
	Parameters :	none
					
	Usage :			Call to left-align the selected text

   ============================================================*/
{

	if( !m_toolbar.IsButtonChecked( BUTTON_LEFTALIGN ) )
		SetAlignment( PFA_LEFT );
}

void CRulerRichEditCtrl::DoCenterAlign()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoCenterAlign
	Description :	Externally accessible member to set the
					selected text to center aligned
	Access :		Public
					
	Return :		void
	Parameters :	none
					
	Usage :			Call to center-align the selected text

   ============================================================*/
{
	if( !m_toolbar.IsButtonChecked( BUTTON_CENTERALIGN ) )
		SetAlignment( PFA_CENTER );
}

void CRulerRichEditCtrl::DoRightAlign()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoRightAlign
	Description :	Externally accessible member to set the
					selected text to right aligned
	Access :		Public
					
	Return :		void
	Parameters :	none
					
	Usage :			Call to right-align the selected text

   ============================================================*/
{
	if( !m_toolbar.IsButtonChecked( BUTTON_RIGHTALIGN ) )
		SetAlignment( PFA_RIGHT );
}

void CRulerRichEditCtrl::DoIndent()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoIndent
	Description :	Externally accessible member to indent the
					selected text to the next tab position
	Access :		Public
					
	Return :		void
	Parameters :	none
					
	Usage :			Call to indent the selected text

   ============================================================*/
{
	// Get current indent
	ParaFormat	para( PFM_STARTINDENT | PFM_TABSTOPS );
	m_rtf.GetParaFormat( para );
	int newindent = para.dxStartIndent;

	// Find next larger tab
	for( int t = MAX_TAB_STOPS - 1 ; t >= 0 ; t-- )
	{

		if( para.rgxTabs[ t ] > para.dxStartIndent )
			newindent = para.rgxTabs[ t ];

	}

	if( newindent != para.dxStartIndent )
	{

		// Set indent to this value
		para.dwMask = PFM_STARTINDENT | PFM_OFFSET;
		para.dxStartIndent = newindent;
		para.dxOffset = newindent;

		m_rtf.SetParaFormat( para );

	}

	m_rtf.SetFocus();
}

void CRulerRichEditCtrl::DoOutdent()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoOutdent
	Description :	Externally accessible member to outdent the
					selected text to the previous tab position
	Access :		Public
					
	Return :		void
	Parameters :	none
					
	Usage :			Call to outdent the selected text

   ============================================================*/
{
	// Get the current indent, if any
	ParaFormat	para( PFM_STARTINDENT | PFM_TABSTOPS );
	m_rtf.GetParaFormat( para );
	int newindent = 0;

	// Find closest smaller tab
	for( int t = 0 ; t < MAX_TAB_STOPS ; t++ )
		if( para.rgxTabs[ t ] < para.dxStartIndent )
			newindent = para.rgxTabs[ t ];

	// Set indent to this value or 0 if none
	para.dwMask = PFM_STARTINDENT | PFM_OFFSET;
	para.dxStartIndent = newindent;
	para.dxOffset = newindent;

	m_rtf.SetParaFormat( para );
	m_rtf.SetFocus();
}

void CRulerRichEditCtrl::DoBullet()
/* ============================================================
	Function :		CRulerRichEditCtrl::DoBullet
	Description :	Externally accessible member to set the
					selected text to bulleted
	Access :		Public
					
	Return :		void
	Parameters :	none
					
	Usage :			Call to set the selected text to bulleted.

   ============================================================*/
{
	m_toolbar.CheckButton( BUTTON_BULLET, !m_toolbar.IsButtonChecked( BUTTON_BULLET ) );

	ParaFormat	para( PFM_NUMBERING );
	if( m_toolbar.IsButtonChecked( BUTTON_BULLET ) )
		para.wNumbering = PFN_BULLET;
	else
		para.wNumbering = 0;

	m_rtf.SetParaFormat( para );
	m_rtf.SetFocus();
}

void CRulerRichEditCtrl::ShowToolbar( BOOL show )
/* ============================================================
	Function :		CRulerRichEditCtrl::ShowToolbar
	Description :	Shows or hides the toolbar
	Access :		Public
					
	Return :		void
	Parameters :	BOOL show	-	"TRUE" to show
					
	Usage :			Call to show or hide the toolbar subcontrol

   ============================================================*/
{
	m_showToolbar = show;

	if( m_hWnd )
	{
		if( show )
			m_toolbar.ShowWindow( SW_SHOW );
		else
			m_toolbar.ShowWindow( SW_HIDE );

		CRect rect;
		GetClientRect( rect );
		LayoutControls( rect.Width(), rect.Height() );
	}
}

void CRulerRichEditCtrl::LayoutControls( int width, int height )
/* ============================================================
	Function :		CRulerRichEditCtrl::LayoutControls
	Description :	Lays out the sub-controls depending on 
					visibility.
	Access :		Private
					
	Return :		void
	Parameters :	int width	-	Width of control
					int height	-	Height of control
					
	Usage :			Called internally to lay out the controls

   ============================================================*/
{
	int toolbarHeight = 0;
	if( m_showToolbar )
		toolbarHeight = m_dpi.Scale(s_toolbarHeight);

	m_toolbar.MoveWindow( 0, 0, width, toolbarHeight );

	int top = toolbarHeight;
	CRect rect( 0, top, width, height );
	m_rtf.MoveWindow( rect );
}

BOOL CRulerRichEditCtrl::IsToolbarVisible() const
/* ============================================================
	Function :		CRulerRichEditCtrl::IsToolbarVisible
	Description :	Returns if the toolbar is visible or not
	Access :		Public
					
	Return :		BOOL	-	"TRUE" if visible
	Parameters :	none
					
	Usage :			Call to get the visibility of the toolbar

   ============================================================*/
{
	return m_showToolbar;
}

void CRulerRichEditCtrl::SetReadOnly( BOOL readOnly )
/* ============================================================
	Function :		CRulerRichEditCtrl::SetReadOnly
	Description :	Sets the control to read only or not.
	Access :		Public
					
	Return :		void
	Parameters :	BOOL readOnly	-	New read only state
					
	Usage :			Call to set the read only state of the 
					control

   ============================================================*/
{
	if( m_rtf.m_hWnd )
		m_rtf.SetReadOnly( readOnly );

	m_readOnly = readOnly;
}

BOOL CRulerRichEditCtrl::GetReadOnly() const
/* ============================================================
	Function :		CRulerRichEditCtrl::GetReadOnly
	Description :	Returns if the control is read only or not
	Access :		Public
					
	Return :		BOOL	-	"TRUE" if read only
	Parameters :	none
					
	Usage :			Call to get the read only-state of the 
					control

   ============================================================*/
{
	return m_readOnly;
}

BOOL CRulerRichEditCtrl::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		// Ctrl + key shortcuts; the key is checked before the Ctrl state
		const std::array<ControlShortcut, 9> shortcuts{ {
			{ 'X', &CRulerRichEditCtrl::RtfCut },
			{ 'C', &CRulerRichEditCtrl::RtfCopy },
			{ 'V', &CRulerRichEditCtrl::RtfPaste },
			{ 'I', &CRulerRichEditCtrl::DoItalic },
			{ 'B', &CRulerRichEditCtrl::DoBold },
			{ 'U', &CRulerRichEditCtrl::DoUnderline },
			{ 'Z', &CRulerRichEditCtrl::RtfUndo },
			{ 'Y', &CRulerRichEditCtrl::RtfRedo },
			{ 'W', &CRulerRichEditCtrl::DoWrap }
		} };

		if(RunControlShortcut(pMsg->wParam, shortcuts))
		{
			return TRUE;
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}

bool CRulerRichEditCtrl::RunControlShortcut(WPARAM key, std::span<const ControlShortcut> shortcuts)
{
	for(const ControlShortcut& shortcut : shortcuts)
	{
		if(shortcut.key == key)
		{
			if(CKeyboard::IsControlPressed())
			{
				(this->*shortcut.handler)();
				return true;
			}
			return false;
		}
	}

	return false;
}

void CRulerRichEditCtrl::RtfCut()
{
	m_rtf.Cut();
}

void CRulerRichEditCtrl::RtfCopy()
{
	m_rtf.Copy();
}

void CRulerRichEditCtrl::RtfPaste()
{
	m_rtf.Paste();
}

void CRulerRichEditCtrl::RtfUndo()
{
	m_rtf.Undo();
}

void CRulerRichEditCtrl::RtfRedo()
{
	m_rtf.Redo();
}
