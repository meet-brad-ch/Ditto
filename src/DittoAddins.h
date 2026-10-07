#pragma once

#include "DittoAddin.h"
#include <memory>
#include <vector>
#include <afxtempl.h>

class CDittoAddins
{
public:
	CDittoAddins(void);
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
};
