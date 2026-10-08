// DittoUtil.h : main header file for the DittoUtil DLL
//

#pragma once

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h" // main symbols
#include "PasteImageAsHtmlImage.h"


// CDittoUtilApp
// See DittoUtil.cpp for the implementation of this class
//

class CDittoUtilApp : public CWinApp
{
public:
	CDittoUtilApp();

	/**
	 * @brief The add-in DLL's one application object.
	 * @return The application object.
	 */
	static CDittoUtilApp& Instance();

	/**
	 * @brief The "paste image as HTML image tag" function, whose image folder and file numbering last as long as the DLL.
	 * @return The DLL's one CPasteImageAsHtmlImage.
	 */
	CPasteImageAsHtmlImage& PasteImageAsHtml();

	// Overrides
public:
	virtual BOOL InitInstance();
	virtual BOOL ExitInstance();

	DECLARE_MESSAGE_MAP()

private:
	/** @brief The DLL's one CPasteImageAsHtmlImage (its pasted images are deleted in ExitInstance). */
	CPasteImageAsHtmlImage m_pasteImageAsHtml{};
};
