#pragma once
#include "rulerricheditctrl\rulerricheditctrl.h"

class CDittoRulerRichEditCtrl : public CRulerRichEditCtrl
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

	enum eSaveTypes
	{
		stNONE = 0x1,
		stCF_TEXT = 0x2,
		stCF_UNICODETEXT = 0x4,
		stRTF = 0x8
	};

	bool LoadItem(long lID, CString csDesc);
	int SaveToDB(BOOL bUpdateDesc);
	bool CloseEdit(bool bPrompt, BOOL bUpdateDesc);
	long GetDBID() { return m_lID; }
	CString GetDesc() { return m_csDescription; }

protected:
	long m_lID;
	CString m_csDescription;

	bool LoadRTFData(CClip& Clip);
	bool LoadTextData(CClip& Clip);

private:
	/** @brief Result of saving the loaded formats of the editor to the database. */
	enum class SaveClipResult
	{
		/** @brief The clip was saved (updated or added). */
		Saved,
		/** @brief The properties dialog of a new clip was cancelled; nothing was saved. */
		Cancelled,
		/** @brief The save failed (already shown to the user); nothing was saved. */
		Failed,
	};

	/** @brief Saves the clip: updates the edited clip, or adds a new one (see AddNewClip).
	 *  @param Clip Clip with the formats to save.
	 *  @param bUpdateDesc Whether to update the description; set to TRUE when a new clip was added.
	 *  @return The result of the save. */
	SaveClipResult SaveClip(CClip& Clip, BOOL& bUpdateDesc);

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
	 *  @return Saved if the clip was added (the edit is then no longer modified), Cancelled if the
	 *          dialog was cancelled, Failed if the order or the add failed (shown to the user). */
	SaveClipResult AddNewClip(CClip& Clip, BOOL& bUpdateDesc);
};
