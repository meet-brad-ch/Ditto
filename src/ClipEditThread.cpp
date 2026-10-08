#include "stdafx.h"
#include "ClipEditThread.h"
#include "Options.h"
#include "Misc.h"
#include "Clip.h"
#include "CP_Main.h"
#include "ConvertRTFToText.h"
#include "..\Shared\TextConvert.h"

CClipEditThread::CClipEditThread(CGetSetOptions& settings, CClipContext& clipContext, CAppWindows& windows) :
	m_settings(settings),
	m_clipContext(clipContext),
	m_windows(windows)
{
	m_folderHandle = INVALID_HANDLE_VALUE;
	m_threadName = _T("ClipEditTrackingThread");
	m_waitTimeout = s_maxTimeout;
}

CClipEditThread::~CClipEditThread()
{
	Close();
}

void CClipEditThread::Close()
{
	Stop();
		
	if (m_folderHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_folderHandle);
		m_folderHandle = INVALID_HANDLE_VALUE;
	}

	RemoveEvent(EventFileChanged);
	m_overlapped.hEvent = INVALID_HANDLE_VALUE;	

	CString editClipFolder = m_settings.GetPath(CGetSetOptions::PathEditClips);
	CTempFileCleaner::DeleteFolderFiles(editClipFolder, TRUE, CTimeSpan(7, 0, 0, 0));
}

void CClipEditThread::StartWatchingFolderForChanges()
{
	CString editClipFolder = m_settings.GetPath(CGetSetOptions::PathEditClips);

	m_folderHandle = CreateFileW(editClipFolder, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
	
	m_overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	AddEvent(EventFileChanged, m_overlapped.hEvent);	

	RefreshWatch();		

	Start();
}

void CClipEditThread::WatchFile(CString filePath)
{
	ATL::CCritSecLock csLock(m_fileEditsLock.m_sect);

	nsPath::CPath path(filePath);

	//start the edit count at 0, the first edit notification is us saving the file, after that handle the file change
	m_fileEditStarts[path.GetName()] = CTime::GetCurrentTime();
}

void CClipEditThread::RefreshWatch()
{
	memset(m_fileChangeBuffer, 0, sizeof(m_fileChangeBuffer));

	DWORD bytesReturned = 0;
	ReadDirectoryChangesW(m_folderHandle, m_fileChangeBuffer, sizeof(m_fileChangeBuffer), FALSE, 
		FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE, 
		&bytesReturned, &m_overlapped, NULL);
}

void CClipEditThread::OnTimeOut(void* /*param*/)
{
	if (m_waitTimeout == s_maxTimeout)
	{
		CString editClipFolder = m_settings.GetPath(CGetSetOptions::PathEditClips);
		CTempFileCleaner::DeleteFolderFiles(editClipFolder, TRUE, CTimeSpan(7, 0, 0, 0));

		//cleanup up the list of edits that we started, after 7 days just keeps the list from growing too large, hopefull they don't have millions of edits in 7 days
		for (auto it = m_fileEditStarts.begin(); it != m_fileEditStarts.end();)
		{
			auto diff = CTime::GetCurrentTime() - it->second;
			if (diff.GetTotalSeconds() > (86400 * 7))
			{
				it = m_fileEditStarts.erase(it);
			}
			else
			{
				it++;
			}
		}		
	}
	else
	{
		for (auto const& toSave : m_filesToSave)
		{
			CString fileName = toSave.first;
			CString editClipFileName = _T("EditClip_");
			CString newClipFileName = _T("NewClip_");

			if (fileName.Find(editClipFileName, 0) == 0)
			{
				nsPath::CPath path(fileName);
				CString idString = path.GetTitle().Mid(editClipFileName.GetLength());

				int id = _wtoi(idString);
				if (id > 0)
				{
					SaveToClip(fileName, id);
				}
			}
			else if (fileName.Find(newClipFileName, 0) == 0)
			{
				SaveToClip(fileName, -1);
			}
		}

		m_filesToSave.clear();
		m_waitTimeout = s_maxTimeout;
	}
}

void CClipEditThread::OnEvent(int eventId, void* /*param*/)
{
	switch (eventId)
	{
		case EventFileChanged:
		{
			OnFileChanged();
			break;
		}
	}
}

void CClipEditThread::OnFileChanged()
{
	FILE_NOTIFY_INFORMATION* pNotify = m_fileChangeBuffer;
	int loopCount = 0;
	bool fileModified = false;

	while (true)
	{
		CString fileName(pNotify->FileName, pNotify->FileNameLength / sizeof(WCHAR));

		//we can't filter by the action, modify as ms word doesn't modify the file they replace it
		///so just look for anything that matches our file names

		if (IsChangeAction(pNotify->Action))
		{
			if (ShouldSaveChangedFile(fileName))
			{
				CLogger::Log(CStringUtil::Format(_T("%s file changed, adding to list to be saved back to Ditto"), fileName.GetString()));
				m_filesToSave[fileName] = true;
				fileModified = true;
			}
		}

		// NextEntryOffset is unsigned: 0 marks the last entry
		if (pNotify->NextEntryOffset == 0 || loopCount > 1000)
		{
			break;
		}

		pNotify = (FILE_NOTIFY_INFORMATION*)((BYTE*)pNotify + pNotify->NextEntryOffset);

		loopCount++;
	}

	RefreshWatch();

	if (fileModified)
	{
		m_waitTimeout = m_settings.m_clipEditSaveDelayAfterSaveSeconds * 1000;
	}
}

bool CClipEditThread::IsChangeAction(DWORD action)
{
	return action != FILE_ACTION_ADDED && action != FILE_ACTION_REMOVED;
}

bool CClipEditThread::ShouldSaveChangedFile(const CString& fileName)
{
	CString editClipFileName = _T("EditClip_");
	CString newClipFileName = _T("NewClip_");
	bool addToChanges = true;

	{
		ATL::CCritSecLock csLock(m_fileEditsLock.m_sect);
		auto exists = m_fileEditStarts.find(fileName);
		if (exists != m_fileEditStarts.end())
		{
			auto startEdit = m_fileEditStarts[fileName];
			auto diff = CTime::GetCurrentTime() - startEdit;
			if (diff.GetTotalSeconds() < m_settings.m_clipEditSaveDelayAfterLoadSeconds)
			{
				CLogger::Log(CStringUtil::Format(_T("%s has changed close to when we started editing the file, diff: %lld, limit: %d, not handling change"), fileName.GetString(), diff.GetTotalSeconds(), m_settings.m_clipEditSaveDelayAfterLoadSeconds));
				addToChanges = false;
			}
		}
		else //not in our list of files we initiated the change with
		{
			if (fileName.Find(newClipFileName, 0) == 0)
			{
				CLogger::Log(CStringUtil::Format(_T("New clip file changed: %s, this was not in Ditto list of files we initiated the change for, not handling change"), fileName.GetString()));
				addToChanges = false;
			}
		}
	}

	if (fileName.Find(editClipFileName, 0) == -1 && fileName.Find(newClipFileName, 0) == -1)
	{
		addToChanges = false;
		CLogger::Log(CStringUtil::Format(_T("File %s is not a Ditto file of format EditClip or NewClip, not handling change"), fileName.GetString()));
	}

	return addToChanges;
}

bool CClipEditThread::SaveToClip(CString filePath, int id)
{
	bool savedClip = false;

	CLogger::Log(CStringUtil::Format(_T("ClipFile: %s, ClipId: %d, has changed saving back to Ditto"), filePath.GetString(), id));

	id = ResolveNewClipId(filePath, id);

	CClip clip(m_clipContext);
	if (id >= 0)
	{
		if (clip.LoadMainTable(id) == FALSE)
		{
			CLogger::Log(CStringUtil::Format(_T("Error loading clip id: %d, not saving"), id));
			return false;
		}

		// without its formats the save would keep the old Data rows next to the new ones
		if (clip.LoadFormats(id) == false)
		{
			CLogger::Log(CStringUtil::Format(_T("Error loading the formats of clip id: %d, not saving"), id));
			return false;
		}
	}

	EditedClipData data{};

	CString editClipFolder = m_settings.GetPath(CGetSetOptions::PathEditClips);
	CString fullFilePath = editClipFolder + filePath;

	nsPath::CPath path(filePath);
	auto extenstion = path.GetExtension().MakeLower();

	if (ReadEditedFile(fullFilePath, extenstion, id, data) == false)
	{
		return false;
	}

	if (IsEmptyNewClip(id, data))
	{
		CLogger::Log(CStringUtil::Format(_T("Not saving new clip that is empty, no text or image bytes, path: %s, clip id: %d, not saving"), fullFilePath.GetString(), id));
		return false;
	}

	BOOL modifyDescription = m_settings.GetUpdateDescWhenSavingClip();

	// a failed save (shown by SaveFormats) is not refreshed as saved
	if (SaveEditedFormats(clip, extenstion, data, modifyDescription) == false)
	{
		return false;
	}

	RefreshEditedClip(filePath, id, clip);

	savedClip = true;

	return savedClip;
}

int CClipEditThread::ResolveNewClipId(const CString& filePath, int id)
{
	if (id < 0)
	{
		auto exists = m_newClipIds.find(filePath);
		if (exists != m_newClipIds.end())
		{
			id = m_newClipIds[filePath];
		}
	}
	return id;
}

bool CClipEditThread::ReadEditedFile(const CString& fullFilePath, const CString& extenstion, int id, EditedClipData& data)
{
	if (extenstion == _T("png") || extenstion == _T("bmp"))
	{
		if (ReadImageFile(fullFilePath, data.cf_dibBytes, data.pngBytes) == false)
		{
			CLogger::Log(CStringUtil::Format(_T("Error reading image file %s, clip id: %d, not saving"), fullFilePath.GetString(), id));
			return false;
		}
	}
	else if (ReadFile(fullFilePath, data.unicode, data.unicodeText, data.utf8Text) == false)
	{
		CLogger::Log(CStringUtil::Format(_T("Error reading text file %s, clip id: %d, not saving"), fullFilePath.GetString(), id));
		return false;
	}
	return true;
}

bool CClipEditThread::IsEmptyNewClip(int id, const EditedClipData& data)
{
	return id < 0 &&
		data.unicodeText == _T("") &&
		data.utf8Text == "" &&
		data.cf_dibBytes.size() <= 0 &&
		data.pngBytes.size() <= 0;
}

bool CClipEditThread::SaveEditedFormats(CClip& clip, const CString& extenstion, EditedClipData& data, BOOL modifyDescription)
{
	if (extenstion == _T("bmp") || extenstion == _T("png"))
	{
		return clip.SaveFormats(nullptr, nullptr, nullptr, modifyDescription, &data.cf_dibBytes, &data.pngBytes) != FALSE;
	}
	else if (extenstion == _T("txt"))
	{
		if (data.unicode == false)
		{
			data.unicodeText = CTextConvert::Utf8ToUnicode(data.utf8Text);
		}
		return clip.SaveFormats(&data.unicodeText, nullptr, nullptr, modifyDescription) != FALSE;
	}
	else if (extenstion == _T("rtf"))
	{
		if (GetTextFromRTF(data.utf8Text, data.unicodeText))
		{
			return clip.SaveFormats(&data.unicodeText, nullptr, &data.utf8Text, modifyDescription) != FALSE;
		}
		return clip.SaveFormats(nullptr, nullptr, &data.utf8Text, modifyDescription) != FALSE;
	}
	return true;
}

void CClipEditThread::RefreshEditedClip(const CString& filePath, int id, const CClip& clip)
{
	//refresh the clip in the UI
	if (id == -1)
	{
		m_newClipIds[filePath] = clip.m_id;
		m_windows.RefreshView(CopyReasonEnum::COPY_TO_UNKOWN);
	}
	else if (id > 0)
	{
		m_windows.RefreshClipInUI(id, CClipRefreshFlags::ClipDescription);
	}
}

bool CClipEditThread::ReadFile(CString filePath, bool &unicode, CString &unicodeText, CStringA &utf8Text)
{
	CFile file;
	CFileException ex;
	if (!file.Open(filePath, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone, &ex))
	{
		CString error;
		ex.GetErrorMessage(error.GetBufferSetLength(200), 200);
		error.ReleaseBuffer();
		CLogger::Write(CStringUtil::Format(_T("LoadFormatsFromFile - Error opening file: %s, Error: %s\r\n"), filePath.GetString(), error.GetString()));
		return false;
	}

	try
	{
		if (file.GetLength() >= 2)
		{
			// initialized and checked: upstream compared an unread header when the read came short
			wchar_t header{};
			if (file.Read(&header, sizeof(wchar_t)) == sizeof(wchar_t) && header == 0xFEFF)
			{
				unicode = true;
			}
			else
			{
				file.SeekToBegin();
			}
		}

		UINT expected{};
		UINT read{};
		if (unicode)
		{
			const UINT bufferSize = (UINT)((file.GetLength() - 2) / 2);
			expected = bufferSize * 2;
			read = file.Read(unicodeText.GetBufferSetLength(bufferSize), expected);
			unicodeText.ReleaseBuffer();
		}
		else
		{
			const UINT bufferSize = (UINT)(file.GetLength());
			expected = bufferSize;
			read = file.Read(utf8Text.GetBufferSetLength(bufferSize), bufferSize);
			utf8Text.ReleaseBuffer();
		}

		if (read != expected)
		{
			CLogger::Log(CStringUtil::Format(_T("LoadFormatsFromFile - read %u of %u bytes of %s, not saving"), read, expected, filePath.GetString()));
			return false;
		}
	}
	catch (CFileException* e)
	{
		TCHAR cause[255]{};
		e->GetErrorMessage(cause, _countof(cause));
		e->Delete();
		CLogger::Log(CStringUtil::Format(_T("LoadFormatsFromFile - Error reading file: %s, Error: %s"), filePath.GetString(), cause));
		return false;
	}

	return true;
}

std::vector<BYTE> CClipEditThread::CImageToPNGBytes(const CImage& image, REFGUID guidFileType)
{
	IStream* pStream = nullptr;
	HRESULT hr = CreateStreamOnHGlobal(nullptr, TRUE, &pStream);
	if (FAILED(hr)) {
		return {};
	}

	ULARGE_INTEGER ulSize;

	hr = image.Save(pStream, guidFileType);
	if (FAILED(hr)) {
		pStream->Release();
		return {};
	}

	LARGE_INTEGER liZero = { 0 };
	hr = pStream->Seek(liZero, STREAM_SEEK_SET, nullptr);
	if (FAILED(hr)) {
		pStream->Release();
		return {};
	}

	hr = pStream->Seek({ 0 }, STREAM_SEEK_END, &ulSize);
	if (FAILED(hr)) {
		pStream->Release();
		return {};
	}

	std::vector<BYTE> pngBytes((UINT)ulSize.QuadPart);

	hr = pStream->Seek(liZero, STREAM_SEEK_SET, nullptr);
	if (FAILED(hr)) {
		pStream->Release();
		return {};
	}

	hr = pStream->Read(pngBytes.data(), (UINT)ulSize.QuadPart, nullptr);
	pStream->Release();

	if (FAILED(hr)) {
		return {};
	}
	return pngBytes;
}

bool CClipEditThread::ReadImageFile(CString path, std::vector<BYTE> &/*cf_dibBytes*/, std::vector<BYTE> & pngBytes)
{
	CImage image;
	HRESULT hr = image.Load(path);
	if (FAILED(hr))
	{
		CLogger::Log(CStringUtil::Format(_T("Failed to load image, %s"), path.GetString()));
		return false;
	}

	pngBytes = CImageToPNGBytes(image, Gdiplus::ImageFormatPNG);	

	return true;
}

BOOL CClipEditThread::GetTextFromRTF(CStringA rtf, CString &unicodeText)
{
	CConvertRTFToText cc;
	if (cc.Create())
	{
		unicodeText = cc.GetTextFromRTF(rtf);
		cc.DestroyWindow();

		if (rtf != "" && unicodeText == "")
		{
			CLogger::Write(CStringUtil::Format(_T("Failed to convert rtf to text, rtf text is not empty but text is empty")));
		}

		return true;
	}
	else
	{
		CLogger::Write(CStringUtil::Format(_T("Failed to create rtf to text window")));
	}	

	return false;
}