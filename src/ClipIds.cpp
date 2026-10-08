#include "stdafx.h"
#include "CP_Main.h"
#include "ClipIds.h"
#include "..\Shared\TextConvert.h"
#include "Clip_ImportExport.h"
#include "CF_HDropAggregator.h"
#include "CF_UnicodeTextAggregator.h"
#include "CF_TextAggregator.h"
#include "richtextaggregator.h"
#include "htmlformataggregator.h"
#include "Popup.h"
#include "ClipboardFormatError.h"
#include "ErrorReport.h"
#include "DittoDbTransaction.h"
#include "ClipContext.h"
#include "AppWindows.h"
#include "RegisteredClipboardFormats.h"

#include <vector>

// allocate an HGLOBAL of the given Format Type representing these Clip IDs.
HGLOBAL CClipIDs::Render(CClipContext& context, UINT cfType)
{
	INT_PTR count = GetSize();
	if (count <= 0)
	{
		return 0;
	}

	if (count == 1)
	{
		return CClip::LoadFormat(context, ElementAt(0), cfType);
	}

	CGetSetOptions& settings{ context.Settings() };
	CStringA SepA = CTextConvert::UnicodeToAnsi(settings.GetMultiPasteSeparator());
	CStringW SepW = settings.GetMultiPasteSeparator();
	const BOOL bReverse{ settings.m_bMultiPasteReverse };
	const CRegisteredClipboardFormats& formats{ context.Formats() };

	if (cfType == CF_TEXT)
	{
		CCF_TextAggregator CFText(SepA);
		return RenderAggregated(context, CFText, CF_TEXT, bReverse);
	}
	else if (cfType == CF_UNICODETEXT)
	{
		CCF_UnicodeTextAggregator CFUnicodeText(SepW);
		return RenderAggregated(context, CFUnicodeText, CF_UNICODETEXT, bReverse);
	}
	else if (cfType == CF_HDROP)
	{
		CCF_HDropAggregator HDrop;
		return RenderAggregated(context, HDrop, CF_HDROP, bReverse);
	}
	else if (cfType == formats.Html())
	{
		CHTMLFormatAggregator Html(SepW);
		return RenderAggregated(context, Html, formats.Html(), bReverse);
	}
	else if (cfType == formats.Rtf())
	{
		CRichTextAggregator RichText(SepW);
		return RenderAggregated(context, RichText, formats.Rtf(), bReverse);
	}

	return NULL;
}

HGLOBAL CClipIDs::RenderAggregated(CClipContext& context, IClipAggregator& Aggregator, UINT cfType, BOOL bReverse)
{
	if (AggregateData(context, Aggregator, cfType, bReverse, false))
	{
		return Aggregator.GetHGlobal();
	}
	return NULL;
}

void CClipIDs::GetTypes(CClipContext& context, CClipTypes& types)
{
	INT_PTR count = GetSize();
	types.RemoveAll();

	if (count == 1)
	{
		CClip::LoadTypes(context, ElementAt(0), types);
	}
	else if (count > 1)
	{
		GetCommonTypes(context, types, count);
	}
}

void CClipIDs::GetCommonTypes(CClipContext& context, CClipTypes& types, INT_PTR count)
{
	//Add the types that are common across all paste ids
	long lCount{};
	CMap<CLIPFORMAT, CLIPFORMAT, long, long> RenderTypes;

	for (int nIDPos = 0; nIDPos < count; nIDPos++)
	{
		CClipTypes CurrTypes;
		CClip::LoadTypes(context, ElementAt(nIDPos), CurrTypes);

		INT_PTR typeCount = CurrTypes.GetSize();

		for (int type = 0; type < typeCount; type++)
		{
			lCount = 0;
			if (nIDPos == 0 || RenderTypes.Lookup(CurrTypes[type], lCount) == TRUE)
			{
				lCount++;
				RenderTypes.SetAt(CurrTypes[type], lCount);
			}
		}
	}

	CLIPFORMAT Format{};
	POSITION pos = RenderTypes.GetStartPosition();
	while (pos)
	{
		RenderTypes.GetNextAssoc(pos, Format, lCount);
		if (lCount == count)
		{
			types.Add(Format);
		}
	}

	//If there were no common types add the first clip
	if (types.GetSize() <= 0)
	{
		CClip::LoadTypes(context, ElementAt(0), types);
	}
}

// Adds the cfType data of every clip to Aggregator. Errors are not handled here: a malformed clip
// (DittoCore::ClipboardFormatError) or a database error (CppSQLite3Exception) stops the paste at its
// boundary (COleClipSource::OnRenderGlobalData, CProcessPaste::DoPaste/DoDrag), which reports it.
bool CClipIDs::AggregateData(CClipContext& context, IClipAggregator& Aggregator, UINT cfType, BOOL bReverse, bool textOnly)
{
	CString csSQL;
	INT_PTR numIDs = GetSize();
	bool bRet = false;

	INT_PTR nIndex;
	for (int i = 0; i < numIDs; i++)
	{
		nIndex = i;
		if (bReverse)
		{
			nIndex = numIDs - i - 1;
		}

		// a text-only paste also takes file lists, as their paths
		CString sqlCF_HDROP = _T("");
		if (textOnly &&
			(cfType == CF_UNICODETEXT || cfType == CF_TEXT))
		{
			sqlCF_HDROP.Format(_T("OR Data.strClipBoardFormat = '%s'"), CClipboardFormats::GetFormatName(CF_HDROP).GetString());
		}

		csSQL.Format(_T("SELECT * FROM Data ")
					 _T("INNER JOIN Main ON Main.lID = Data.lParentID ")
					 _T("WHERE (Data.strClipBoardFormat = '%s'")
					 _T(" %s) ")
					 _T("AND Main.lID = %d"),
					 // Clipboard format ids are 16-bit values, so they fit a CLIPFORMAT
					 CClipboardFormats::GetFormatName(static_cast<CLIPFORMAT>(cfType)).GetString(),
					 sqlCF_HDROP.GetString(),
					 ElementAt(nIndex));

		CppSQLite3Query q = context.Database().execQuery(csSQL);

		if (q.eof() == false)
		{
			int nDataLen = 0;
			LPVOID pData = (LPVOID)q.getBlobField(_T("ooData"), nDataLen);
			if (pData == NULL)
			{
				continue;
			}

			if (Aggregator.AddClip(pData, nDataLen, (int)i, (int)numIDs, CClipboardFormats::GetFormatID(q.getStringField(_T("strClipBoardFormat")))))
			{
				bRet = true;
			}
		}
	}

	return bRet;
}

// Blindly Moves IDs into the lParentID Group sequentially with the given order
BOOL CClipIDs::MoveTo(CClipContext& context, long lParentID, double /*dFirst*/, double /*dIncrement*/)
{
	try
	{
		int count = (int)GetSize();

		CLogger::Log(CStringUtil::Format(_T("MoveTo, Start, Size: %d, ParentId: %d"), count, lParentID));

		// all or none: upstream moved clip by clip, so a failure left part of them moved
		CDittoDbTransaction transaction(context.Database());

		for (int i = count - 1; i >= 0; i--)
		{
			CString sql;

			if (lParentID > 0)
			{
				sql = CStringUtil::Format(_T("UPDATE Main SET lParentID = %d, clipGroupOrder = %f WHERE lID = %d AND lID <> %d;"),
										  lParentID,
										  CClip::GetNewOrder(context, lParentID, ElementAt(i)),
										  ElementAt(i),
										  lParentID);
			}
			else
			{
				sql = CStringUtil::Format(_T("UPDATE Main SET lParentID = %d WHERE lID = %d AND lID <> %d;"),
										  lParentID,
										  ElementAt(i),
										  lParentID);
			}

			int ret = context.Database().execDMLEx(sql);

			CLogger::Log(CStringUtil::Format(_T("MoveTo, Sql Ret: %d, SQL: %s"), ret, sql.GetString()));
		}

		transaction.Commit();
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Moving the clips to group %d failed, none was moved: %s"), lParentID, e.errorMessage()));
		return FALSE;
	}

	return (TRUE);
}

BOOL CClipIDs::DeleteIDs(CAppWindows& windows, bool fromClipWindow, CDittoDb& db)
{
	CPopup status(0, 0, ::GetForegroundWindow());
	bool bAllowShow;
	bAllowShow = CWindowInspector::IsAppWnd(::GetForegroundWindow());

	INT_PTR count = GetSize();
	int batchCount = 25;
	// the windows are told only after the commit, of clips that are really gone
	std::vector<int> deletedIds{};

	CLogger::Log(CStringUtil::Format(_T("Begin delete clips, Count: %zd from Window: %d"), count, fromClipWindow));

	if (count <= 0)
		return FALSE;

	try
	{
		// one transaction: upstream deleted batch by batch without one, so a failure left part of
		// the clips deleted, and the windows had been told of clips that were not
		CDittoDbTransaction transaction(db);

		CString sql = _T("DELETE FROM Main where lId in(");
		CString sqlIn = _T("");
		CString workingString = _T("Deleting clips, building query statement");
		INT_PTR startIndex = 0;
		INT_PTR index = 0;

		ShowDeleteStatus(status, bAllowShow, workingString);

		for (index = 0; index < count; index++)
		{
			int clipId = ElementAt(index);
			if (clipId <= 0)
				continue;

			CLogger::Log(CStringUtil::Format(_T("Delete clip Id: %d"), clipId));

			AddExistingClipToDelete(db, clipId, sqlIn);

			if (IsDeleteBatchEnd(index, batchCount))
			{
				ShowDeleteStatus(status, bAllowShow, CStringUtil::Format(_T("Deleting %zd - %zd of %zd..."), startIndex + 1, index, count));
				startIndex = index;

				db.execDMLEx(sql + sqlIn + _T(")"));
				sqlIn = "";

				ShowDeleteStatus(status, bAllowShow, workingString);
			}


			if (fromClipWindow == false)
			{
				deletedIds.push_back(clipId);
			}
		}

		if (sqlIn.GetLength() > 0)
		{
			ShowDeleteStatus(status, bAllowShow, CStringUtil::Format(_T("Deleting %zd - %zd of %zd..."), startIndex + 1, index, count));

			db.execDMLEx(sql + sqlIn + _T(")"));
		}

		transaction.Commit();
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Deleting the selected clips failed, none was deleted: %s"), e.errorMessage()));
		return FALSE;
	}

	for (const int clipId : deletedIds)
	{
		windows.OnDeleteID(clipId);
	}

	CLogger::Log(CStringUtil::Format(_T("End delete clips, Count: %zd"), count));

	return TRUE;
}

bool CClipIDs::IsDeleteBatchEnd(INT_PTR index, int batchCount)
{
	return index > 0 &&
		   (index % batchCount) == 0;
}

void CClipIDs::ShowDeleteStatus(CPopup& status, bool bAllowShow, const CString& text)
{
	if (bAllowShow)
	{
		status.Show(text);
	}
}

void CClipIDs::AddExistingClipToDelete(CppSQLite3DB& db, int clipId, CString& sqlIn)
{
	bool cont{ false };
	bool bGroup{ false };
	{
		CppSQLite3Query q{ db.execQueryEx(_T("SELECT bIsGroup FROM Main WHERE lId = %d"), clipId) };
		cont = !q.eof();
		if (cont)
		{
			bGroup = q.getIntField(_T("bIsGroup")) > 0;
		}
	}

	if (cont)
	{
		if (bGroup)
		{
			db.execDMLEx(_T("UPDATE Main SET lParentID = -1 WHERE lParentID = %d;"), clipId);
		}

		if (sqlIn.GetLength() > 0)
		{
			sqlIn += ", ";
		}
		sqlIn += CStringUtil::Format(_T("%d"), clipId);
	}
}

BOOL CClipIDs::CreateExportSqliteDB(CppSQLite3DB& db)
{
	BOOL bRet = FALSE;
	try
	{
		db.execDML(_T("CREATE TABLE Main(")
				   _T("lID INTEGER PRIMARY KEY AUTOINCREMENT, ")
				   _T("lVersion INTEGER, ")
				   _T("mText TEXT);"));

		db.execDML(_T("CREATE TABLE Data(")
				   _T("lID INTEGER PRIMARY KEY AUTOINCREMENT, ")
				   _T("lParentID INTEGER, ")
				   _T("strClipBoardFormat TEXT, ")
				   _T("lOriginalSize INTEGER, ")
				   _T("ooData BLOB);"));

		db.execDML(_T("CREATE UNIQUE INDEX Main_ID on Main(lID ASC)"));
		db.execDML(_T("CREATE UNIQUE INDEX Data_ID on Data(lID ASC)"));

		db.execDML(_T("CREATE TRIGGER delete_data_trigger BEFORE DELETE ON Main FOR EACH ROW\n")
				   _T("BEGIN\n")
				   _T("DELETE FROM Data WHERE lParentID = old.lID;\n")
				   _T("END\n"));

		bRet = TRUE;
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Creating the export database failed: %s"), e.errorMessage()));
		return FALSE;
	}

	return bRet;
}

BOOL CClipIDs::Export(CClipContext& context, CString csFilePath)
{
	INT_PTR count = GetSize();
	if (count == 0)
		return TRUE;

	BOOL bRet = FALSE;

	if (CFileSystem::FileExists(csFilePath) && DeleteFile(csFilePath) == FALSE)
	{
		// shown: upstream only logged it, so the export silently did nothing
		CErrorReport::Show(CStringUtil::Format(_T("Exporting the clips failed: the existing file %s could not be replaced, error %lu"), csFilePath.GetString(), ::GetLastError()));
		return FALSE;
	}

	try
	{
		CppSQLite3DB db;
		db.open(csFilePath);

		if (CreateExportSqliteDB(db) == FALSE)
			return FALSE;

		bRet = ExportClips(context, db);

		db.close();
	}
	catch (const DittoCore::ClipboardFormatError& error)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Export stopped: a clip could not be exported (%s)."), CString(error.what()).GetString()));
		return FALSE;
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Exporting the clips to %s failed: %s"), csFilePath.GetString(), e.errorMessage()));
		return FALSE;
	}

	return bRet;
}

BOOL CClipIDs::ExportClips(CClipContext& context, CppSQLite3DB& db)
{
	INT_PTR exported{ 0 };
	CString skipped{};
	INT_PTR count{ GetSize() };
	for (int i = 0; i < count; i++)
	{
		int nID{ ElementAt(i) };

		CClip_ImportExport clip{ context };

		// a clip that does not load (a database error is shown by the load) or has no data (a
		// group) is not exported; upstream skipped it silently and still returned success
		if (clip.LoadMainTable(nID) == FALSE || clip.LoadFormats(nID) == false)
		{
			if (skipped.IsEmpty() == false)
			{
				skipped += _T(", ");
			}
			skipped.AppendFormat(_T("%d"), nID);
			continue;
		}

		clip.ExportToSqliteDB(db);
		exported++;
	}

	if (skipped.IsEmpty() == false)
	{
		CErrorReport::Show(CStringUtil::Format(_T("%Id of %Id clips were exported. These clips could not be loaded or have no data and were not exported: %s"), exported, count, skipped.GetString()));
		return FALSE;
	}

	return TRUE;
}
