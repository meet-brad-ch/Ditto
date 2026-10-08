#pragma once

#include "DittoAddin.h"
#include <memory>
#include <vector>
#include <afxtempl.h>

class CAppWindows;
class CGetSetOptions;
class CMultiLanguage;

class CDittoAddins
{
public:
	/**
	 * @brief Creates the (empty) add-in list.
	 * @param settings The application's settings (add-in folder, database path); must outlive this object.
	 * @param language The UI texts (menu text, language code for the add-ins); must outlive this object.
	 * @param windows The application's windows (the quick paste window for the add-ins); must outlive this object.
	 */
	CDittoAddins(CGetSetOptions& settings, CMultiLanguage& language, CAppWindows& windows);
	~CDittoAddins(void);

	CDittoAddins(const CDittoAddins&) = delete;
	CDittoAddins& operator=(const CDittoAddins&) = delete;

	bool LoadAll();
	bool UnloadAll();

	bool Loaded() { return m_Addins.size() > 0; }

	bool AddPrePasteAddinsToMenu(CMenu* pMenu);
	bool CallPrePasteFunction(int Id, IClip* pClip);
	void AboutScreenText(CStringArray& arr);

protected:
	// The loaded addins; this object owns them
	std::vector<std::unique_ptr<CDittoAddin>> m_Addins;

	class CFunctionLookup
	{
	public:
		// One of m_Addins (not owned)
		CDittoAddin* m_pAddin{};
		CStringA m_csFunctionName;
	};

	CMap<int, int, CFunctionLookup, CFunctionLookup> m_FunctionMap;

protected:
	void LoadDittoInfo(CDittoInfo& DittoInfo);

	/// The application's settings (not owned).
	CGetSetOptions& m_settings;
	/// The UI texts (not owned).
	CMultiLanguage& m_language;
	/// The application's windows (not owned).
	CAppWindows& m_windows;
};
