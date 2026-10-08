#include "stdafx.h"
#include "CP_Main.h"
#include ".\clip_importexport.h"
#include "sqlite/CppSQLite3.h"
#include "ClipboardFormatError.h"
#include "DtoCodec.h"
#include "GlobalBytes.h"
#include "Misc.h"
#include <basetsd.h>
#include <minwindef.h>
#include <tchar.h>
#include <WinBase.h>
#include <WinUser.h>
#include <afx.h>
#include <afxstr.h>
#include "Clip.h"
#include "ClipContext.h"
#include "AppWindows.h"

#include <span>
#include <string>
#include <vector>

CClip_ImportExport::CClip_ImportExport(CClipContext& context) :
	CClip(context),
	m_importCount(0)
{

}

CClip_ImportExport::~CClip_ImportExport(void)
{
}

// A CppSQLite3Exception propagates: the caller (CClipIDs::Export) is the operation boundary and reports it.
bool CClip_ImportExport::ExportToSqliteDB(CppSQLite3DB& db)
{
	//Add to Main Table
	m_Desc.Replace(_T("'"), _T("''"));
	db.execDMLEx(_T("insert into Main values(NULL, %d, '%s');"), s_currentExportVersion, m_Desc.GetString());
	const sqlite_int64 lId{ db.lastRowId() };

	//Add to Data table
	CClipFormat* pCF;
	CppSQLite3Statement stmt = db.compileStatement(_T("insert into Data values (NULL, ?, ?, ?, ?);"));

	for (INT_PTR i = m_Formats.GetSize() - 1; i >= 0; i--)
	{
		pCF = &m_Formats.ElementAt(i);

		stmt.bindInt64(1, lId);
		stmt.bind(2, CClipboardFormats::GetFormatName(pCF->m_cfType).GetString());

		const DittoCore::GlobalBytes block(pCF->m_hgData);
		const std::vector<std::byte> compressed = DittoCore::DtoCodec::Compress(block.Bytes());
		stmt.bind(3, static_cast<int>(block.Bytes().size()));
		stmt.bind(4, reinterpret_cast<const unsigned char*>(compressed.data()), static_cast<int>(compressed.size()));

		stmt.execDML();
		stmt.reset();

		m_Formats.RemoveAt(i);
	}

	return true;
}

// A CppSQLite3Exception propagates: the callers (CClipCommands::ImportClips and CCP_MainApp::ImportFileFromCommandLine)
// are the operation boundaries and show it.
bool CClip_ImportExport::ImportFromSqliteDB(CppSQLite3DB& db, bool bAddToDB, bool bPutOnClipboard)
{
	bool bRet = false;
	CStringA csCF_TEXT;
	CStringW csCF_UNICODETEXT;

	CppSQLite3Query q = db.execQuery(_T("Select * from Main order by lId DESC"));
	while (q.eof() == false)
	{
		Clear();

		if (ImportRow(db, q, bAddToDB, bPutOnClipboard))
		{
			bRet = true;
		}

		m_importCount++;

		//If putting on the clipboard and there are multiple
		//then append cf_text and cf_unicodetext
		if (bPutOnClipboard)
		{
			Append_CF_TEXT_AND_CF_UNICODETEXT(csCF_TEXT, csCF_UNICODETEXT);
		}

		q.nextRow();
	}

	if (bRet)
	{
		FinishImport(bAddToDB, bPutOnClipboard, csCF_TEXT, csCF_UNICODETEXT);
	}

	return bRet;
}

bool CClip_ImportExport::ImportRow(CppSQLite3DB& db, CppSQLite3Query& q, bool bAddToDB, bool bPutOnClipboard)
{
	int nVersion = q.getIntField(_T("lVersion"));
	if (nVersion != 1 || !ImportFromSqliteV1(db, q))
	{
		return false;
	}

	if (bAddToDB)
	{
		MakeLatestOrder();
		AddToDB(true);
		return true;
	}

	return bPutOnClipboard;
}

void CClip_ImportExport::FinishImport(bool bAddToDB, bool bPutOnClipboard, CStringA& csCF_TEXT, CStringW& csCF_UNICODETEXT)
{
	if (bAddToDB)
	{
		Context().Windows().RefreshView();
	}
	else if (m_importCount == 1 && bPutOnClipboard)
	{
		PlaceFormatsOnclipboard();
	}
	else if (bPutOnClipboard)
	{
		PlaceCF_TEXT_AND_CF_UNICODETEXT_OnClipboard(csCF_TEXT, csCF_UNICODETEXT);
	}
}

bool CClip_ImportExport::PlaceCF_TEXT_AND_CF_UNICODETEXT_OnClipboard(CStringA& csCF_TEXT, CStringW& csCF_UNICODETEXT)
{
	bool bRet = false;

	if (OpenClipboard(Context().Windows().MainHwnd()))
	{
		EmptyClipboard();

		if (csCF_TEXT.IsEmpty() == FALSE)
		{
			long lLen = csCF_TEXT.GetLength();
			HGLOBAL hGlobal = CGlobalMemory::NewGlobalP(csCF_TEXT.GetBuffer(lLen), lLen + 1);
			csCF_TEXT.ReleaseBuffer();
			SetClipboardData(CF_TEXT, hGlobal);

			bRet = true;
		}
		if (csCF_UNICODETEXT.IsEmpty() == FALSE)
		{
			long lLen = csCF_UNICODETEXT.GetLength() * sizeof(wchar_t);
			// with a whole wide terminator; upstream added 1 byte, half of one
			HGLOBAL hGlobal = CGlobalMemory::NewGlobalP(csCF_UNICODETEXT.GetBuffer(lLen), lLen + sizeof(wchar_t));
			csCF_UNICODETEXT.ReleaseBuffer();
			SetClipboardData(CF_UNICODETEXT, hGlobal);

			bRet = true;
		}

		CloseClipboard();
	}
	else
	{
		CLogger::Log(_T("Error opening clipboard"));
	}

	return bRet;
}

bool CClip_ImportExport::PlaceFormatsOnclipboard()
{
	bool bRet = false;

	if (OpenClipboard(Context().Windows().MainHwnd()))
	{
		EmptyClipboard();

		INT_PTR formatCount = m_Formats.GetSize();
		for (int i = 0; i < formatCount; i++)
		{
			CClipFormat* pCF;
			pCF = &m_Formats.ElementAt(i);
			LPVOID Data = (LPVOID)GlobalLock(pCF->m_hgData);
			if (Data)
			{
				HGLOBAL hGlobal = CGlobalMemory::NewGlobalP(Data, GlobalSize(pCF->m_hgData));
				if (hGlobal)
				{
					SetClipboardData(pCF->m_cfType, hGlobal);
				}

				GlobalUnlock(pCF->m_hgData);
			}
		}

		CloseClipboard();

		bRet = true;
	}
	else
	{
		CLogger::Log(_T("PlaceFormatsOnclipboard::Error opening clipboard"));
	}

	return bRet;
}

// A CppSQLite3Exception propagates to the import boundary (see ImportFromSqliteDB).
bool CClip_ImportExport::ImportFromSqliteV1(CppSQLite3DB& db, CppSQLite3Query& qMain)
{
	//Load the Main Table
	m_Desc = qMain.getStringField(_T("mText"));
	long lID = qMain.getIntField(_T("lID"));

	//Load the data Table
	CClipFormat cf;
	m_Formats.RemoveAll();

	CString csSQL;
	csSQL.Format(
		_T("SELECT Data.* FROM Data ")
		_T("INNER JOIN Main ON Main.lID = Data.lParentID ")
		_T("WHERE Main.lID = %d ORDER BY Data.lID desc"), lID);

	CppSQLite3Query qData = db.execQuery(csSQL);
	while (qData.eof() == false)
	{
		cf.m_cfType = CClipboardFormats::GetFormatID(qData.getStringField(_T("strClipBoardFormat")));
		const long long originalSize = qData.getInt64Field(_T("lOriginalSize"));

		int nDataLen = 0;
		const unsigned char* cData = qData.getBlobField(_T("ooData"), nDataLen);
		if (cData == NULL || nDataLen < 0)
		{
			throw DittoCore::ClipboardFormatError("the exported clip has a format without data");
		}
		// the size comes from the file: DtoCodec checks it before allocating
		const std::vector<std::byte> data = DittoCore::DtoCodec::Uncompress(
			std::span(reinterpret_cast<const std::byte*>(cData), static_cast<std::size_t>(nDataLen)), originalSize);
		cf.m_hgData = CGlobalMemory::NewGlobalP(const_cast<std::byte*>(data.data()), data.size());
		if (cf.m_hgData == NULL)
		{
			throw DittoCore::ClipboardFormatError("no memory for an imported format of " + std::to_string(data.size()) + " bytes");
		}
		m_Formats.Add(cf);
		cf.m_hgData = NULL; //m_format owns m_hgData now

		qData.nextRow();
	}

	return m_Formats.GetSize() > 0;
}

bool CClip_ImportExport::Append_CF_TEXT_AND_CF_UNICODETEXT(CStringA& csCF_TEXT, CStringW& csCF_UNICODETEXT)
{
	bool bRet = false;
	CClipFormat* pCF;
	INT_PTR count = m_Formats.GetSize();
	for (int i = 0; i < count; i++)
	{
		pCF = &m_Formats.ElementAt(i);

		switch (pCF->m_cfType)
		{
		case CF_TEXT:
		{

			if (csCF_TEXT.IsEmpty() == FALSE)
				csCF_TEXT += "\r\n";

			csCF_TEXT += pCF->GetAsCStringA();
			bRet = true;
		}
		break;
		case CF_UNICODETEXT:
		{
			if (csCF_UNICODETEXT.IsEmpty() == FALSE)
				csCF_UNICODETEXT += _T("\r\n");

			csCF_UNICODETEXT += pCF->GetAsCString();;
			bRet = true;
		}
		break;
		}
	}

	return bRet;
}