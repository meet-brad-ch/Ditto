#include "stdafx.h"
#include "CP_Main.h" // the clip, frame and editor types in the order they need
#include "ClipCommands.h"
#include "AppState.h"
#include "AppWindows.h"
#include "RegisteredClipboardFormats.h"
#include "Clip_ImportExport.h"
#include "ClipboardFormatError.h"
#include "ErrorReport.h"
#include "FileDialogPath.h"
#include "Path.h"
#include "ShowTaskBarIcon.h"

CClipCommands::CClipCommands(CGetSetOptions& settings, CMultiLanguage& language, const CRegisteredClipboardFormats& clipboardFormats, CAppState& state, CAppWindows& windows, CClipEditThread& editThread, CClipContext& clipContext) :
	m_settings(settings),
	m_language(language),
	m_clipboardFormats(clipboardFormats),
	m_state(state),
	m_windows(windows),
	m_editThread(editThread),
	m_clipContext(clipContext)
{
}

bool CClipCommands::EditItems(CClipIDs& Ids, bool /*bShowError*/, bool forceTextEdit)
{
	bool ret = false;

	int lastFileCheckId = 1;

	for (int i = 0; i < min(Ids.GetCount(), 20); i++)
	{
		if (EditItem(Ids[i], forceTextEdit, lastFileCheckId))
		{
			ret = true;
		}
	}

	return ret;
}

bool CClipCommands::EditItem(int id, bool forceTextEdit, int& lastFileCheckId)
{
	CClip clip(m_clipContext);
	if (id >= 0 && clip.LoadFormats(id) == false)
	{
		CLogger::Log(CStringUtil::Format(_T("Failed to load formats for clipId: %d"), id));
		return false;
	}

	ClipEditTarget target{};
	if (!ChooseClipEditTarget(clip, id, forceTextEdit, target))
	{
		return false;
	}

	if (EditInInternalEditor(target, id))
	{
		return false;
	}

	CString savePath = MakeEditFilePath(id, target.extension, lastFileCheckId);
	if (savePath == _T(""))
	{
		CErrorReport::Show(CStringUtil::Format(_T("The new clip was not opened for editing: every NewClip_<n>.%s name in %s is taken"),
											   target.extension.GetString(), m_settings.GetPath(CGetSetOptions::PathEditClips).GetString()));
		return false;
	}

	m_editThread.WatchFile(savePath);

	// a file that could not be written is not opened in the editor (upstream opened it anyway)
	const bool written = target.imageFile
							 ? clip.WriteImageToFileOrReport(savePath, _T("edit"))
							 : clip.WriteTextToFile(savePath, target.unicodeFile, target.asciFile, target.rtfFile, (id == -1)) != FALSE;
	if (!written)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Clip id %d was not opened for editing: it could not be written to %s"), id, savePath.GetString()));
		return false;
	}

	return LaunchClipEditor(target.exePath, savePath, id);
}

bool CClipCommands::ChooseClipEditTarget(CClip& clip, int id, bool forceTextEdit, ClipEditTarget& target)
{
	if (forceTextEdit == false && clip.ContainsClipFormat(m_clipboardFormats.Rtf()))
	{
		target.extension = _T("rtf");
		target.rtfFile = true;
		target.exePath = m_settings.GetRTFEditorPath();
	}
	else if (clip.ContainsClipFormat(CF_UNICODETEXT))
	{
		target.extension = _T("txt");
		target.unicodeFile = true;
		target.exePath = m_settings.GetTextEditorPath();
	}
	else if (clip.ContainsClipFormat(CF_TEXT))
	{
		target.extension = _T("txt");
		target.asciFile = true;
		target.exePath = m_settings.GetTextEditorPath();
	}
	else if (id == -1)
	{
		target.extension = _T("txt");
		target.unicodeFile = true;
		target.exePath = m_settings.GetTextEditorPath();
	}
	else if (clip.ContainsClipFormat(m_clipboardFormats.Png()))
	{
		target.imageFile = true;
		target.extension = _T("png");
		target.exePath = m_settings.GetImageEditorPath();
	}
	else if (clip.ContainsClipFormat(CF_DIB))
	{
		target.imageFile = true;
		target.extension = _T("bmp");
		target.exePath = m_settings.GetImageEditorPath();
	}
	else
	{
		return false;
	}

	return true;
}

bool CClipCommands::EditInInternalEditor(const ClipEditTarget& target, int id)
{
	if ((target.unicodeFile || target.asciFile || target.rtfFile) && target.exePath == _T(""))
	{
		CLogger::Log(CStringUtil::Format(_T("Clip id %d is a text or rtf file without a specific editor set, using internal editor"), id));

		CClipIDs editIds{};
		editIds.Add(id);
		m_windows.MainFrame()->ShowEditWnd(editIds);
		return true;
	}

	return false;
}

CString CClipCommands::MakeEditFilePath(int id, const CString& extension, int& lastFileCheckId)
{
	CString startingFilePath = CStringUtil::Format(_T("%sEditClip_%d.%s"), m_settings.GetPath(CGetSetOptions::PathEditClips).GetString(), id, extension.GetString());

	if (id == -1)
	{
		startingFilePath = CStringUtil::Format(_T("%sNewClip_1.%s"), m_settings.GetPath(CGetSetOptions::PathEditClips).GetString(), extension.GetString());
	}

	CString savePath = startingFilePath;

	//for new files make a unique file name
	if (id < 0 &&
		CFileSystem::FileExists(startingFilePath))
	{
		savePath = _T("");

		for (int y = lastFileCheckId; y < 1000000; y++)
		{
			CString testFilePath = CStringUtil::Format(_T("%sNewClip_%d.%s"), m_settings.GetPath(CGetSetOptions::PathEditClips).GetString(), y, extension.GetString());

			if (CFileSystem::FileExists(testFilePath) == FALSE)
			{
				savePath = testFilePath;
				lastFileCheckId = y + 1;
				break;
			}
		}
	}

	return savePath;
}

bool CClipCommands::LaunchClipEditor(const CString& exePath, const CString& savePath, int id)
{
	SHELLEXECUTEINFO sei = { sizeof(sei) };
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.lpVerb = _T("open");

	if (exePath != _T(""))
	{
		sei.lpFile = exePath;
		sei.lpParameters = savePath;

		CLogger::Log(CStringUtil::Format(_T("Launching editor path: %s, file: %s"), exePath.GetString(), savePath.GetString()));
	}
	else
	{
		sei.lpFile = savePath;

		CLogger::Log(CStringUtil::Format(_T("Launching editor without specific exe path, file: %s"), savePath.GetString()));
	}

	sei.nShow = SW_NORMAL;

	if (ShellExecuteEx(&sei) == FALSE)
	{
		CLogger::Log(CStringUtil::Format(_T("ShellExecuteEx failed, not editing clipid: %d"), id));
		return false;
	}

	return true;
}

bool CClipCommands::ImportClips(HWND hWnd)
{
	CString filePath{};
	if (!AskImportFile(filePath))
	{
		return false;
	}

	ImportFile(hWnd, filePath);

	return true;
}

bool CClipCommands::AskImportFile(CString& filePath)
{
	OPENFILENAME FileName{};
	TCHAR szFileName[400]{};

	// the dialog reads the folder from the string itself: no copy into a fixed buffer (a longer
	// path overflowed the 400 characters before)
	const CString csInitialDir = m_settings.GetLastImportDir();

	FileName.lStructSize = sizeof(FileName);
	FileName.lpstrTitle = _T("Import Clips");
	FileName.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	FileName.nMaxFile = _countof(szFileName);
	FileName.lpstrFile = szFileName;
	FileName.lpstrInitialDir = csInitialDir.GetString();
	FileName.lpstrFilter = _T("Exported Ditto Clips (.dto)\0*.dto\0\0");
	FileName.lpstrDefExt = _T("dto");

	if (GetOpenFileName(&FileName) == 0)
	{
		return false;
	}

	filePath = CFileDialogPath::From(FileName);

	using namespace nsPath;
	CPath path(filePath);
	CString csPath(path.GetPath());
	m_settings.SetLastImportDir(csPath);

	return true;
}

void CClipCommands::ImportFile(HWND hWnd, const CString& filePath)
{
	try
	{
		CppSQLite3DB db{};
		db.open(filePath);

		CClip_ImportExport clip(m_clipContext);
		if (clip.ImportFromSqliteDB(db, true, false))
		{
			CShowTaskBarIcon show(m_windows, m_state);

			CString cs{};

			cs.Format(_T("%s %d "), m_language.GetString("Import_Successfully", "Successfully imported").GetString(), clip.m_importCount);
			if (clip.m_importCount == 1)
				cs += m_language.GetString("Clip", "clip");
			else
				cs += m_language.GetString("Clips", "clips");

			MessageBox(hWnd, cs, _T("Ditto"), MB_OK);
		}
		else
		{
			CShowTaskBarIcon show(m_windows, m_state);
			MessageBox(hWnd, m_language.GetString("Error_Importing", "Error importing exported clip"), _T("Ditto"), MB_OK);
		}
	}
	catch (CppSQLite3Exception& e)
	{
		ASSERT(FALSE);

		CString csError{};
		csError.Format(_T("%s - Exception - %d - %s"), m_language.GetString("Error_Parsing", "Error parsing exported clip").GetString(), e.errorCode(), e.errorMessage());
		MessageBox(hWnd, csError, _T("Ditto"), MB_OK);
	}
	catch (const DittoCore::ClipboardFormatError& error)
	{
		CString csError{};
		csError.Format(_T("%s - %s"), m_language.GetString("Error_Parsing", "Error parsing exported clip").GetString(), CString(error.what()).GetString());
		MessageBox(hWnd, csError, _T("Ditto"), MB_OK);
	}
}
