#pragma once

#include "DittoAddin.h"
#include <memory>
#include <vector>
#include <afxtempl.h>

class CGetSetOptions;

class CDittoAddins
{
public:
	/**
	 * @brief Creates the (empty) add-in list.
	 * @param settings The application's settings (add-in folder, database path); must outlive this object.
	 */
	explicit CDittoAddins(CGetSetOptions& settings);
	~CDittoAddins(void);

	bool LoadAll();
	bool UnloadAll();

	bool Loaded()	{ return m_Addins.size() > 0; }

	bool AddPrePasteAddinsToMenu(CMenu *pMenu);
	bool CallPrePasteFunction(int Id, IClip *pClip);
	void AboutScreenText(CStringArray &arr);

protected:
	// The loaded addins; this object owns them
	std::vector<std::unique_ptr<CDittoAddin>> m_Addins;

	class CFunctionLookup
	{
	public:
		// One of m_Addins (not owned)
		CDittoAddin *m_pAddin{};
		CStringA m_csFunctionName;
	};

	CMap<int, int, CFunctionLookup, CFunctionLookup> m_FunctionMap;

protected:
	void LoadDittoInfo(CDittoInfo &DittoInfo);

	/// The application's settings (not owned).
	CGetSetOptions& m_settings;
};
