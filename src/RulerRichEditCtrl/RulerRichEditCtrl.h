#if !defined(AFX_RULERRICHEDITCTRL_H__4CD13283_82E4_484A_83B4_DBAD5B64F17C__INCLUDED_)
#define AFX_RULERRICHEDITCTRL_H__4CD13283_82E4_484A_83B4_DBAD5B64F17C__INCLUDED_

#include <array>
#include <span>

#include "RulerRichEdit.h"
#include "RRECToolbar.h"
#include "../DPI.h"
#include "../RichEditCtrlEx.h"

/////////////////////////////////////////////////////////////////////////////
// Helper structs

#ifdef _UNICODE
struct CharFormat : public CHARFORMATW
#else
struct CharFormat : public CHARFORMAT
#endif
{
	CharFormat()
	{
		memset( this, 0, sizeof ( CharFormat ) );
		cbSize = sizeof( CharFormat );
	};

};

struct ParaFormat : public PARAFORMAT
{
	ParaFormat( DWORD mask )
	{
		memset( this, 0, sizeof ( ParaFormat ) );
		cbSize = sizeof( ParaFormat );
		dwMask = mask;
	}
};

/////////////////////////////////////////////////////////////////////////////
// CRulerRichEditCtrl window

class CRulerRichEditCtrl : public CWnd
{

public:
// Construction/creation/destruction
	CRulerRichEditCtrl();
	virtual ~CRulerRichEditCtrl();
	virtual BOOL Create( DWORD dwStyle, const RECT &rect, CWnd* pParentWnd, UINT nID, BOOL autohscroll = FALSE );

// Registered messages for ruler/toolbar/CRulerRichEditCtrl communication. Each is registered on the
// first call; the getters return a reference because ON_REGISTERED_MESSAGE takes the id's address.
	/**
	 * @brief The registered message the toolbar sends when the user picks a font name (WPARAM: the LPCTSTR name).
	 * @return The message id.
	 */
	static const UINT& SetCurrentFontNameMessage();
	/**
	 * @brief The registered message the toolbar sends when the user picks a font size (LPARAM: the size).
	 * @return The message id.
	 */
	static const UINT& SetCurrentFontSizeMessage();
	/**
	 * @brief The registered message the toolbar sends when the user picks a font colour (LPARAM: the COLORREF).
	 * @return The message id.
	 */
	static const UINT& SetCurrentFontColorMessage();

// Attributes
	void	SetMode( int mode );
	int		GetMode() const;

	void ShowToolbar( BOOL show = TRUE );

	BOOL IsToolbarVisible() const;

	CRichEditCtrl& GetRichEditCtrl( );

// Implementation
	CString GetRTF();
	void	SetRTF( const CString& rtf );
	void	SetText(CString sText);
	CString GetText();

	void SetReadOnly( BOOL readOnly );
	BOOL GetReadOnly() const;
	void OnDpiChanged(CWnd* pParent, int dpi);

// Formatting
	virtual void DoFont();
	virtual void DoColor();
	virtual void DoBold();
	virtual void DoItalic();
	virtual void DoUnderline();
	virtual void DoLeftAlign();
	virtual void DoCenterAlign();
	virtual void DoRightAlign();
	virtual void DoIndent();
	virtual void DoOutdent();
	virtual void DoBullet();
	virtual void DoWrap();

	void SetCurrentFontName( const CString& font );
	void SetCurrentFontSize( int points );
	void SetCurrentFontColor( COLORREF color );

// Overrides
	//{{AFX_VIRTUAL(CRulerRichEditCtrl)
	protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

protected:
// Message handlers
	//{{AFX_MSG(CRulerRichEditCtrl)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnButtonFont();
	afx_msg void OnButtonColor();
	afx_msg void OnButtonBold();
	afx_msg void OnButtonItalic();
	afx_msg void OnButtonUnderline();
	afx_msg void OnButtonLeftAlign();
	afx_msg void OnButtonCenterAlign();
	afx_msg void OnButtonRightAlign();
	afx_msg void OnButtonIndent();
	afx_msg void OnButtonOutdent();
	afx_msg void OnButtonBullet();
	afx_msg void OnButtonWrap();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg LRESULT OnSetText (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetText (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetTextLength (WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG

	LRESULT OnSetCurrentFontName(WPARAM font, LPARAM size);
	LRESULT OnSetCurrentFontSize(WPARAM font, LPARAM size);
	LRESULT OnSetCurrentFontColor(WPARAM font, LPARAM size);

	DECLARE_MESSAGE_MAP()

protected:
	// Internal data
	CDWordArray		m_tabs;				// An array containing the tab-positions in device pixels
	int				m_margin;			// The margin to use for the ruler and buttons

	int				m_physicalInch;		// The number of pixels for an inch on screen
	int				m_movingtab;		// The tab-position being moved, or -1 if none
	int				m_offset;			// Internal offset of the tab-marker being moved.

	BOOL			m_showToolbar;
	BOOL			m_readOnly;

	BOOL			m_bInWrapMode;

	// Sub-controls
	CRulerRichEdit	m_rtf;
	CRRECToolbar	m_toolbar;	

	// Private helpers
	void	SetTabStops( LPLONG tabs, int size );
	void	UpdateTabStops();

	BOOL	CreateToolbar();
	BOOL	CreateRTFControl( BOOL autohscroll );
	void	CreateMargins();

	void	UpdateToolbarButtons();

	void	SetEffect( int mask, int effect );
	void	SetAlignment( int alignment );

	void	LayoutControls( int width, int height );
	int		ToolbarIdPerDPI();

	CDPI m_dpi;

	/** @brief The height of the formatting toolbar, in unscaled pixels. */
	static constexpr int s_toolbarHeight{28};

	/**
	 * @brief Rich edit style SES_HYPERLINKTOOLTIPS (Richedit.h, _RICHEDIT_VER >= 0x0500). MFC's
	 * afxwin.h sets _RICHEDIT_VER to 0x0210 before Richedit.h, so the SDK name is not defined here.
	 */
	static constexpr DWORD s_sesHyperlinkTooltips{8};
	/** @brief Rich edit style SES_NOFOCUSLINKNOTIFY (Richedit.h, _RICHEDIT_VER >= 0x0500); see s_sesHyperlinkTooltips. */
	static constexpr DWORD s_sesNoFocusLinkNotify{32};

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

private:
	/** @brief One Ctrl + key shortcut of PreTranslateMessage. */
	struct ControlShortcut
	{
		/** @brief Virtual key code (an upper case letter). */
		WPARAM key{};
		/** @brief Member function that runs the shortcut. */
		void (CRulerRichEditCtrl::*handler)() = nullptr;
	};

	/** @brief Runs the shortcut of a key when Ctrl is pressed.
	 *  @param key Virtual key code of the WM_KEYDOWN message.
	 *  @param shortcuts The shortcut table.
	 *  @return true if the key has a shortcut, Ctrl is pressed and the shortcut ran. */
	bool RunControlShortcut(WPARAM key, std::span<const ControlShortcut> shortcuts);

	/** @brief Cuts the selection of the embedded RTF control. */
	void RtfCut();
	/** @brief Copies the selection of the embedded RTF control. */
	void RtfCopy();
	/** @brief Pastes into the embedded RTF control. */
	void RtfPaste();
	/** @brief Undoes the last change of the embedded RTF control. */
	void RtfUndo();
	/** @brief Redoes the last undone change of the embedded RTF control. */
	void RtfRedo();

	/** @brief Toolbar button state: enabled, and checked if asked.
	 *  @param checked true to show the button as checked.
	 *  @return TBSTATE_ENABLED, with TBSTATE_CHECKED if checked. */
	static UINT ToolbarButtonState( bool checked );

	/** @brief Shows the font name, size and colour of the selection in the toolbar.
	 *  @param cf Character format of the selection. */
	void UpdateToolbarFont( const CharFormat& cf );

	/** @brief Fills a LOGFONT from the character format of the selection.
	 *  @param cf Character format of the selection.
	 *  @param lf LOGFONT to fill. */
	void CharFormatToLogFont( const CharFormat& cf, LOGFONT& lf ) const;

	/** @brief Copies the bold, italic and underline effects into a LOGFONT.
	 *  @param cf Character format of the selection.
	 *  @param lf LOGFONT to fill. */
	static void CharEffectsToLogFont( const CharFormat& cf, LOGFONT& lf );

	/** @brief Sets a character format from the choice in the font dialog.
	 *  @param dlg Font dialog closed with OK.
	 *  @param cf Character format to set. */
	static void FontDialogToCharFormat( CFontDialog& dlg, CharFormat& cf );
};

#endif // !defined(AFX_RULERRICHEDITCTRL_H__4CD13283_82E4_484A_83B4_DBAD5B64F17C__INCLUDED_)
