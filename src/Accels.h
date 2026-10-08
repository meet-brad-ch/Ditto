#pragma once

#include <map>
#include "..\Shared\ArrayEx.h"

using namespace std;

class CAccel
{
public:
    DWORD Key;
	DWORD Key2;
    DWORD Cmd;
	int RefId;

    CAccel(DWORD key = 0, DWORD cmd = 0, DWORD key2 = 0)
    {
        Key = key;
		Key2 = key2;
        Cmd = cmd;
		RefId = 0;
    }
};

/*------------------------------------------------------------------*\
CAccels - Manages a set of CAccel
\*------------------------------------------------------------------*/
class CAccels
{
public:
	/**
	 * @brief Makes an accelerator key: the modifiers in the high byte, the virtual key in the low byte.
	 * @param vkey The virtual key code.
	 * @param mod The HOTKEYF_* modifiers.
	 * @return (mod << 8) | vkey.
	 */
	static constexpr int MakeKey(int vkey, int mod)
	{
		return (mod << 8) | vkey;
	}

    CAccels();

    void AddAccel(CAccel a);

	void AddAccel(DWORD cmd, DWORD key, DWORD key2 = 0);

	void RemoveAll();

	CString GetCmdKeyText(DWORD cmd);

    /**
     * @brief Handles a key's first WM_KEYDOWN or WM_SYSKEYDOWN message; uses GetKeyState to test for modifiers.
     * @param pMsg The message.
     * @param a Receives the matching accelerator.
     * @param doubleKeyStrokeTimeout The settings' double keystroke timeout (ms): how long a two-key shortcut waits for its second key.
     * @return True when a shortcut matched.
     */
    bool OnMsg(MSG *pMsg, CAccel &a, int doubleKeyStrokeTimeout);

	bool ContainsKey(int vKey);

	bool m_handleRepeatKeys;
	bool m_checkModifierKeys;

    static BYTE GetKeyStateModifiers();

protected:

	multimap<DWORD, CAccel> m_multiMap;
	DWORD m_activeFirstKey;

	ULONGLONG m_firstMapTick;

private:
	/**
	 * @brief Tells whether the first key of a two-key shortcut was pressed within the double keystroke timeout.
	 * @param doubleKeyStrokeTimeout The double keystroke timeout (ms).
	 * @return True while OnMsg waits for the second key.
	 */
	bool IsSecondKeyPending(int doubleKeyStrokeTimeout) const;

	/**
	 * @brief OnMsg's second-key step: finds the shortcut whose second key is key after the pending first key.
	 * @param key The pressed key (MakeKey of virtual key and modifiers).
	 * @param a Receives the matching accelerator.
	 * @return True when a shortcut matched (the pending first key is then cleared).
	 */
	bool MatchSecondKey(DWORD key, CAccel &a);

	/**
	 * @brief OnMsg's first-key step: finds the single-key shortcut of key, or starts waiting for a second key.
	 * @param key The pressed key (MakeKey of virtual key and modifiers).
	 * @param a Receives the matching single-key accelerator.
	 * @return True when a single-key shortcut matched and no two-key shortcut starts with key.
	 */
	bool MatchFirstKey(DWORD key, CAccel &a);
};
