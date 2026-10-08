#include "stdafx.h"
#include "CP_Main.h" // the clip and database types in the order they need
#include "ClipDataReader.h"
#include "ErrorReport.h"

CClipDataReader::CClipDataReader(CDittoDb& database) :
	m_database(database)
{
}

BOOL CClipDataReader::GetClipData(long parentId, CClipFormat &Clip)
{
	BOOL bRet = FALSE;

	try
	{
		CppSQLite3Query q = m_database.execQueryEx(_T("SELECT ooData FROM Data WHERE lParentID = %d AND strClipboardFormat = '%s'"), parentId, CClipboardFormats::GetFormatName(Clip.m_cfType).GetString());
		if(q.eof() == false)
		{
			int nDataLen = 0;
			const unsigned char *cData = q.getBlobField(_T("ooData"), nDataLen);
			if(cData != NULL)
			{
				Clip.m_hgData = CGlobalMemory::NewGlobal(nDataLen);

				CGlobalMemory::CopyToGlobalHP(Clip.m_hgData, (LPVOID)cData, nDataLen);

				bRet = TRUE;
			}
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Loading the data of clip id %ld from the clip database failed: %s"), parentId, e.errorMessage()));
		return FALSE;
	}

	return bRet;
}

std::unique_ptr<CClipTypes> CClipDataReader::LoadTypesFromDB()
{
	std::unique_ptr<CClipTypes> pTypes{std::make_unique<CClipTypes>()};

	try
	{
		CppSQLite3Query q = m_database.execQuery(_T("SELECT TypeText FROM Types"));
		while(q.eof() == false)
		{
			pTypes->Add(CClipboardFormats::GetFormatID(q.getStringField(_T("TypeText"))));

			q.nextRow();
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Loading the clipboard types to save from the clip database failed: %s"), e.errorMessage()));
		return nullptr;
	}

	if(pTypes->GetSize() <= 0)
	{
		pTypes->Add(CF_TEXT);
		pTypes->Add(CClipboardFormats::GetFormatID(CF_RTF));
		pTypes->Add(CF_UNICODETEXT);
		pTypes->Add(CF_HDROP);
		pTypes->Add(CF_DIB);
		pTypes->Add(CClipboardFormats::GetFormatID(_T("HTML Format")));
		pTypes->Add(CClipboardFormats::GetFormatID(_T("PNG")));
	}

	return pTypes;
}
