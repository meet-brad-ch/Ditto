#pragma once

#include "..\Shared\ArrayEx.h"
#include <memory>
#include <vector>

class CHotKey
{
public:
	enum HotKeyType
	{ 
		PASTE_OPEN_CLIP,
		MOVE_TO_GROUP
	};

	CString	m_Name;
	CString m_description;
	ATOM	m_Atom;
	DWORD	m_Key; //704 is ctrl-tilda
	bool	m_bIsRegistered;
	bool	m_bUnRegisterOnShowDitto;
	int		m_clipId;
	int		m_globalId;
	HotKeyType m_hkType;
	static int m_nextId;
	
	// Create hot keys through CHotKeys::Create: the registry owns every hot key.
	CHotKey( CString name, DWORD defKey = 0, bool bUnregOnShowDitto = false, HotKeyType hkType = PASTE_OPEN_CLIP, CString description = _T(""));
	~CHotKey();
	CHotKey(const CHotKey&) = delete; // the hot key owns its global atom
	CHotKey& operator=(const CHotKey&) = delete;

	bool	IsRegistered() { return m_bIsRegistered; }
	CString GetName()      { return m_Name; }
	DWORD   GetKey()       { return m_Key; }
	CString GetHotKeyDisplay();
	
	void SetKey( DWORD key, bool bSave = false );
	// profile
	void LoadKey();
	bool SaveKey();

	void CopyFromCtrl(CHotKeyCtrl& ctrl, HWND hParent, int nWindowsCBID);
	void CopyToCtrl(CHotKeyCtrl& ctrl, HWND hParent, int nWindowsCBID);

	UINT GetModifier() { return GetModifier(HIBYTE(m_Key)); }

	bool Register();
	bool Unregister(bool bOnShowingDitto = false);

	static BOOL ValidateHotKey(DWORD dwHotKey);
	static UINT GetModifier(DWORD dwHotKey);
	static CString GetHotKeyDisplayStatic(DWORD dwHotKey);
	static CString GetVirKeyName(unsigned int virtualKey);
};


/*------------------------------------------------------------------*\
	CHotKeys - Manages system-wide hotkeys
\*------------------------------------------------------------------*/

/**
 * @brief The registry of hot keys; it owns every CHotKey it holds.
 */
class CHotKeys
{
public:
	HWND	m_hWnd;

	CHotKeys();
	~CHotKeys();

	void Init( HWND hWnd ) { m_hWnd = hWnd; }

	/**
	 * @brief Creates a hot key and stores it in the registry, which owns it.
	 * @param name The profile name of the hot key.
	 * @param defKey The key used when the profile holds none.
	 * @param bUnregOnShowDitto True: the key is unregistered while Ditto's window shows.
	 * @param hkType What the hot key does.
	 * @param description The text shown for the hot key.
	 * @return The new hot key; it stays valid until the registry removes it.
	 */
	CHotKey& Create(CString name, DWORD defKey = 0, bool bUnregOnShowDitto = false, CHotKey::HotKeyType hkType = CHotKey::PASTE_OPEN_CLIP, CString description = _T(""));

	/**
	 * @brief The number of hot keys in the registry.
	 * @return The count.
	 */
	INT_PTR GetSize() const { return static_cast<INT_PTR>(m_keys.size()); }
	/**
	 * @brief The number of hot keys in the registry (same as GetSize).
	 * @return The count.
	 */
	INT_PTR GetCount() const { return GetSize(); }
	/**
	 * @brief The hot key at an index; the registry keeps ownership.
	 * @param index A position from 0 to GetSize() - 1.
	 * @return The hot key, not owned by the caller.
	 */
	CHotKey* ElementAt(INT_PTR index) const { return m_keys.at(static_cast<size_t>(index)).get(); }
	/**
	 * @brief The hot key at an index; the registry keeps ownership.
	 * @param index A position from 0 to GetSize() - 1.
	 * @return The hot key, not owned by the caller.
	 */
	CHotKey* operator[](INT_PTR index) const { return ElementAt(index); }

	INT_PTR Find( CHotKey* pHotKey );
	/**
	 * @brief Removes a hot key from the registry and destroys it (the registry owns it).
	 * @param pHotKey The hot key to remove; it is invalid after the call when it was found.
	 * @return True when the hot key was in the registry.
	 */
	bool Remove( CHotKey* pHotKey );

	bool Remove(int clipId, CHotKey::HotKeyType hkType);

	BOOL ValidateClip(int clipId, DWORD key, CString desc, CHotKey::HotKeyType hkType);

	// profile load / save
	void LoadAllKeys();
	void SaveAllKeys();

	void RegisterAll(bool bMsgOnError = false);
	void UnregisterAll(bool bMsgOnError = false, bool bOnShowDitto = false);

	void GetKeys( ARRAY& keys );
	void SetKeys( ARRAY& keys, bool bSave = false ); // caution! this alters hotkeys based upon corresponding indexes

	static bool FindFirstConflict( ARRAY& keys, INT_PTR* pX = NULL, INT_PTR* pY = NULL );
	// if true, pX and pY (if valid) are set to the index of the conflicting hotkeys.
	bool FindFirstConflict( INT_PTR* pX = NULL, INT_PTR* pY = NULL );

private:
	/** @brief The hot keys, owned by the registry. */
	std::vector<std::unique_ptr<CHotKey>> m_keys{};
};

extern CHotKeys g_HotKeys;