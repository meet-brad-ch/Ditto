#pragma once
#include "EventThread.h"
#include "sqlite/CppSQLite3.h"

class CQPasteWnd;
class CMainTable;
class CClipFormatQListCtrl;
class CGetSetOptions;

class CQPasteWndThread: public CEventThread
{
public:
    /**
     * @brief Creates the quick paste window's loader thread (not started yet).
     * @param settings The application settings (owned by the composition root; outlives the thread).
     */
    explicit CQPasteWndThread(CGetSetOptions& settings);
    ~CQPasteWndThread(void);

    enum eCQPasteWndThreadEvents
    {
		DO_SET_LIST_COUNT, 
		LOAD_ACCELERATORS, 
		UNLOAD_ACCELERATORS, 
		LOAD_ITEMS, 
		LOAD_EXTRA_DATA, 

        ECQPASTEWNDTHREADEVENTS_COUNT  //must be last

    };

    void FireSetListCount()
    {
        FireEvent(DO_SET_LIST_COUNT);
    }
    void FireLoadItems(bool /*firstLoad*/)
    {
        FireEvent(LOAD_ITEMS);
    }
    void FireLoadExtraData(int rowHeight)
    {
		m_rowHeight = rowHeight;
        FireEvent(LOAD_EXTRA_DATA);
    }
    void FireLoadAccelerators()
    {
        FireEvent(LOAD_ACCELERATORS);
    }
    void FireUnloadAccelerators()
    {
        FireEvent(UNLOAD_ACCELERATORS);
    }

    HANDLE m_SearchingEvent;

	void SetRowHeight(int height) { m_rowHeight = height; }
    void SetSearchSql(CString sql, CString countSql) { m_sql = sql; m_countSql = countSql; }

protected:
    virtual void OnEvent(int eventId, void *param);
    virtual void OnTimeOut(void *param);

    void OnSetListCount(void *param);
    void OnLoadItems(void *param);
    void OnLoadExtraData(void *param);
	/** @brief Trims a format cache (CF_DibTypeMap) of more than 50 entries to the 30 most recently used ones.
	@param mapItem the cache, keyed by row.
	@param critSection the paste window's lock, held while the cache is trimmed.
	@param mapName the cache name for the log ("image", "rtf"). */
	static void ReduceMapItems(std::map<int, CClipFormatQListCtrl> &mapItem, CCriticalSection &critSection, CString mapName);
    void OnLoadAccelerators(void *param);
    void OnUnloadAccelerators(void *param);

	CString EnumName(eCQPasteWndThreadEvents e);

	/** @brief The first queued range of list rows to load, as OnLoadItems takes it. */
	struct LoadItemsRequest
	{
		/** @brief The first row to load. */
		int index{ 0 };
		/** @brief The number of rows to load. */
		int count{ 0 };
		/** @brief true for the first load after a fill (the range starts at -1). */
		bool firstLoad{ false };
		/** @brief The list size when the range was taken (for the log). */
		size_t listSize{ 0 };
		/** @brief true when a range was queued (and must be removed after the load). */
		bool clearFirstLoadItem{ false };
	};

	/** @brief Takes the first queued range of rows to load, if there is one, and clears the stop flag.
	@param pasteWnd the paste window.
	@param request the range to set. */
	void TakeLoadItemsRequest(CQPasteWnd *pasteWnd, LoadItemsRequest &request);
	/** @brief Loads the rows of a range into the list.
	@param pasteWnd the paste window.
	@param localSql the list query with LIMIT and OFFSET.
	@param request the range.
	@return the number of rows loaded.
	@throws CppSQLite3Exception on a database error. */
	int LoadItemRows(CQPasteWnd *pasteWnd, const CString &localSql, const LoadItemsRequest &request);
	/** @brief Stores a loaded row in the list at a position, adding empty rows before it when needed.
	@param pasteWnd the paste window.
	@param table the loaded row.
	@param pos the list position.
	@return the list index of the row, or -1. */
	int StoreLoadedItem(CQPasteWnd *pasteWnd, const CMainTable &table, int pos);

	/** @brief Is a queued format neither cached nor known to be missing?
	@param pasteWnd the paste window.
	@param format the queued format.
	@return true when it must be loaded. */
	bool NeedsExtraDataLoad(CQPasteWnd *pasteWnd, const CClipFormatQListCtrl &format);
	/** @brief Loads a queued format and caches it, or records that the clip does not have it.
	@param pasteWnd the paste window.
	@param format the queued format. */
	void LoadExtraDataFormat(CQPasteWnd *pasteWnd, CClipFormatQListCtrl &format);
	/** @brief Reads the data of a format; a missing bitmap falls back to PNG.
	@param format the format; its type changes to PNG on the fallback.
	@return TRUE when the data was found. */
	BOOL GetExtraClipData(CClipFormatQListCtrl &format);
	/** @brief Puts a loaded image (scaled to the row height) or rich text format in its cache.
	@param pasteWnd the paste window.
	@param format the loaded format. */
	void CacheExtraData(CQPasteWnd *pasteWnd, CClipFormatQListCtrl &format);
	/** @brief Records that a clip does not have an image or rich text format.
	@param pasteWnd the paste window.
	@param format the format that was not found. */
	void MarkNoExtraData(CQPasteWnd *pasteWnd, const CClipFormatQListCtrl &format);

	int m_rowHeight;
	/** @brief The application settings (the image cache reads the thumbnail mode). */
	CGetSetOptions& m_settings;

    CString m_sql;
    CString m_countSql;
};
