// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__56F3D184_7208_47FE_AFE2_E270325F356A__INCLUDED_)
//#define _ATL_APARTMENT_THREADED
#define AFX_STDAFX_H__56F3D184_7208_47FE_AFE2_E270325F356A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma warning(disable : 4995)

// _WIN32_WINNT and WINVER (Windows 10) are set for every project in Directory.Build.targets.
// _CRT_SECURE_NO_DEPRECATE, _CRT_NON_CONFORMING_SWPRINTFS and VC_EXTRALEAN are set in the
// PreprocessorDefinitions of CP_Main.vcxproj and tests\AppTests\AppTests.vcxproj.

#include <afxwin.h>   // MFC core and standard components
#include <afxext.h>   // MFC extensions
#include <afxdisp.h>  // MFC Automation classes
#include <afxdtctl.h> // MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h> // MFC support for Windows Common Controls
#endif              // _AFX_NO_AFXCMN_SUPPORT
#include <afxole.h>

#include <imm.h>
#include <afxcontrolbars.h>
// TOM's FindText collides with the Win32 FindText macro (nothing calls the TOM one), and the
// remote-handle types are already declared by the Windows headers
#import "riched20.dll" raw_interfaces_only, raw_native_types, no_namespace, named_guids, exclude("UINT_PTR"), exclude("LONG_PTR"), exclude("wireHWND", "_RemotableHandle", "__MIDL_IWinTypes_0009"), rename("FindText", "TomFindText")

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <gdiplus.h>
#include <afxdlgs.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>
#endif // !defined(AFX_STDAFX_H__56F3D184_7208_47FE_AFE2_E270325F356A__INCLUDED_)
