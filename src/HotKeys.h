#pragma once

#include "..\Shared\ArrayEx.h"
#include <array>
#include <memory>
#include <vector>

class CGetSetOptions;
class CHotKeys;

class CHotKey
{
public:
	enum HotKeyType
	{
		PASTE_OPEN_CLIP,
		MOVE_TO_GROUP
	};

	CString m_Name;
	CString m_description;
	ATOM m_Atom;
	DWORD m_Key; //704 is ctrl-tilda
	bool m_bIsRegistered;
	bool m_bUnRegisterOnShowDitto;
	int m_clipId;
	int m_globalId;
	HotKeyType m_hkType;

	/**
	 * @brief Creates a hot key and reads its key from the settings (create hot keys through
	 *        CHotKeys::Create: the registry owns every hot key).
	 * @param registry The registry that owns the hot key (the window the key is registered for).
	 * @param settings The application's settings (the stored key); must outlive the hot key.
	 * @param globalId The hot key's number, given by the registry in creation order.
	 * @param name The profile name of the hot key.
	 * @param defKey The key used when the profile holds none.
	 * @param bUnregOnShowDitto True: the key is unregistered while Ditto's window shows.
	 * @param hkType What the hot key does.
	 * @param description The text shown for the hot key.
	 */
	CHotKey(const CHotKeys& registry, CGetSetOptions& settings, int globalId, CString name, DWORD defKey = 0, bool bUnregOnShowDitto = false, HotKeyType hkType = PASTE_OPEN_CLIP, CString description = _T(""));
	~CHotKey();
	CHotKey(const CHotKey&) = delete; // the hot key owns its global atom
	CHotKey& operator=(const CHotKey&) = delete;

	bool IsRegistered() { return m_bIsRegistered; }
	CString GetName() { return m_Name; }
	DWORD GetKey() { return m_Key; }
	CString GetHotKeyDisplay();

	void SetKey(DWORD key, bool bSave = false);
	// profile
	void LoadKey();
	bool SaveKey();

	void CopyFromCtrl(CHotKeyCtrl& ctrl, HWND hParent, int nWindowsCBID);
	void CopyToCtrl(CHotKeyCtrl& ctrl, HWND hParent, int nWindowsCBID);

	UINT GetModifier() { return GetModifier(HIBYTE(m_Key)); }

	bool Register();
	bool Unregister(bool bOnShowingDitto = false);

	static UINT GetModifier(DWORD dwHotKey);
	static CString GetHotKeyDisplayStatic(DWORD dwHotKey);
	static CString GetVirKeyName(unsigned int virtualKey);

private:
	/// The registry that owns this hot key (not owned); Register and Unregister use its window.
	const CHotKeys& m_registry;
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
		VK_PRIOR, VK_NEXT,                 // page up and page down
		VK_END, VK_HOME,
		VK_INSERT, VK_DELETE,
		VK_DIVIDE, // numpad slash
		VK_NUMLOCK
	};

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
 * @brief The registry of hot keys; it owns every CHotKey it holds, numbers them in creation order
 *        and holds the named (application) hot keys. Owned by CAppServices (HotKeys()).
 */
class CHotKeys
{
public:
	/** @brief The named hot keys of the application (CreateNamed creates them in this order). */
	enum class Id : int
	{
		DittoHotKey,          ///< shows Ditto's quick paste window
		DittoHotKey2,         ///< shows Ditto's quick paste window
		DittoHotKey3,         ///< shows Ditto's quick paste window
		PosOne,               ///< pastes the clip at position 1
		PosTwo,               ///< pastes the clip at position 2
		PosThree,             ///< pastes the clip at position 3
		PosFour,              ///< pastes the clip at position 4
		PosFive,              ///< pastes the clip at position 5
		PosSix,               ///< pastes the clip at position 6
		PosSeven,             ///< pastes the clip at position 7
		PosEight,             ///< pastes the clip at position 8
		PosNine,              ///< pastes the clip at position 9
		PosTen,               ///< pastes the clip at position 10
		CopyBuffer1,          ///< copies into copy buffer 1
		PasteBuffer1,         ///< pastes copy buffer 1
		CutBuffer1,           ///< cuts into copy buffer 1
		CopyBuffer2,          ///< copies into copy buffer 2
		PasteBuffer2,         ///< pastes copy buffer 2
		CutBuffer2,           ///< cuts into copy buffer 2
		CopyBuffer3,          ///< copies into copy buffer 3
		PasteBuffer3,         ///< pastes copy buffer 3
		CutBuffer3,           ///< cuts into copy buffer 3
		CopyBuffer4,          ///< copies into copy buffer 4
		PasteBuffer4,         ///< pastes copy buffer 4
		CutBuffer4,           ///< cuts into copy buffer 4
		CopyBuffer5,          ///< copies into copy buffer 5
		PasteBuffer5,         ///< pastes copy buffer 5
		CutBuffer5,           ///< cuts into copy buffer 5
		TextOnlyPaste,        ///< pastes the clipboard as text only
		SaveClipboard,        ///< saves the current clipboard
		CopyAndSaveClipboard, ///< copies and saves the clipboard
		Count                 ///< the number of named hot keys (not a hot key)
	};

	/**
	 * @brief Creates the empty registry.
	 * @param settings The application's settings (the stored keys); must outlive the registry.
	 */
	explicit CHotKeys(CGetSetOptions& settings);
	~CHotKeys();
	CHotKeys(const CHotKeys&) = delete; // the registry owns its hot keys
	CHotKeys& operator=(const CHotKeys&) = delete;

	/**
	 * @brief Sets the window the hot keys are registered for (WM_HOTKEY goes there).
	 * @param hWnd The window.
	 */
	void Init(HWND hWnd) { m_hWnd = hWnd; }

	/**
	 * @brief The window the hot keys are registered for.
	 * @return The window set by Init, or NULL.
	 */
	HWND Window() const { return m_hWnd; }

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

	/** @brief Creates the named hot keys (Id), in the order of Id, with their profile names and defaults. */
	void CreateNamed();

	/**
	 * @brief A named hot key.
	 * @param id The hot key (not Id::Count).
	 * @return The hot key (owned by the registry), or null before CreateNamed.
	 */
	CHotKey* Named(Id id) const;

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

	INT_PTR Find(CHotKey* pHotKey);
	/**
	 * @brief Removes a hot key from the registry and destroys it (the registry owns it).
	 * @param pHotKey The hot key to remove; it is invalid after the call when it was found.
	 * @return True when the hot key was in the registry.
	 */
	bool Remove(CHotKey* pHotKey);

	bool Remove(int clipId, CHotKey::HotKeyType hkType);

	/**
	 * @brief Finds the clip's hot key of a type (creates it when missing) and sets its key, name
	 *        and clip id.
	 * @param clipId The clip.
	 * @param key The clip's shortcut.
	 * @param desc The name of the hot key.
	 * @param hkType What the hot key does.
	 * @return ValidateHotKey(key).
	 */
	BOOL ValidateClip(int clipId, DWORD key, CString desc, CHotKey::HotKeyType hkType);

	/**
	 * @brief Tells whether a key can be registered as a hot key now (registers and unregisters a test key).
	 * @param dwHotKey The key (virtual key in the low byte, HOTKEYF_ modifiers in the high byte).
	 * @return TRUE when RegisterHotKey took it.
	 */
	BOOL ValidateHotKey(DWORD dwHotKey) const;

	// profile load / save
	void LoadAllKeys();
	void SaveAllKeys();

	void RegisterAll(bool bMsgOnError = false);
	void UnregisterAll(bool bMsgOnError = false, bool bOnShowDitto = false);

	void GetKeys(ARRAY& keys);
	void SetKeys(ARRAY& keys, bool bSave = false); // caution! this alters hotkeys based upon corresponding indexes

	static bool FindFirstConflict(ARRAY& keys, INT_PTR* pX = NULL, INT_PTR* pY = NULL);
	// if true, pX and pY (if valid) are set to the index of the conflicting hotkeys.
	bool FindFirstConflict(INT_PTR* pX = NULL, INT_PTR* pY = NULL);

private:
	/** @brief The profile name and the defaults of a named hot key. */
	struct NamedHotKeySpec
	{
		/** @brief The named hot key. */
		Id id{};
		/** @brief The profile name. */
		const TCHAR* name{};
		/** @brief The key used when the profile holds none. */
		DWORD defaultKey{};
		/** @brief True: the key is unregistered while Ditto's window shows. */
		bool unregisterOnShowDitto{};
	};

	/** @brief The number of named hot keys. */
	static constexpr size_t s_namedCount{ static_cast<size_t>(Id::Count) };

	/** @brief The named hot keys in creation order (the order CCP_MainApp::AfterMainCreate always used). */
	static constexpr std::array<NamedHotKeySpec, s_namedCount> s_namedHotKeys{ {
		{ Id::DittoHotKey, _T("DittoHotKey"), 704, false }, //704 is ctrl-tilda
		{ Id::DittoHotKey2, _T("DittoHotKey2"), 0, false },
		{ Id::DittoHotKey3, _T("DittoHotKey3"), 0, false },
		{ Id::PosOne, _T("Position1"), 0, true },
		{ Id::PosTwo, _T("Position2"), 0, true },
		{ Id::PosThree, _T("Position3"), 0, true },
		{ Id::PosFour, _T("Position4"), 0, true },
		{ Id::PosFive, _T("Position5"), 0, true },
		{ Id::PosSix, _T("Position6"), 0, true },
		{ Id::PosSeven, _T("Position7"), 0, true },
		{ Id::PosEight, _T("Position8"), 0, true },
		{ Id::PosNine, _T("Position9"), 0, true },
		{ Id::PosTen, _T("Position10"), 0, true },
		{ Id::CopyBuffer1, _T("CopyBufferCopyHotKey_0"), 0, true },
		{ Id::PasteBuffer1, _T("CopyBufferPasteHotKey_0"), 0, true },
		{ Id::CutBuffer1, _T("CopyBufferCutHotKey_0"), 0, true },
		{ Id::CopyBuffer2, _T("CopyBufferCopyHotKey_1"), 0, true },
		{ Id::PasteBuffer2, _T("CopyBufferPasteHotKey_1"), 0, true },
		{ Id::CutBuffer2, _T("CopyBufferCutHotKey_1"), 0, true },
		{ Id::CopyBuffer3, _T("CopyBufferCopyHotKey_2"), 0, true },
		{ Id::PasteBuffer3, _T("CopyBufferPasteHotKey_2"), 0, true },
		{ Id::CutBuffer3, _T("CopyBufferCutHotKey_2"), 0, true },
		{ Id::CopyBuffer4, _T("CopyBufferCopyHotKey_3"), 0, true },
		{ Id::PasteBuffer4, _T("CopyBufferPasteHotKey_3"), 0, true },
		{ Id::CutBuffer4, _T("CopyBufferCutHotKey_3"), 0, true },
		{ Id::CopyBuffer5, _T("CopyBufferCopyHotKey_4"), 0, true },
		{ Id::PasteBuffer5, _T("CopyBufferPasteHotKey_4"), 0, true },
		{ Id::CutBuffer5, _T("CopyBufferCutHotKey_4"), 0, true },
		{ Id::TextOnlyPaste, _T("TextOnlyPaste"), 0, true },
		{ Id::SaveClipboard, _T("SaveClipboard"), 0, false },
		{ Id::CopyAndSaveClipboard, _T("CopyAndSaveClipboard"), 0, false },
	} };

	/** @brief The application's settings (not owned); every hot key reads its key there. */
	CGetSetOptions& m_settings;
	/** @brief The window the hot keys are registered for (WM_HOTKEY goes there). */
	HWND m_hWnd{};
	/** @brief The number the next created hot key gets (CHotKey::m_globalId). */
	int m_nextId{};
	/** @brief The named hot keys (owned by m_keys); null before CreateNamed. */
	std::array<CHotKey*, s_namedCount> m_named{};
	/** @brief The hot keys, owned by the registry. */
	std::vector<std::unique_ptr<CHotKey>> m_keys{};
};
