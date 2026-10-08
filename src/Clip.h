// Clip.h: classes for manage clips
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROCESSCOPY_H__185CBB6F_4B63_4397_8FF9_E18D777DA506__INCLUDED_)
#define AFX_PROCESSCOPY_H__185CBB6F_4B63_4397_8FF9_E18D777DA506__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <afxole.h>
#include <afxtempl.h>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include "Crc32.h"
#include "ClipRepository.h"
#include "ClipSavePolicy.h"
#include "RegExFilterHelper.h"
#include "..\Shared\IClip.h"
#include "Misc.h"

class CClip;
class CClipContext;
class CCopyThread;
class CGetSetOptions;

typedef CArray<CLIPFORMAT, CLIPFORMAT> CClipTypes;

/*----------------------------------------------------------------------------*\
	COleDataObjectEx
\*----------------------------------------------------------------------------*/
class COleDataObjectEx : public COleDataObject
{
public:
	// creates global from IStream if necessary
	HGLOBAL GetGlobalData(CLIPFORMAT cfFormat, LPFORMATETC lpFormatEtc = NULL);
	/**
	 * @brief The formats on the clipboard (EnumClipboardFormats, without CF_MAX).
	 * @param clipboardOwner The window that opens the clipboard (the main window).
	 * @return The formats; empty when the clipboard cannot be opened.
	 */
	std::shared_ptr<CClipTypes> GetAvailableTypes(HWND clipboardOwner);

private:
	// Copies a whole stream into a new global block; null when the stream is empty, over 4 GB, or
	// cannot be read completely
	static HGLOBAL StreamToGlobal(IStream* stream);
};

/*----------------------------------------------------------------------------*\
	CClipFormat - holds the data of one clip format.
\*----------------------------------------------------------------------------*/
class CClipFormat : public IClipFormat
{
public:
	CLIPFORMAT m_cfType;
    HGLOBAL m_hgData;
	bool m_autoDeleteData;
	int m_dataId{-1};
	int m_parentId;

	CClipFormat(CLIPFORMAT cfType = 0, HGLOBAL hgData = 0, int parentId = -1);
	~CClipFormat();

	void Clear();
	virtual void Free();

	virtual CLIPFORMAT Type() { return m_cfType; }
	virtual HGLOBAL Data() { return m_hgData; }
	virtual void Type(CLIPFORMAT type) { m_cfType = type; }
	virtual void Data(HGLOBAL data) { m_hgData = data; }
	virtual void AutoDeleteData(bool autoDeleteData) { m_autoDeleteData = autoDeleteData; }
	virtual bool AutoDeleteData()	{ return m_autoDeleteData; }

	// The format's 8-bit text up to the first null or the end of the block; empty without data
	CStringA GetAsCStringA();

	// The format's UTF-16 text up to the first null or the end of the block; empty without data
	CString GetAsCString();

	/**
	 * @brief Decodes a PNG or DIB format into a GDI+ bitmap.
	 * @param pngFormat The registered "PNG" format (CRegisteredClipboardFormats::Png()).
	 * @return The bitmap, owned by the caller; null for other formats or unreadable data.
	 */
	std::unique_ptr<Gdiplus::Bitmap> LoadGdiplusBitmap(CLIPFORMAT pngFormat);

	/**
	 * @brief IClipFormat (the add-in interface): LoadGdiplusBitmap for an add-in.
	 *
	 * The interface has no parameter for the services, so the "PNG" id is looked up by name
	 * (CClipboardFormats::GetFormatID: RegisterClipboardFormat returns the id that is already
	 * registered for the name, the one CRegisteredClipboardFormats::Png() holds).
	 * @return The bitmap, owned by the add-in caller (the interface's raw-pointer ABI); null as for LoadGdiplusBitmap.
	 */
	virtual Gdiplus::Bitmap* CreateGdiplusBitmap() override;
};

/*----------------------------------------------------------------------------*\
	CClipFormats - holds an array of CClipFormat
\*----------------------------------------------------------------------------*/
class CClipFormats : public CArray<CClipFormat,CClipFormat&>, public IClipFormats
{
public:
	// returns a pointer to the CClipFormat in this array which matches the given type
	//  or NULL if that type doesn't exist in this array.
	CClipFormat* FindFormat(UINT cfType); 

	virtual int Size() { return (int)this->GetCount(); }
	virtual IClipFormat *GetAt(int nPos) { return &this->ElementAt(nPos); }
	virtual void DeleteAt(int nPos) { this->RemoveAt(nPos); }
	virtual void DeleteAll() { this->RemoveAll(); }
	virtual INT_PTR AddNew(CLIPFORMAT type, HGLOBAL data) {CClipFormat ft(type, data, -1); ft.m_autoDeleteData = false; return this->Add(ft); }
	virtual IClipFormat *FindFormatEx(CLIPFORMAT type)	{ return FindFormat((UINT)type); }
	virtual bool RemoveFormat(CLIPFORMAT type);
};


/*----------------------------------------------------------------------------*\
	CClip - holds multiple CClipFormats and clip statistics
	- provides static functions for manipulating a Clip as a single unit.
\*----------------------------------------------------------------------------*/
class CClip : public IClip
{
public:
	/**
	 * @brief Creates an empty clip that takes its save settings from the settings when they are
	 *        first needed.
	 * @param context The services the clip works with (CAppServices::ClipContext()): the settings,
	 *        the clip saved last (the duplicate check reads it, a save records this clip there), the
	 *        database, the registered formats and the main window; must outlive this clip.
	 */
	explicit CClip(CClipContext& context);
	~CClip();
	// Copies the clip's data; the save settings and the last-added record stay this clip's own
	const CClip& operator=(const CClip &clip);

	/** @brief The sticky order of a clip that is not sticky (stored in the database's sticky columns). */
	static constexpr int InvalidSticky = -(2147483647);

	int m_id;
	CClipFormats m_Formats;
	CTime m_Time;
	CString m_Desc;
	ULONG m_lTotalCopySize{};
	int m_parentId;
	int m_dontAutoDelete;
	int m_shortCut;
	BOOL m_bIsGroup;
	DWORD m_CRC;
	CString m_csQuickPaste;
	int m_param1;
	double m_clipOrder;
	double m_clipGroupOrder;
	double m_stickyClipOrder;
	double m_stickyClipGroupOrder;
	BOOL m_globalShortCut;
	CTime m_lastPasteDate;
	int m_moveToGroupShortCut;
	BOOL m_globalMoveToGroupShortCut;
	CopyReasonEnum::CopyReason m_copyReason;

	virtual CString Description() { return m_Desc; }
	virtual void Description(CString csValue) { m_Desc = csValue; }
	virtual CTime PasteTime() { return m_Time; }
	virtual int ID() { return m_id; }
	virtual int Parent() { return m_parentId; }
	virtual void Parent(int nParent) { m_parentId = nParent; }
	virtual int DontAutoDelete() { return m_dontAutoDelete; }
	virtual void DontAutoDelete(int Dont) { m_dontAutoDelete = Dont; }
	virtual CString QuickPaste() { return m_csQuickPaste; }
	virtual void QuickPaste(CString csValue) { m_csQuickPaste = csValue; }

	virtual void SetSaveToDbSticky(AddToDbStickyEnum::AddToDbSticky option) { m_addToDbStickyEnum = option; }

	virtual IClipFormats *Clips() { return (IClipFormats*)&m_Formats; }

	void Clear();
	void EmptyFormats();
	bool AddFormat(CLIPFORMAT cfType, void* pData, SIZE_T nLen, bool setDesc = false);
	// regexFilters: the text filters that keep a copy out of the history
	int LoadFromClipboard(CClipTypes* pClipTypes, CRegExFilterHelper& regexFilters, bool checkClipboardIgnore = true, CString activeApp = _T(""));
	bool SetDescFromText(HGLOBAL hgData, bool unicode);
	bool SetDescFromType();
	bool AddToDB(bool bCheckForDuplicates = true);
	bool ModifyMainTable();
	bool ModifyDescription();
	/**
	 * @brief Gives the clip the order above the newest clip of the main list.
	 * @return False when the order could not be read (shown to the user); the order is unchanged.
	 */
	bool MakeLatestOrder();
	/**
	 * @brief Gives a clip in a group the order above the newest clip of the group (nothing for a
	 *        clip outside groups).
	 * @return False when the order could not be read (shown); the order is unchanged.
	 */
	bool MakeLatestGroupOrder();
	/**
	 * @brief Gives the clip the order below the oldest clip of the main list.
	 * @return False when the order could not be read (shown); the order is unchanged.
	 */
	bool MakeLastOrder();
	/**
	 * @brief Gives a clip in a group the order below the oldest clip of the group (nothing for a
	 *        clip outside groups).
	 * @return False when the order could not be read (shown); the order is unchanged.
	 */
	bool MakeLastGroupOrder();
	/**
	 * @brief Makes the clip the top sticky clip of the main list (parentId < 0) or of a group.
	 * @param parentId The group; negative for the main list.
	 * @return False when the order could not be read (shown); the order is unchanged.
	 */
	bool MakeStickyTop(int parentId);
	/**
	 * @brief Makes the clip the last sticky clip of the main list (parentId < 0) or of a group.
	 * @param parentId The group; negative for the main list.
	 * @return False when the order could not be read (shown); the order is unchanged.
	 */
	bool MakeStickyLast(int parentId);
	bool RemoveStickySetting(int parentId);
	BOOL LoadMainTable(int id);
	DWORD GenerateCRC();
	/**
	 * @brief Moves the clip one place up in the main list (parentId < 0) or a group.
	 * @param parentId The group; negative for the main list.
	 * @return False when the neighbouring orders could not be read (shown); the order is unchanged.
	 */
	bool MoveUp(int parentId);
	/**
	 * @brief Moves the clip one place down in the main list (parentId < 0) or a group.
	 * @param parentId The group; negative for the main list.
	 * @return False when the neighbouring orders could not be read (shown); the order is unchanged.
	 */
	bool MoveDown(int parentId);
	bool SaveFromEditWnd(BOOL bUpdateDesc);

	CStringW GetUnicodeTextFormat();
	CStringA GetCFTextTextFormat();
	CStringA GetRTFTextFormat();

	BOOL ContainsClipFormat(CLIPFORMAT clipFormat);

	BOOL WriteTextToFile(CString path, BOOL unicode, BOOL asci, BOOL rtf, BOOL forceUnicode = FALSE, BOOL utf8 = FALSE);
	BOOL WriteImageToFile(CString path);
	// Boundary for the user operations that save an image (edit, export, save): a malformed
	// image is reported with the operation's verb ("edit", "export", "save") and false returned.
	bool WriteImageToFileOrReport(const CString& path, const CString& operation);

	BOOL SaveFormats(CString* unicode, CStringA* asci, CStringA* rtf, BOOL updateDescription, std::vector<BYTE>* cf_dibBytes = nullptr, std::vector<BYTE>* pngBytes = nullptr);

	// Allocates a Global containing the requested Clip's Format Data (context: the database)
	static HGLOBAL LoadFormat(CClipContext& context, int id, UINT cfType);
	// Fills "formats" with the Data of all Formats in the db for the given Clip ID
	bool LoadFormats(int id, bool bOnlyLoad_CF_TEXT = false, bool includeRichTextForTextOnly = false, int dataId = -1);
	// Fills "types" with all Types in the db for the given Clip ID (context: the database)
	static void LoadTypes(CClipContext& context, int id, CClipTypes& types);

	// The order above the newest clip of the main list or a group (context: the database); the
	// order readers below throw CppSQLite3Exception when the query fails
	static double GetNewOrder(CClipContext& context, int parentId, int clipId);
	double GetNewLastOrder(int parentId, int clipId);
	// The sticky order above the top sticky clip (context: the database)
	static double GetNewTopSticky(CClipContext& context, int parentId, int clipId);
	// The sticky order below the last sticky clip (context: the database)
	static double GetNewLastSticky(CClipContext& context, int parentId, int clipId);
	// The id of the top sticky clip; -1 for none (context: the database); throws CppSQLite3Exception
	static int GetExistingTopStickyClipId(CClipContext& context, int parentId);
	// Clears a saved clip's sticky setting in the database (context: the database)
	static bool RemoveStickySetting(CClipContext& context, int clipId, int parentId);

	bool AddFileDataToData(CString &errorMessage);

	std::unique_ptr<Gdiplus::Bitmap> CreateGdiplusBitmap();
	
protected:
	// Adds the Main row and the Data rows, and clears another clip's top-sticky setting, in one
	// transaction; returns false (rolled back) when a step fails
	bool AddRowsInTransaction(int removeStickySettingClipId);
	// The database part of SaveFormats: deletes the replaced formats and writes the clip, in one
	// transaction; returns false (rolled back) when a step fails
	bool SaveFormatsInTransaction(const ARRAY& deletedData, BOOL updateDescription);
	// Adds a new clip's Main row, or updates an existing clip's description when asked
	bool SaveMainRow(BOOL updateDescription);
	bool AddToMainTable();
	bool AddToDataTable();
	int FindDuplicate();

	AddToDbStickyEnum::AddToDbSticky m_addToDbStickyEnum;

	/**
	 * @brief The services this clip works with (for the derived clips).
	 * @return The context given to the constructor (not owned).
	 */
	CClipContext& Context() const { return m_context; }

private:
	// AddToDB's duplicate step: when a saved clip has this clip's CRC, moves it to the top of its
	// lists, takes its id and returns true; false when there is no duplicate
	bool MoveDuplicateToTop();

	// Where a clip's position lives for one list: the order member that MoveUp/MoveDown change,
	// its column, and whether the clip is sticky there
	struct OrderSlot
	{
		CClipRepository::OrderColumn column{};
		bool sticky{};
		std::optional<int> parentId{};
		double* order{};
	};

	// The order slot of this clip in the main list (parentId < 0) or a group
	OrderSlot SlotFor(int parentId);
	// Moves the clip one place up or down in its list (midpoint of the two neighbours); false
	// (shown) when the neighbours could not be read
	bool Move(int parentId, bool up);
	/**
	 * @brief Runs a step that reads clip orders, as the public order changes do.
	 * @param step The step; it may throw CppSQLite3Exception.
	 * @return False when the step threw (the error is shown to the user).
	 */
	bool TryOrderStep(const std::function<void()>& step);
	/**
	 * @brief Sets the newest main order and, for a clip in a group, the newest group order.
	 * @throws CppSQLite3Exception When an order cannot be read (for callers inside a database boundary).
	 */
	void SetLatestOrders();
	// The highest or lowest order of a column in the main list or a group; nullopt when empty;
	// throws CppSQLite3Exception when the query fails
	static std::optional<double> EdgeOrder(CClipContext& context, CClipRepository::OrderColumn column, bool sticky, int parentId, bool highest);
	// The repository's parent filter: the group for parentId > -1, all clips otherwise
	static std::optional<int> ParentFilter(int parentId);
	// The repository over the context's database
	static CClipRepository Repository(CClipContext& context);
	// The save settings: the options' (read once, when first needed)
	const DittoCore::ClipSavePolicy& SavePolicy();
	std::optional<DittoCore::ClipSavePolicy> m_savePolicy{};
	/// The services this clip works with (not owned): SavePolicy reads the save settings,
	/// FindDuplicate reads the clip saved last and AddToMainTable records this clip there.
	CClipContext& m_context;
	// This clip's Main row, and back
	ClipRecord ToRecord() const;
	void FromRecord(const ClipRecord& record);

	// A file read for "Ditto File Data": its UTF-8 path, the MD5 of its contents, the contents
	struct CopiedFile
	{
		std::string path{};
		std::string md5{};
		std::vector<std::byte> contents{};
	};

	// Reads a file for AddFileDataToData; appends the reason to errorMessage and returns false
	// when it cannot be opened or read, or is not smaller than maxSize
	static bool ReadFileContents(const CString& path, ULONGLONG maxSize, CopiedFile& file, CString& errorMessage);

	// Reads the DWORD at the start of a clipboard block and frees the block; throws
	// DittoCore::ClipboardFormatError when the block is shorter than a DWORD
	static DWORD TakeDword(HGLOBAL block);

	// Adds one format's data to the clip's CRC; with adjust, ignores the parts that change on every
	// copy (RTF datastore and rsid values, text block slack); rtfFormat: the registered "Rich Text Format"
	static void AddToCrc(DittoCore::Crc32& crc, const CClipFormat& format, bool adjust, CLIPFORMAT rtfFormat);

	// CF_TEXT and CF_UNICODETEXT bytes up to and including the terminator, within the block;
	// other formats unchanged
	static std::span<const std::byte> TextBytesWithTerminator(CLIPFORMAT type, std::span<const std::byte> bytes);

	/**
	 * @brief LoadFromClipboard's checks before attaching: the ignore and exclude formats, and the
	 * multi-paste delay (sleeps 1500 ms when the delay format is on the clipboard).
	 * @return False when this clipboard change is to be skipped (logged).
	 */
	bool MayReadClipboard();

	/**
	 * @brief LoadFromClipboard's CanIncludeInClipboardHistory check (when the ignore formats are enforced).
	 * @param oleData The attached clipboard.
	 * @return True when the source asks to keep the copy out of the history (logged).
	 */
	bool IsExcludedFromHistory(COleDataObjectEx& oleData);

	/**
	 * @brief LoadFromClipboard's description step: sets m_Desc from CF_UNICODETEXT, else from CF_TEXT.
	 * @param oleData The attached clipboard.
	 * @param cfDesc Receives the format the description was read from and its data.
	 * @return Whether the description was set.
	 */
	bool LoadDescription(COleDataObjectEx& oleData, CClipFormat& cfDesc);

	/**
	 * @brief Tries to set the description from one text format, fetching its data up to 10 times.
	 * @param oleData The attached clipboard.
	 * @param cfDesc Gets the format type, and its data when the format is available.
	 * @param type CF_UNICODETEXT or CF_TEXT.
	 * @param unicode Whether the format is UTF-16 text.
	 * @param typeName The format's name in the log ("cf_unicode", "cf_text").
	 * @return Whether the description was set.
	 */
	bool TryDescriptionFormat(COleDataObjectEx& oleData, CClipFormat& cfDesc, CLIPFORMAT type, bool unicode, const TCHAR* typeName);

	/**
	 * @brief LoadFromClipboard's format loop: adds the clipboard data of each supported type to m_Formats.
	 * @param oleData The attached clipboard.
	 * @param types The supported types, in order.
	 * @param cf The working format (its data is handed to m_Formats).
	 * @param cfDesc The description format; its data moves to m_Formats when its type is loaded.
	 * @param activeApp The source application (lower-cased in place by the CF_DIB check).
	 * @return False when a format is over the maximum clip size (logged); the load is then aborted.
	 */
	bool LoadClipboardFormats(COleDataObjectEx& oleData, CClipTypes& types, CClipFormat& cf, CClipFormat& cfDesc, CString& activeApp);

	/**
	 * @brief Loads one supported type from the clipboard into m_Formats.
	 * @param oleData The attached clipboard.
	 * @param cf The working format; its type is set by the caller.
	 * @param cfDesc The description format.
	 * @param activeApp The source application (lower-cased in place by the CF_DIB check).
	 * @return False when the format is over the maximum clip size (logged).
	 */
	bool LoadClipboardFormat(COleDataObjectEx& oleData, CClipFormat& cf, CClipFormat& cfDesc, CString& activeApp);

	/**
	 * @brief Whether a CF_DIB is skipped because the clipboard has text and the source app's DIBs are ignored.
	 * @param oleData The attached clipboard.
	 * @param type The type being loaded.
	 * @param activeApp The source application; lower-cased in place when the first two checks hold.
	 * @return True when the CF_DIB is skipped.
	 */
	bool IsIgnoredDib(COleDataObjectEx& oleData, CLIPFORMAT type, CString& activeApp);

	/**
	 * @brief Gets the data of cf's type: the description's data when it is that type, else from the clipboard (2 tries).
	 * @param oleData The attached clipboard.
	 * @param cf The working format; receives the data.
	 * @param cfDesc The description format; gives up its data when it is cf's type.
	 * @return False when the type is not on the clipboard (logged).
	 */
	static bool FetchFormatData(COleDataObjectEx& oleData, CClipFormat& cf, CClipFormat& cfDesc);

	/**
	 * @brief Hands cf's data to m_Formats when it is not empty; frees empty data.
	 * @param cf The working format; its data is cleared unless the size check fails.
	 * @param bSuccess Set to true when the format was added.
	 * @return False when the data is over the maximum clip size (logged).
	 */
	bool StoreFetchedFormat(CClipFormat& cf, BOOL& bSuccess);

	/**
	 * @brief LoadFromClipboard's last step: the time, the type description, the description cleanup, the regex filters.
	 * @param oleData The attached clipboard; released here.
	 * @param cfDesc The description format; its data is freed when it was not added.
	 * @param bIsDescSet Whether the description was set from text.
	 * @param regexFilters The text filters that keep a copy out of the history.
	 * @param activeApp The source application.
	 * @return TRUE when the clip is to be saved, FALSE without formats, -1 when a filter matches.
	 */
	int FinishLoadFromClipboard(COleDataObjectEx& oleData, CClipFormat& cfDesc, bool bIsDescSet, CRegExFilterHelper& regexFilters, CString& activeApp);

	/**
	 * @brief AddToDB's sticky step: sets the new sticky order that m_addToDbStickyEnum asks for.
	 * @return The clip whose top-sticky setting is to be removed; -1 for none.
	 * @throws CppSQLite3Exception When an order cannot be read (AddToDB reports it and stops).
	 */
	int ApplyAddToDbSticky();

	/**
	 * @brief WriteTextToFile's write step: writes the first text format that is asked for and present.
	 * @param f The open file.
	 * @param unicode Write CF_UNICODETEXT (UTF-16 with a byte order mark).
	 * @param asci Write CF_TEXT.
	 * @param rtf Write the RTF format.
	 * @param forceUnicode Write the UTF-16 file even when the clip has no unicode text.
	 * @param utf8 Write CF_UNICODETEXT as UTF-8.
	 * @return Whether a format was written.
	 */
	bool WriteTextFormat(CFile& f, BOOL unicode, BOOL asci, BOOL rtf, BOOL forceUnicode, BOOL utf8);

	/**
	 * @brief WriteTextFormat's 8-bit step: writes CF_TEXT, else the RTF, when asked for and not empty.
	 * @param f The open file.
	 * @param a The clip's CF_TEXT.
	 * @param rtfA The clip's RTF.
	 * @param asci Write CF_TEXT.
	 * @param rtf Write the RTF format.
	 * @return Whether a format was written.
	 */
	static bool WriteAnsiTextFormat(CFile& f, CStringA& a, CStringA& rtfA, BOOL asci, BOOL rtf);

	/**
	 * @brief SaveFormats' image step: adds the CF_DIB and PNG bytes that are given and not empty.
	 * @param cf_dibBytes The CF_DIB bytes, or null.
	 * @param pngBytes The PNG bytes, or null.
	 */
	void AddImageFormats(std::vector<BYTE>* cf_dibBytes, std::vector<BYTE>* pngBytes);

	/** @brief Where AddFileDataToData found its formats in m_Formats; -1 when absent. */
	struct FileDataIndexes
	{
		/** @brief The (last) CF_HDROP format. */
		int hdrop{ -1 };
		/** @brief The (last) "Ditto File Data" format. */
		int dittoData{ -1 };
	};

	/**
	 * @brief Finds the CF_HDROP and "Ditto File Data" formats.
	 * @param size The number of formats to search.
	 * @return Their indexes.
	 */
	FileDataIndexes FindFileDataIndexes(INT_PTR size);

	/**
	 * @brief Reads the dropped files for AddFileDataToData.
	 * @param files The paths of the dropped files.
	 * @param copied Receives the files that could be read.
	 * @param newDesc Gets each read file's path and a line feed appended.
	 * @param errorMessage Gets the reasons appended for the files that could not be read.
	 */
	void ReadDroppedFiles(const std::vector<std::wstring>& files, std::vector<CopiedFile>& copied, CString& newDesc, CString& errorMessage);

	/**
	 * @brief Adds one "Ditto File Data" record holding all copied files.
	 * @param copied The files.
	 * @param errorMessage Gets the reason appended when the record is too large.
	 * @return False when the record is too large to save.
	 */
	bool AddFileDataRecord(const std::vector<CopiedFile>& copied, CString& errorMessage);

	/**
	 * @brief Saves the description and the file data record to the database.
	 * @return False when a step failed (that step showed the error); the next step is skipped.
	 */
	bool SaveFileDataToDatabase();
};


/*----------------------------------------------------------------------------*\
	CClipList
\*----------------------------------------------------------------------------*/

// An ordered list of clips that owns them
class CClipList
{
public:
	// Appends a clip at the end; the list owns it from now on
	void Add(std::unique_ptr<CClip> clip);
	// Moves all clips into the returned list, in order; this list is empty afterwards
	CClipList TakeAll();
	// returns the number of clips actually saved
	// while this does empty the Format Data, it does not delete the Clips.
	int AddToDB( bool bLatestOrder = false);
	// The clip added last; the list must not be empty
	CClip& Last();
	/**
	 * @brief The clip saved last by AddToDB (the newest clip whose save succeeded).
	 * @return The clip, owned by this list; nullptr when AddToDB saved no clip.
	 */
	CClip* LastSaved() const { return m_lastSaved; }
	// The number of clips in the list
	size_t Count() const { return m_clips.size(); }
	// True when the list holds no clips
	bool IsEmpty() const { return m_clips.empty(); }

private:
	// The clips, in the order they were added
	std::vector<std::unique_ptr<CClip>> m_clips{};
	/** @brief Non-owning: the clip saved last by AddToDB (one of m_clips), or nullptr. */
	CClip* m_lastSaved{nullptr};
};

#endif // !defined(AFX_PROCESSCOPY_H__185CBB6F_4B63_4397_8FF9_E18D777DA506__INCLUDED_)
