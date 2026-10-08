#include "stdafx.h"
#include "ClipCompare.h"
#include "Misc.h"
#include "Options.h"

CClipCompare::CClipCompare(CGetSetOptions& settings) :
	m_settings(settings)
{
}


CClipCompare::~CClipCompare(void)
{
}


void CClipCompare::Compare(int leftId, int rightId)
{
	CClip leftClip(m_settings);
	if(leftClip.LoadFormats(leftId, true))
	{
		CClip rightClip(m_settings);
		if(rightClip.LoadFormats(rightId, true))
		{
			CompareClips(leftId, leftClip, rightId, rightClip);
		}
		else
		{
			CLogger::Log(CStringUtil::Format(_T("CClipCompare::Compare, Failed to load RIGHT clip formats Id: %d"), rightId));
		}
	}
	else
	{
		CLogger::Log(CStringUtil::Format(_T("CClipCompare::Compare, Failed to load LEFT clip formats Id: %d"), leftId));
	}
}

CClipCompare::CompareFormats CClipCompare::GetCompareFormats(CClip& leftClip, CClip& rightClip)
{
	CompareFormats formats{};

	if (m_settings.GetPreferUtf8ForCompare() == FALSE)
	{
		CLogger::Log(CStringUtil::Format(_T("CClipCompare::Compare, option is set to not use utf8")));
		formats.saveUtf8 = false;
	}

	if(leftClip.GetUnicodeTextFormat() == _T("") || rightClip.GetUnicodeTextFormat() == _T(""))
	{
		formats.saveW = false;
		formats.saveUtf8 = false;
	}

	if(leftClip.GetCFTextTextFormat() == "" || rightClip.GetCFTextTextFormat() == "")
	{
		formats.saveA = false;
	}

	return formats;
}

void CClipCompare::CompareClips(int leftId, CClip& leftClip, int rightId, CClip& rightClip)
{
	const CompareFormats formats = GetCompareFormats(leftClip, rightClip);

	if(formats.saveW || formats.saveA || formats.saveUtf8)
	{
		LaunchCompare(leftId, leftClip, rightId, rightClip, formats);
	}
	else
	{
		CLogger::Log(CStringUtil::Format(_T("CClipCompare::Compare, did not find valid text for both passed in clips")));
	}
}

void CClipCompare::LaunchCompare(int leftId, CClip& leftClip, int rightId, CClip& rightClip, const CompareFormats& formats)
{
	CString leftFile = SaveToFile(leftId, &leftClip, formats.saveW, formats.saveA, formats.saveUtf8);
	CString rightFile = SaveToFile(rightId, &rightClip, formats.saveW, formats.saveA, formats.saveUtf8);

	CString params = _T("");
	CString path = GetComparePath(params);

	if(path != _T(""))
	{
		SHELLEXECUTEINFO sei = { sizeof(sei) };
		sei.lpFile = path;
		CString csParam;
		csParam.Format(_T("%s\"%s\" \"%s\""), params.GetString(), leftFile.GetString(), rightFile.GetString());
		sei.lpParameters = csParam;
		sei.nShow = SW_NORMAL;

		CLogger::Log(CStringUtil::Format(_T("Comparing two clips, left Id %d, right Id %d, Path: %s %s"), leftId, rightId, path.GetString(), csParam.GetString()));

		if (!ShellExecuteEx(&sei))
		{
		}
	}
	else
	{
		CLogger::Log(CStringUtil::Format(_T("CClipCompare::Compare, No Valid compare apps, not doing compare")));

		MessageBox(NULL, _T("No compare application found. Install WinMerge or set \"Diff application path\" in Advanced options."), _T("Ditto"), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
	}
}

CString CClipCompare::GetComparePath(CString &params)
{
	CString path = m_settings.GetDiffApp().MakeLower();

	if(path != _T(""))
	{
		SetConfiguredAppParams(path, params);
		return path;
	}

	for (const CompareApp& app : m_compareApps)
	{
		path = app.path;
		if (app.resolvePath)
		{
			path = m_settings.ResolvePath(app.path);
		}

		if (CFileSystem::FileExists(path))
		{
			if (app.params != nullptr)
			{
				params = app.params;
			}
			return path;
		}
	}

	return _T("");
}

void CClipCompare::SetConfiguredAppParams(const CString& path, CString& params)
{
	if (path.Find(_T("totalcmd.exe")) != -1 ||
		path.Find(_T("totalcmd64.exe")) != -1)
	{
		params = _T(" /S=C ");
	}
	else if (path.Find(_T("code.exe")) != -1)
	{
		params = _T(" --diff ");
	}
}

CString CClipCompare::SaveToFile(int id, CClip *pClip, bool saveW, bool saveA, bool saveUtf8)
{
	CString path;
	CString pathCompare = m_settings.GetPath(CGetSetOptions::PathClipDiff);
	CString cs;
	cs.Format(_T("%sditto_compare_%d.txt"), pathCompare.GetString(), id);

	if(CFileSystem::FileExists(cs))
	{
		for(int i = 0; i < 1000; i++)
		{			
			cs.Format(_T("%sditto_compare_%d.txt"), pathCompare.GetString(), id);
			if(CFileSystem::FileExists(cs))
			{
				path = cs;
				break;
			}
		}
	}
	else
	{
		path = cs;
	}

	if(path != _T("") && 
		pClip != NULL)
	{
		pClip->WriteTextToFile(path, saveW, saveA, FALSE, FALSE, saveUtf8);
	}

	return path;
}