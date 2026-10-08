#pragma once

#include "EventThread.h"
#include "Path.h"

class CClip;
class CGetSetOptions;
namespace ATL { class CImage; }

class CClipEditThread : public CEventThread
{
public:
	/**
	 * @brief Creates the (not yet started) edit-file watcher.
	 * @param settings The application's settings (edit folder, save delays); must outlive this object.
	 */
	explicit CClipEditThread(CGetSetOptions& settings);
	virtual ~CClipEditThread();
		
	void Close();
	void StartWatchingFolderForChanges();

	void WatchFile(CString filePath);

private:
	void OnEvent(int eventId, void* param) override;
	void OnFileChanged();
	void OnTimeOut(void* param) override;
	bool ReadFile(CString filePath, bool& unicode, CString& unicodeText, CStringA& utf8Text);
	bool ReadImageFile(CString path, std::vector<BYTE>& cf_dibBytes, std::vector<BYTE>& pngBytes);
	/**
	 * @brief Saves an image into an in-memory stream and returns the bytes.
	 * @param image the loaded image.
	 * @param guidFileType the GDI+ image format to save as (e.g. Gdiplus::ImageFormatPNG).
	 * @return the encoded bytes; empty if a stream step failed.
	 */
	static std::vector<BYTE> CImageToPNGBytes(const ATL::CImage& image, REFGUID guidFileType);
	BOOL GetTextFromRTF(CStringA rtf, CString& unicodeText);
	void RefreshWatch();
	bool SaveToClip(CString filePath, int id);

	/** @brief The event ids of the thread (CEventThread::AddEvent / OnEvent). */
	enum : int
	{
		/** @brief The watched folder changed. */
		EventFileChanged = 1,
	};
	/** @brief The wait timeout while no edited file waits to be saved (CEventThread::m_waitTimeout). */
	static constexpr int s_maxTimeout{86400};

	/** @brief The content read back from an edited clip file. */
	struct EditedClipData
	{
		/** @brief Text of a UTF-16 (BOM) text file; also the converted text of a txt/rtf file. */
		CString unicodeText;
		/** @brief Bytes of a non-UTF-16 text file (UTF-8 text or RTF). */
		CStringA utf8Text;
		/** @brief true if the text file started with a UTF-16 BOM. */
		bool unicode{};
		/** @brief CF_DIB bytes of an image file (not filled today). */
		std::vector<BYTE> cf_dibBytes;
		/** @brief PNG bytes of an image file. */
		std::vector<BYTE> pngBytes;
	};

	/**
	 * @brief Tells whether a directory change notification action can be a file save.
	 * @param action the FILE_NOTIFY_INFORMATION action.
	 * @return false for FILE_ACTION_ADDED and FILE_ACTION_REMOVED, true otherwise.
	 */
	static bool IsChangeAction(DWORD action);
	/**
	 * @brief OnFileChanged's check of one changed file: a Ditto EditClip/NewClip file that was not
	 * just written by Ditto itself (logs the reason when it is skipped).
	 * @param fileName the changed file name (no folder).
	 * @return true if the file is to be saved back to its clip.
	 */
	bool ShouldSaveChangedFile(const CString& fileName);
	/**
	 * @brief SaveToClip's step: maps a new clip file (id < 0) to the clip id it was saved to before.
	 * @param filePath the edited file name.
	 * @param id the clip id from the file name, -1 for a new clip file.
	 * @return the clip id to update, unchanged if the file was not saved before.
	 */
	int ResolveNewClipId(const CString& filePath, int id);
	/**
	 * @brief SaveToClip's step: reads an image (png/bmp) or text file (logs a read error).
	 * @param fullFilePath the full path of the edited file.
	 * @param extenstion the lower-case file extension.
	 * @param id the clip id (for the log).
	 * @param data receives the file content.
	 * @return false if the file could not be read.
	 */
	bool ReadEditedFile(const CString& fullFilePath, const CString& extenstion, int id, EditedClipData& data);
	/**
	 * @brief Tells whether a new clip file (id < 0) has no text and no image bytes.
	 * @param id the clip id.
	 * @param data the file content.
	 * @return true for an empty new clip, which is not saved.
	 */
	static bool IsEmptyNewClip(int id, const EditedClipData& data);
	/**
	 * @brief SaveToClip's step: saves the file content as the clip formats for its extension.
	 * @param clip the clip to save into.
	 * @param extenstion the lower-case file extension (bmp/png, txt or rtf; others save nothing).
	 * @param data the file content (the converted text is written to data.unicodeText).
	 * @param modifyDescription TRUE to update the clip description.
	 */
	void SaveEditedFormats(CClip& clip, const CString& extenstion, EditedClipData& data, BOOL modifyDescription);
	/**
	 * @brief SaveToClip's step: refreshes the saved clip in the UI and remembers a new clip's id.
	 * @param filePath the edited file name.
	 * @param id the clip id, -1 for a new clip.
	 * @param clip the saved clip.
	 */
	void RefreshEditedClip(const CString& filePath, int id, const CClip& clip);

	/// The application's settings (not owned).
	CGetSetOptions& m_settings;
	HANDLE m_folderHandle;
	FILE_NOTIFY_INFORMATION m_fileChangeBuffer[10000]{};
	OVERLAPPED m_overlapped{};
	std::map<CString, bool> m_filesToSave;
	std::map<CString, int> m_newClipIds;

	CCriticalSection m_fileEditsLock;
	std::map<CString, CTime> m_fileEditStarts;
};

