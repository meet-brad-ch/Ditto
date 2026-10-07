#include "stdafx.h"
#include "QPasteWndThread.h"
#include "Misc.h"
#include "Options.h"
#include "QPasteWnd.h"
#include "cp_main.h"
#include "ErrorReport.h"
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <string>

CQPasteWndThread::CQPasteWndThread(void)
{
	m_rowHeight = 0;
	m_threadName = "CQPasteWndThread";
    m_waitTimeout = CMilliseconds::OneHour * 12;

    m_SearchingEvent = CreateEvent(NULL, TRUE, FALSE, _T(""));

    for(int eventEnum = 0; eventEnum < ECQPASTEWNDTHREADEVENTS_COUNT; eventEnum++)
    {
        AddEvent(eventEnum);
    }
}

CQPasteWndThread::~CQPasteWndThread(void)
{
    CloseHandle(m_SearchingEvent);
}

void CQPasteWndThread::OnTimeOut(void * /*param*/)
{
}

void CQPasteWndThread::OnEvent(int eventId, void *param)
{
	ULONGLONG startTick = GetTickCount64();
	CLogger::Log(StrF(_T("Start of OnEvent, eventId: %s"), EnumName((eCQPasteWndThreadEvents)eventId).GetString()));

    switch((eCQPasteWndThreadEvents)eventId)
    {
        case DO_SET_LIST_COUNT:
            OnSetListCount(param);
            break;
        case LOAD_ACCELERATORS:
            OnLoadAccelerators(param);
            break;
        case UNLOAD_ACCELERATORS:
            OnUnloadAccelerators(param);
            break;
        case LOAD_ITEMS:
            OnLoadItems(param);
            break;
        case LOAD_EXTRA_DATA:
            OnLoadExtraData(param);
            break;
    }

	ULONGLONG length = GetTickCount64() - startTick;
	CLogger::Log(StrF(_T("End of OnEvent, eventId: %s, Time: %llu(ms)"), EnumName((eCQPasteWndThreadEvents)eventId).GetString(), length));
}

void CQPasteWndThread::OnSetListCount(void *param)
{
    CQPasteWnd *pasteWnd = (CQPasteWnd*)param;

    static CEvent UpdateTimeEvent(TRUE, TRUE, _T("Ditto_Update_Clip_Time"), NULL);
    //If we pasted then wait for the time on the pasted event to be updated before we query the db
    if (WaitForSingleObject(UpdateTimeEvent, 2000) == WAIT_FAILED)
    {
        throw std::runtime_error("waiting for the clip time update event failed, error " + std::to_string(::GetLastError()));
    }

    ResetEvent(m_SearchingEvent);
    ULONGLONG lTick = GetTickCount64();

	CString countSQL = m_countSql;

    long lRecordCount = 0;

    try
    {
        lRecordCount = theApp.m_db.execScalar(countSQL);
        ::PostMessage(pasteWnd->m_hWnd, CQListCtrl::NmSetListCount, lRecordCount, 0);
    }
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Counting the clips for the clip list failed: %s"), e.errorMessage()));
		SetEvent(m_SearchingEvent); // the count is over, so waiters on the search may go on
		return;
	}

    SetEvent(m_SearchingEvent);

    CLogger::Log(StrF(_T("Set list count = %d, time = %llu"), lRecordCount, GetTickCount64() - lTick));
}

void CQPasteWndThread::OnLoadItems(void *param)
{
    CQPasteWnd *pasteWnd = (CQPasteWnd*)param;

    ResetEvent(m_SearchingEvent);

	while(true)
	{
		ULONGLONG startTick = GetTickCount64();
		LoadItemsRequest request{};
		CString localSql = m_sql;

		TakeLoadItemsRequest(pasteWnd, request);

	    if(request.clearFirstLoadItem)
	    {
			try
			{
				CLogger::Log(StrF(_T("Load Items start = %d, count = %d, list size: %zu"), request.index, request.count, request.listSize));

				CString limit;
				limit.Format(_T(" LIMIT %d OFFSET %d"), request.count, request.index);
				localSql += limit;

				int loadCount = LoadItemRows(pasteWnd, localSql, request);

				ULONGLONG loadTime = GetTickCount64() - startTick;
				ULONGLONG countCountStart = GetTickCount64();
				ULONGLONG countCount = 0;
				ULONGLONG acceleratorCount = 0;

				if(request.firstLoad)
				{
					// OnRefeshRow reads the clip id back as an int, so -2 survives the WPARAM round trip.
					::PostMessage(pasteWnd->m_hWnd, CQListCtrl::NmRefreshRow, static_cast<WPARAM>(-2), 0);
					//allow the next thread message to process, this should be the message to set the list count

					OnSetListCount(param);

					countCount = GetTickCount64() - countCountStart;
					ULONGLONG acceleratorCountStart = GetTickCount64();

					OnLoadAccelerators(param);

					acceleratorCount = GetTickCount64() - acceleratorCountStart;
				}
				else
				{
					::PostMessage(pasteWnd->m_hWnd, CQListCtrl::NmRefreshRow, static_cast<WPARAM>(-1), 0);
				}

				if(request.clearFirstLoadItem)
				{
					ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

					pasteWnd->m_loadItems.erase(pasteWnd->m_loadItems.begin());
				}

				CLogger::Log(StrF(_T("Load items End count = %d, Total Time = %llu, LoadItems: %llu, Count: %llu, Accel: %llu"), loadCount, GetTickCount64() - startTick, loadTime, countCount, acceleratorCount));
			}
			catch (CppSQLite3Exception& e)	\
			{								\
				CLogger::Log(StrF(_T("ONLoadItems - SQLITE Exception %d - %s"), e.errorCode(), e.errorMessage()));	\
				ASSERT(FALSE);				\
				break;
			}	
		}
		else
		{
			break;
		}
	}

    SetEvent(m_SearchingEvent);
}

void CQPasteWndThread::TakeLoadItemsRequest(CQPasteWnd *pasteWnd, LoadItemsRequest &request)
{
	ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

    if(pasteWnd->m_loadItems.size() > 0)
    {
		request.firstLoad = (pasteWnd->m_loadItems.begin()->x == -1);
        request.index = max(pasteWnd->m_loadItems.begin()->x, 0);
        request.count = pasteWnd->m_loadItems.begin()->y - pasteWnd->m_loadItems.begin()->x;
        pasteWnd->m_bStopQuery = false;
		request.listSize = pasteWnd->m_listItems.size();
        request.clearFirstLoadItem = true;
    }
}

int CQPasteWndThread::LoadItemRows(CQPasteWnd *pasteWnd, const CString &localSql, const LoadItemsRequest &request)
{
	int loadItemsIndex = request.index;
	int loadCount = 0;
	int pos = loadItemsIndex;

	CMainTable table;

	CppSQLite3Query q = theApp.m_db.execQuery(localSql);
	while(!q.eof())
	{
		CQPasteWnd::FillMainTable(table, q);

		int updateIndex = StoreLoadedItem(pasteWnd, table, pos);

		if(pasteWnd->m_bStopQuery)
		{
			CLogger::Log(StrF(_T("StopQuery called exiting filling cache count = %d"), loadItemsIndex));
			break;
		}

		q.nextRow();

		if(request.firstLoad == false)
		{
			/*if (updateIndex != loadItemsIndex)
			{
				CLogger::Log(StrF(_T("index difference old: %d, new: %d"), loadItemsIndex, updateIndex));
			}*/

    		::PostMessage(pasteWnd->m_hWnd, CQListCtrl::NmRefreshRow, table.m_lID, updateIndex);
		}

		loadItemsIndex++;
		loadCount++;
		pos++;
	}

	return loadCount;
}

int CQPasteWndThread::StoreLoadedItem(CQPasteWnd *pasteWnd, const CMainTable &table, int pos)
{
	int updateIndex = -1;

	ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

	// pos starts at loadItemsIndex (never negative) and only grows
	const size_t listPos{ static_cast<size_t>(pos) };
	if (listPos < pasteWnd->m_listItems.size())
	{
		pasteWnd->m_listItems[pos] = table;

		updateIndex = pos;

		//CLogger::Log(StrF(_T("updating list pos = %d, id: %d, size: %d"), pos, table.m_lID, pasteWnd->m_listItems.size() - 1));
	}
	else if (listPos == pasteWnd->m_listItems.size())
	{
		pasteWnd->m_listItems.push_back(table);
		updateIndex = (int)pasteWnd->m_listItems.size() - 1;
		//CLogger::Log(StrF(_T("adding (same size) list pos = %d, id: %d, size: %d"), pasteWnd->m_listItems.size()-1, table.m_lID, pasteWnd->m_listItems.size() - 1));
	}
	else if (listPos > pasteWnd->m_listItems.size())
	{
		for (int toAdd = (int)pasteWnd->m_listItems.size()-1; toAdd < pos - 1; toAdd++)
		{
			CMainTable empty;
			empty.m_lID = -1;
			pasteWnd->m_listItems.push_back(empty);

			//CLogger::Log(StrF(_T("adding dummy row size: %d"), pasteWnd->m_listItems.size()-1));
		}

		pasteWnd->m_listItems.push_back(table);

		updateIndex = (int)pasteWnd->m_listItems.size() - 1;

		//CLogger::Log(StrF(_T("adding list pos = %d, id: %d, size: %d"), pasteWnd->m_listItems.size()-1, table.m_lID, pasteWnd->m_listItems.size() - 1));
	}

	return updateIndex;
}

void ReduceMapItems(CF_DibTypeMap &mapItem, CCriticalSection &critSection, CString mapName)
{
	ATL::CCritSecLock csLock(critSection.m_sect);

	const size_t maxSize{ 50 };
	const int reduceToSize{ 30 };

	if (mapItem.size() > maxSize)
	{
		//create a vector so we can sort and keep the last x number of events
		vector<INT64> counterArray;
		for (CF_DibTypeMap::iterator iterDib = mapItem.begin(); iterDib != mapItem.end(); iterDib++)
		{
			counterArray.push_back(iterDib->second.m_counter);
		}
		std::sort(counterArray.begin(), counterArray.end());
		counterArray.erase(counterArray.begin(), counterArray.end() - reduceToSize);

		//remove the oldest x number if bitmaps
		for (CF_DibTypeMap::iterator iterDib = mapItem.begin(); iterDib != mapItem.end();)
		{
			if (std::binary_search(counterArray.begin(), counterArray.end(), iterDib->second.m_counter) == false)
			{
				CLogger::Log(StrF(_T("reduced size of %s cache, Id: %d, Row: %d"), mapName.GetString(), iterDib->second.m_parentId, iterDib->second.m_clipRow));

				mapItem.erase(iterDib++);
			}
			else
			{
				++iterDib;
			}
		}

		CLogger::Log(StrF(_T("reduced size of %s cache, count: %d"), mapName.GetString(), mapItem.size()));
	}
}

void CQPasteWndThread::OnLoadExtraData(void *param)
{
    ResetEvent(m_SearchingEvent);

    CQPasteWnd *pasteWnd = (CQPasteWnd*)param;

    CLogger::Log(_T("Start of load extra data, Bitmaps/rtf"));

    std::list<CClipFormatQListCtrl> localFormats;
	{
		ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

		for (std::list<CClipFormatQListCtrl>::iterator it = pasteWnd->m_ExtraDataLoadItems.begin(); it != pasteWnd->m_ExtraDataLoadItems.end(); it++)
		{
			localFormats.push_back(*it);
		}
	    pasteWnd->m_ExtraDataLoadItems.clear();
	}
	
	for (std::list<CClipFormatQListCtrl>::iterator it = localFormats.begin(); it != localFormats.end(); it++)
    {
		bool loadClip = NeedsExtraDataLoad(pasteWnd, *it);

		if (loadClip)
		{
			LoadExtraDataFormat(pasteWnd, *it);
		}

		if (it->m_cfType == CF_DIB)
		{
			ReduceMapItems(pasteWnd->m_cf_dibCache, pasteWnd->m_CritSection, _T("image"));
		}
		else if (it->m_cfType == theApp.m_RTFFormat)
		{
			ReduceMapItems(pasteWnd->m_cf_rtfCache, pasteWnd->m_CritSection, _T("rtf"));
		}
    }

    SetEvent(m_SearchingEvent);
    CLogger::Log(_T("End of load extra data, Bitmaps/rtf"));
}

bool CQPasteWndThread::NeedsExtraDataLoad(CQPasteWnd *pasteWnd, const CClipFormatQListCtrl &format)
{
	bool loadClip = true;

	if (format.m_cfType == CF_DIB)
	{
		ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

		CF_DibTypeMap::iterator iterDib = pasteWnd->m_cf_dibCache.find(format.m_parentId);
		if (iterDib != pasteWnd->m_cf_dibCache.end())
		{
			loadClip = false;
		}
		else
		{
			CF_NoDibTypeMap::iterator iterNoDib = pasteWnd->m_cf_NO_dibCache.find(format.m_parentId);
			if (iterNoDib != pasteWnd->m_cf_NO_dibCache.end())
			{
				loadClip = false;
			}
		}
	}
	else if (format.m_cfType == theApp.m_RTFFormat)
	{
		ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

		CF_DibTypeMap::iterator iterDib = pasteWnd->m_cf_rtfCache.find(format.m_parentId);
		if (iterDib != pasteWnd->m_cf_rtfCache.end())
		{
			loadClip = false;
		}
		else
		{
			CF_NoDibTypeMap::iterator iterNoRtf = pasteWnd->m_cf_NO_rtfCache.find(format.m_parentId);
			if (iterNoRtf != pasteWnd->m_cf_NO_rtfCache.end())
			{
				loadClip = false;
			}
		}
	}

	return loadClip;
}

void CQPasteWndThread::LoadExtraDataFormat(CQPasteWnd *pasteWnd, CClipFormatQListCtrl &format)
{
	ULONGLONG startLoadClipData = GetTickCount64();

	BOOL foundClipData = GetExtraClipData(format);

	if (foundClipData)
	{
		ULONGLONG timeTook = GetTickCount64() - startLoadClipData;
		if (timeTook > 20)
		{
			CLogger::Log(StrF(_T("GetClipData for clip %d, took: %llu"), format.m_parentId, timeTook));
		}

		CacheExtraData(pasteWnd, format);

		::PostMessage(pasteWnd->m_hWnd, CQListCtrl::NmRefreshRow, format.m_parentId, format.m_clipRow);
	}
	else
	{
		MarkNoExtraData(pasteWnd, format);
	}
}

BOOL CQPasteWndThread::GetExtraClipData(CClipFormatQListCtrl &format)
{
	BOOL foundClipData = theApp.GetClipData(format.m_parentId, format);
	if (foundClipData == false &&
		format.m_cfType == CF_DIB)
	{
		format.Free();
		format.m_cfType = theApp.m_PNG_Format;

		foundClipData = theApp.GetClipData(format.m_parentId, format);
	}

	return foundClipData;
}

void CQPasteWndThread::CacheExtraData(CQPasteWnd *pasteWnd, CClipFormatQListCtrl &format)
{
	if (format.m_cfType == CF_DIB ||
		format.m_cfType == theApp.m_PNG_Format)
	{
		ULONGLONG startConvertImage = GetTickCount64();

		HDC dc = GetDC(NULL);

		format.GetDibFittingToHeight(CDC::FromHandle(dc), m_rowHeight);

		ReleaseDC(NULL, dc);

		ULONGLONG convertTime = GetTickCount64() - startConvertImage;
		if (convertTime > 20)
		{
			CLogger::Log(StrF(_T("GetDibFittingToHeight for clip %d, took: %llu"), format.m_parentId, convertTime));
		}

		{
			ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

			pasteWnd->m_cf_dibCache[format.m_parentId] = format;
			//the cache now owns the format data, set it to delete the data in the destructor
			pasteWnd->m_cf_dibCache[format.m_parentId].m_autoDeleteData = true;

			CLogger::Log(StrF(_T("Loaded, extra data for clipId: %d, Row: %d image cache count: %d"), format.m_parentId, format.m_clipRow, pasteWnd->m_cf_dibCache.size()));
		}
	}
	else if (format.m_cfType == theApp.m_RTFFormat)
	{
		ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

		pasteWnd->m_cf_rtfCache[format.m_parentId] = format;
		format.m_autoDeleteData = false;
		//the cache now owns the format data, set it to delete the data in the destructor
		pasteWnd->m_cf_rtfCache[format.m_parentId].m_autoDeleteData = true;

		CLogger::Log(StrF(_T("Loaded, extra data for clip %d, rtf cache count: %d"), format.m_parentId, pasteWnd->m_cf_rtfCache.size()));
	}
}

void CQPasteWndThread::MarkNoExtraData(CQPasteWnd *pasteWnd, const CClipFormatQListCtrl &format)
{
	ATL::CCritSecLock csLock(pasteWnd->m_CritSection.m_sect);

	if (format.m_cfType == CF_DIB ||
		format.m_cfType == theApp.m_PNG_Format)
	{
		pasteWnd->m_cf_NO_dibCache[format.m_parentId] = true;
	}
	else if (format.m_cfType == theApp.m_RTFFormat)
	{
		pasteWnd->m_cf_NO_rtfCache[format.m_parentId] = true;
	}
}

void CQPasteWndThread::OnLoadAccelerators(void *param)
{
    CQPasteWnd *pasteWnd = (CQPasteWnd*)param;
    pasteWnd->m_lstHeader.DestroyAndCreateAccelerator(TRUE, theApp.m_db);
}

void CQPasteWndThread::OnUnloadAccelerators(void *param)
{
    CQPasteWnd *pasteWnd = (CQPasteWnd*)param;
    pasteWnd->m_lstHeader.DestroyAndCreateAccelerator(FALSE, theApp.m_db);
}

CString CQPasteWndThread::EnumName(eCQPasteWndThreadEvents e)
{
	switch(e)
	{
	case DO_SET_LIST_COUNT:
		return _T("Load List Count");
	case LOAD_ACCELERATORS:
		return _T("Load Accelerators");
	case UNLOAD_ACCELERATORS:
		return _T("Unload Accelerators");
	case LOAD_ITEMS:
		return _T("Load clips");
	case LOAD_EXTRA_DATA:
		return _T("Load Extra Data (rtf/bitmaps)");
	}

	return _T("");
}