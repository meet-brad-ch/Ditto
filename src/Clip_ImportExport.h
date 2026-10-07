#pragma once
#include "clip.h"

class CClip_ImportExport :	public CClip
{
public:
	CClip_ImportExport(void);
	~CClip_ImportExport(void);

	bool ExportToSqliteDB(CppSQLite3DB &m_db);
	bool ImportFromSqliteDB(CppSQLite3DB &db, bool bAddToDB, bool bPutOnClipboard);
	
	int m_importCount;

protected:
	bool ImportFromSqliteV1(CppSQLite3DB &db, CppSQLite3Query &qMain);
	bool Append_CF_TEXT_AND_CF_UNICODETEXT(CStringA &csCF_TEXT, CStringW &csCF_UNICODETEXT);

	bool PlaceFormatsOnclipboard();
	bool PlaceCF_TEXT_AND_CF_UNICODETEXT_OnClipboard(CStringA &csCF_TEXT, CStringW &csCF_UNICODETEXT);

private:
	/** @brief The format version that ExportToSqliteDB writes into the export's Main table. */
	static constexpr int s_currentExportVersion{1};

	/**
	 * @brief ImportFromSqliteDB's row step: imports a version-1 row and adds it to the database when asked.
	 * @param db The export database.
	 * @param q The query, on the row to import.
	 * @param bAddToDB Add the imported clip to the database.
	 * @param bPutOnClipboard The import is to be put on the clipboard.
	 * @return True when the row was imported and is to be added or put on the clipboard.
	 */
	bool ImportRow(CppSQLite3DB &db, CppSQLite3Query &q, bool bAddToDB, bool bPutOnClipboard);

	/**
	 * @brief ImportFromSqliteDB's last step after an import: refreshes the view, or puts the clips on the clipboard.
	 * @param bAddToDB The clips were added to the database.
	 * @param bPutOnClipboard The import is to be put on the clipboard.
	 * @param csCF_TEXT The joined CF_TEXT of all imported clips.
	 * @param csCF_UNICODETEXT The joined CF_UNICODETEXT of all imported clips.
	 */
	void FinishImport(bool bAddToDB, bool bPutOnClipboard, CStringA &csCF_TEXT, CStringW &csCF_UNICODETEXT);
};
