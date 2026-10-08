#pragma once

#include "..\Shared\ArrayEx.h"
#include <array>
#include <memory>
#include <vector>

class CGetSetOptions;

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
	
	/**
	 * @brief Creates a hot key and reads its key from the settings (create hot keys through
	 *        CHotKeys::Create: the registry owns every hot key).
	 * @param settings The application's settings (the stored key); must outlive the hot key.
	 * @param name The profile name of the hot key.
	 * @param defKey The key used when the profile holds none.
	 * @param bUnregOnShowDitto True: the key is unregistered while Ditto's window shows.
	 * @param hkType What the hot key does.
	 * @param description The text shown for the hot key.
	 */
	CHotKey( CGetSetOptions& settings, CString name, DWORD defKey = 0, bool bUnregOnShowDitto = false, HotKeyType hkType = PASTE_OPEN_CLIP, CString description = _T(""));
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

private:
	/// The application's settings (not owned); LoadKey and SaveKey read and write the key there.
	CGetSetOptions& m_settings;

	/** @brief The display name of a key that GetKeyNameText does not name. */
	struct NamedKey
	{
		/** @brief The virtual key code. */
		unsigned int virtualKey{};
		/** @brief The name shown for the key. */
		const TCHAR* name{};
	};

	/** @brief Friendly names of the multimedia and browser keys (GetKeyNameText returns none for them). */
	static constexpr std::array<NamedKey, 18> s_namedKeys{ {
		{ VK_VOLUME_MUTE, _T("Volume Mute") },
		{ VK_VOLUME_DOWN, _T("Volume Down") },
		{ VK_VOLUME_UP, _T("Volume Up") },
		{ VK_MEDIA_NEXT_TRACK, _T("Next Track") },
		{ VK_MEDIA_PREV_TRACK, _T("Prev Track") },
		{ VK_MEDIA_PLAY_PAUSE, _T("Play/Pause") },
		{ VK_MEDIA_STOP, _T("Stop") },
		{ VK_BROWSER_BACK, _T("Browser Back") },
		{ VK_BROWSER_FORWARD, _T("Browser Forward") },
		{ VK_BROWSER_REFRESH, _T("Browser Refresh") },
		{ VK_BROWSER_STOP, _T("Browser Stop") },
		{ VK_BROWSER_SEARCH, _T("Browser Search") },
		{ VK_BROWSER_FAVORITES, _T("Browser Favorites") },
		{ VK_BROWSER_HOME, _T("Browser Home") },
		{ VK_LAUNCH_MAIL, _T("Launch Mail") },
		{ VK_LAUNCH_MEDIA_SELECT, _T("Launch Media") },
		{ VK_LAUNCH_APP1, _T("Launch App1") },
		{ VK_LAUNCH_APP2, _T("Launch App2") },
	} };

	/** @brief The keys whose scan code needs the extended bit (MapVirtualKey strips it for them). */
	static constexpr std::array<unsigned int, 12> s_extendedKeys{
		VK_LEFT, VK_UP, VK_RIGHT, VK_DOWN, // arrow keys
		VK_PRIOR, VK_NEXT, // page up and page down
		VK_END, VK_HOME,
		VK_INSERT, VK_DELETE,
		VK_DIVIDE, // numpad slash
		VK_NUMLOCK };

	/**
	 * @brief Tells whether a key is one whose scan code needs the extended bit.
	 * @param virtualKey The virtual key code.
	 * @return True for an arrow, page, home/end, insert/delete, numpad slash or num lock key.
	 */
	static bool IsExtendedKey(unsigned int virtualKey);
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
	 * @param settings The application's settings (the stored key); must outlive the hot key.
	 * @param name The profile name of the hot key.
	 * @param defKey The key used when the profile holds none.
	 * @param bUnregOnShowDitto True: the key is unregistered while Ditto's window shows.
	 * @param hkType What the hot key does.
	 * @param description The text shown for the hot key.
	 * @return The new hot key; it stays valid until the registry removes it.
	 */
	CHotKey& Create(CGetSetOptions& settings, CString name, DWORD defKey = 0, bool bUnregOnShowDitto = false, CHotKey::HotKeyType hkType = CHotKey::PASTE_OPEN_CLIP, CString description = _T(""));

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

	/**
	 * @brief Finds the clip's hot key of a type (creates it when missing) and sets its key, name
	 *        and clip id.
	 * @param settings The application's settings (for a hot key that is created).
	 * @param clipId The clip.
	 * @param key The clip's shortcut.
	 * @param desc The name of the hot key.
	 * @param hkType What the hot key does.
	 * @return CHotKey::ValidateHotKey(key).
	 */
	BOOL ValidateClip(CGetSetOptions& settings, int clipId, DWORD key, CString desc, CHotKey::HotKeyType hkType);

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