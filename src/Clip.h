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
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>
#include "tinyxml\tinyxml.h"
#include "Crc32Dynamic.h"
#include "ClipRepository.h"
#include "..\Shared\IClip.h"
#include "Misc.h"

class CClip;
class CCopyThread;

typedef CArray<CLIPFORMAT, CLIPFORMAT> CClipTypes;

/*----------------------------------------------------------------------------*\
	COleDataObjectEx
\*----------------------------------------------------------------------------*/
class COleDataObjectEx : public COleDataObject
{
public:
	// creates global from IStream if necessary
	HGLOBAL GetGlobalData(CLIPFORMAT cfFormat, LPFORMATETC lpFormatEtc = NULL);
	std::shared_ptr<CClipTypes> GetAvailableTypes();

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
	int m_dataId;
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
	
	Gdiplus::Bitmap *CreateGdiplusBitmap();
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
	CClip();
	~CClip();
	const CClip& operator=(const CClip &clip);

	static DWORD m_LastAddedCRC;
	static int m_lastAddedID;

	int m_id;
	CClipFormats m_Formats;
	CTime m_Time;
	CString m_Desc;
	ULONG m_lTotalCopySize;
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
	bool AddFormat(CLIPFORMAT cfType, void* pData, UINT nLen, bool setDesc = false);
	int LoadFromClipboard(CClipTypes* pClipTypes, bool checkClipboardIgnore = true, CString activeApp = _T(""));
	bool SetDescFromText(HGLOBAL hgData, bool unicode);
	bool SetDescFromType();
	bool AddToDB(bool bCheckForDuplicates = true);
	bool ModifyMainTable();
	bool ModifyDescription();	
	void MakeLatestOrder();
	void MakeLatestGroupOrder();
	void MakeLastOrder();
	void MakeLastGroupOrder();
	void MakeStickyTop(int parentId);
	void MakeStickyLast(int parentId);
	bool RemoveStickySetting(int parentId);
	BOOL LoadMainTable(int id);
	DWORD GenerateCRC();
	void MoveUp(int parentId);
	void MoveDown(int parentId);
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

	// Allocates a Global containing the requested Clip's Format Data
	static HGLOBAL LoadFormat(int id, UINT cfType);
	// Fills "formats" with the Data of all Formats in the db for the given Clip ID
	bool LoadFormats(int id, bool bOnlyLoad_CF_TEXT = false, bool includeRichTextForTextOnly = false, int dataId = -1);
	// Fills "types" with all Types in the db for the given Clip ID
	static void LoadTypes(int id, CClipTypes& types);

	static double GetNewOrder(int parentId, int clipId);
	double GetNewLastOrder(int parentId, int clipId);
	static double GetNewTopSticky(int parentId, int clipId);
	static double GetNewLastSticky(int parentId, int clipId);
	static int GetExistingTopStickyClipId(int parentId);
	static bool RemoveStickySetting(int clipId, int parentId);

	bool AddFileDataToData(CString &errorMessage);

	Gdiplus::Bitmap *CreateGdiplusBitmap();
	
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

private:
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
	// Moves the clip one place up or down in its list (midpoint of the two neighbours)
	void Move(int parentId, bool up);
	// The highest or lowest order of a column in the main list or a group; nullopt when empty
	static std::optional<double> EdgeOrder(CClipRepository::OrderColumn column, bool sticky, int parentId, bool highest);
	// The repository's parent filter: the group for parentId > -1, all clips otherwise
	static std::optional<int> ParentFilter(int parentId);
	// The repository over theApp's database
	static CClipRepository Repository();
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
	// copy (RTF datastore and rsid values, text block slack)
	static void AddToCrc(CCrc32Dynamic& crc32, const CClipFormat& format, bool adjust, DWORD& crc);

	// CF_TEXT and CF_UNICODETEXT bytes up to and including the terminator, within the block;
	// other formats unchanged
	static std::span<const std::byte> TextBytesWithTerminator(CLIPFORMAT type, std::span<const std::byte> bytes);
};


/*----------------------------------------------------------------------------*\
	CClipList
\*----------------------------------------------------------------------------*/

class CClipList : public CList<CClip*,CClip*>
{
public:
	~CClipList();
	// returns the number of clips actually saved
	// while this does empty the Format Data, it does not delete the Clips.
	int AddToDB( bool bLatestOrder = false);

	const CClipList& operator=(const CClipList &cliplist);
};

#endif // !defined(AFX_PROCESSCOPY_H__185CBB6F_4B63_4397_8FF9_E18D777DA506__INCLUDED_)
