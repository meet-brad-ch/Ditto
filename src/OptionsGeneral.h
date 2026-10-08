#if !defined(AFX_OPTIONSGENERAL_H__A13ABBF6_7636_4426_9A31_0189D4CA8F2F__INCLUDED_)
#define AFX_OPTIONSGENERAL_H__A13ABBF6_7636_4426_9A31_0189D4CA8F2F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptionsGeneral.h : header file
//
#include "stdafx.h"
#include "CP_Main.h"
#include "OptionsSheet.h"
#include "NumberEdit.h"
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// COptionsGeneral dialog

class COptionsGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(COptionsGeneral)

	// Construction
public:
	COptionsGeneral();
	~COptionsGeneral();

	// Dialog Data
	//{{AFX_DATA(COptionsGeneral)
	enum
	{
		IDD = IDD_OPTIONS_GENERAL
	};
	//CButton	m_EnsureConnected;
	CNumberEdit m_SaveDelay;
	CComboBox m_cbLanguage;
	CEdit m_MaxClipSize;
	CButton m_btSendPasteMessage;
	CButton m_btHideDittoOnHotKey;
	CNumberEdit m_DescTextSize;
	CEdit m_ePath;
	CNumberEdit m_eExpireAfter;
	CNumberEdit m_eMaxSavedCopies;
	CButton m_btMaximumCheck;
	CButton m_btExpire;
	CButton m_btShowIconInSysTray;
	CButton m_btRunOnStartup;
	CButton m_btSaveMultiPaste;
	CString m_csPlaySound;
	CEdit m_ClipSeparator;
	CEdit m_copyAppInclude;
	CEdit m_copyAppExclude;
	CStatic m_envVarLink;

	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COptionsGeneral)
protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support
	virtual BOOL OnApply();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	COptionsSheet* m_pParent{};
	CString m_csTitle;
	CBrush m_brush;
	LOGFONT m_LogFont;
	CFont m_envVarFont;
	CString m_originalEnvVariables;

	void FillThemes();
	void FillLanguages();
	int GetFontSize(HWND hWnd, const LOGFONT& lf);

	// Generated message map functions
	//{{AFX_MSG(COptionsGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnGetPath();
	afx_msg void OnButtonAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButtonAdvanced();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedButtonTheme();
	afx_msg void OnBnClickedButtonPreviewTheme();
	afx_msg void OnCbnSelchangeComboTheme();
	void ApplySelectedThemeToPreview();
	afx_msg void OnBnClickedButtonDefaultFault();
	afx_msg void OnBnClickedButtonFont();
	CComboBox m_cbTheme;
	CButton m_btFont;
	CButton m_btDefaultButton;
	CComboBox m_popupPositionCombo;
	//afx_msg void OnNMClickSyslinkEnvVarInfo(NMHDR *pNMHDR, LRESULT *pResult);
	//afx_msg void OnEnChangePath();
	afx_msg void OnEnChangePath();
	afx_msg void OnClickedMaximumEntries();
	afx_msg void OnClickedExpireEntries();

private:
	/** @brief The theme list's entry for the built-in theme. */
	static constexpr const TCHAR* s_defaultTheme{ _T("(Ditto)") };
	/** @brief FillLanguages' index of the English entry while it is not found yet. */
	static constexpr int s_noMatch{ -2 };

	/** @brief OnApply's language step: stores the selected language file and loads it (reports a load error). */
	void ApplyLanguage();

	/**
	 * @brief OnApply's database step: validates, creates or opens the database path the user entered.
	 * @return False when OnApply must stop (the user declined to create it, or it is invalid or cannot be opened).
	 */
	bool ApplyDatabasePath();

	/**
	 * @brief Asks whether to create a missing database and creates it.
	 * @param resolvedPath The database path, environment variables resolved.
	 * @return True when the database was created; false when the user declined or the create
	 * failed (CreateDB showed the error).
	 */
	bool PromptCreateDatabase(const CString& resolvedPath);

	/**
	 * @brief Stores the database path and opens the database (reports a failure).
	 * @param toSavePath The path as entered (stored in the options).
	 * @param resolvedPath The path with environment variables resolved (opened).
	 * @return False when the database could not be opened.
	 */
	bool OpenNewDatabase(const CString& toSavePath, const CString& resolvedPath);

	/** @brief OnApply's theme step: stores the selected theme and flags a theme change to the sheet. */
	void ApplyTheme();

	/**
	 * @brief FillThemes' step: adds the theme files of a supported version to the theme list.
	 * @param csFile The search pattern of the theme files.
	 * @param csTheme The current theme; it is selected when found.
	 * @return True when the current theme was found and selected.
	 */
	bool AddThemeFiles(const CString& csFile, const CString& csTheme);

	/** @brief Selects the "follow windows theme" entry (item data 0) of the theme list. */
	void SelectFollowWindowsTheme();

	/**
	 * @brief The application's services.
	 * @return theApp.Services().
	 */
	CAppServices& Services() const;

	/**
	 * @brief The application's settings.
	 * @return Services().Settings().
	 */
	CGetSetOptions& Settings() const;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONSGENERAL_H__A13ABBF6_7636_4426_9A31_0189D4CA8F2F__INCLUDED_)
