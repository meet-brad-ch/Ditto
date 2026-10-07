#pragma once
#include "rulerricheditctrl\rulerricheditctrl.h"

class CDittoRulerRichEditCtrl :	public CRulerRichEditCtrl
{
public:
	/** @brief SaveToDB's results besides FALSE (nothing saved). */
	enum : int
	{
		/** @brief The clip was saved to the database. */
		SavedClipToDb = 1,
		/** @brief The clip was not modified, so nothing was saved. */
		DidntNeedToSave = 2,
	};

	CDittoRulerRichEditCtrl(void);
	~CDittoRulerRichEditCtrl(void);

	enum eSaveTypes{stNONE = 0x1, stCF_TEXT = 0x2, stCF_UNICODETEXT = 0x4, stRTF = 0x8};

	bool LoadItem(long lID, CString csDesc);
	int SaveToDB(BOOL bUpdateDesc);
	long GetTypeFlags(long lID);
	bool CloseEdit(bool bPrompt, BOOL bUpdateDesc);
	long GetDBID()		{ return m_lID; }
	CString GetDesc()	{ return m_csDescription; }

	void d();
	    
protected:
	long m_lID;
	CString m_csDescription;

	bool LoadRTFData(CClip &Clip);
	bool LoadTextData(CClip &Clip);

private:
	// The eSaveTypes flags of the saved clip types: stRTF for rtf, stCF_TEXT | stCF_UNICODETEXT for text
	static int SaveTypesOf(CClipTypes& types);

	/** @brief Reads one clip format from the database.
	 *  @param lID Clip id.
	 *  @param Clip Format to read; m_cfType selects the format.
	 *  @return true if the format was read and has data. */
	static bool HasClipData(long lID, CClipFormat& Clip);

	/** @brief Adds the rtf and/or text of the editor to the clip, as the save types ask.
	 *  @param Clip Clip to add the formats to.
	 *  @param saveTypes eSaveTypes flags (see SaveTypesOf). */
	void LoadFormatsToSave(CClip& Clip, int saveTypes);

	/** @brief Shows the properties dialog for a new clip and adds the clip to the database on OK.
	 *  @param Clip The new clip.
	 *  @param bUpdateDesc Set to TRUE when the clip was added.
	 *  @return true if the clip was added (the edit is then no longer modified). */
	bool AddNewClip(CClip& Clip, BOOL& bUpdateDesc);
};
