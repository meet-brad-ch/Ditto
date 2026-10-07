#ifndef __SENDKEYS_04192004__INC__
#define __SENDKEYS_04192004__INC__

#include <windows.h>
#include <tchar.h>
#include <span>
// Please see SendKeys.cpp for copyright and usage issues.

class CSendKeys
{
private:
  bool m_bWait{}, m_bUsingParens{}, m_bShiftDown{}, m_bLShiftDown{}, m_bRShiftDown{}, m_bAltDown{}, m_bControlDown{}, m_bLControlDown{}, m_bRControlDown{}, m_bWinDown{};
  DWORD  m_nDelayAlways, m_nDelayNow, m_keyDownDelay;

  static BOOL CALLBACK enumwindowsProc(HWND hwnd, LPARAM lParam);
  void   CarryDelay();

  struct enumwindow_t
  {
    LPTSTR str;
    HWND hwnd;
  };

  struct key_desc_t
  {
    LPCTSTR keyName;
    BYTE VKey;
    bool normalkey; // a normal character or a VKEY ?
  };

  enum
  {
    MaxSendKeysRecs  = 98,
    MaxExtendedVKeys = 12
  };

  /*
  Reference: VkKeyScan() / MSDN
  Bit Meaning 
  --- --------
  1   Either SHIFT key is pressed. 
  2   Either CTRL key is pressed. 
  4   Either ALT key is pressed. 
  8   The Hankaku key is pressed 
  16  Reserved (defined by the keyboard layout driver). 
  32  Reserved (defined by the keyboard layout driver). 
  */
  static const WORD VKKEYSCANSHIFTON;
  static const WORD VKKEYSCANCTRLON;
  static const WORD VKKEYSCANALTON;
  static const WORD INVALIDKEY;

  /** @brief The key names {KEY} understands with their virtual keys, sorted by name (binary searched). */
  static const key_desc_t KeyNames[MaxSendKeysRecs];
  static const BYTE ExtendedVKeys[MaxExtendedVKeys];

  static bool BitSet(BYTE BitTable, UINT BitMask);

  void PopUpShiftKeys();

  /**
   * @brief Sends a key up for a modifier key when its "down" flag is set.
   * @param bDown the "down" flag of the modifier key.
   * @param VKey the virtual key to release.
   */
  void ReleaseKeyIfDown(bool bDown, BYTE VKey);

  /**
   * @brief Sends the key string character at pKey (a control character, a {...} group or a normal key).
   * @param pKey in: the current character; out: the last character consumed ('}' of a group).
   * @param KeyString the group buffer shared by all groups of one SendKeys() call.
   * @param NumTimes repeat count of the last group key; kept between groups.
   * @return false if a group is invalid (SendKeys() then stops and returns false).
   */
  bool SendKeysChar(LPTSTR &pKey, std::span<TCHAR> KeyString, WORD &NumTimes);

  /**
   * @brief Handles the modifier and control characters ( ) % + ^ @ ~ of a key string.
   * @param ch the key string character.
   * @return true if ch was a control character and was handled, false for a normal key.
   */
  bool SendControlChar(TCHAR ch);

  /**
   * @brief Copies a {...} group into KeyString, parses it and sends its key.
   * @param pKey in: the '{'; out: the character that ends the group.
   * @param KeyString the group buffer shared by all groups of one SendKeys() call.
   * @param NumTimes repeat count of the last group key; kept between groups.
   * @return false if the group is too long or has an invalid number.
   */
  bool SendKeyGroup(LPTSTR &pKey, std::span<TCHAR> KeyString, WORD &NumTimes);

  /**
   * @brief Parses a group text (VKEY, BEEP, APPACTIVATE, DELAY or a key name) and runs commands.
   * @param KeyString the group text without braces (BEEP edits it in place).
   * @param MKey in: INVALIDKEY; out: the key to send, if any.
   * @param NumTimes out: the repeat count of a key name with a count.
   * @return false if a VKEY or count number is out of the WORD range.
   */
  bool ParseKeyCommand(LPTSTR KeyString, WORD &MKey, WORD &NumTimes);

  /**
   * @brief Runs a {BEEP frequency delay} group.
   * @param KeyString the group text starting with "BEEP" (the space between the numbers is cut).
   */
  void BeepCommand(LPTSTR KeyString);

  /**
   * @brief Looks up a key name (with an optional repeat count) in KeyNames.
   * @param KeyString the group text.
   * @param MKey out: the key, INVALIDKEY if the name is unknown.
   * @param NumTimes out: the repeat count (1 when none is given); unchanged for an unknown name.
   * @return false if the repeat count is out of the WORD range.
   */
  bool ParseKeyName(LPCTSTR KeyString, WORD &MKey, WORD &NumTimes);

  static bool IsVkExtended(BYTE VKey);
  void SendKeyUp(BYTE VKey);
  void SendKeyDown(BYTE VKey, WORD NumTimes, bool GenUpMsg, bool bDelay = false);
  void SendKey(WORD MKey, WORD NumTimes, bool GenDownMsg);
  /**
   * @brief Sends the key parsed from a {...} group; INVALIDKEY sends nothing.
   * @param MKey the key: a left/right CTRL or SHIFT is held down, any other key is pressed.
   * @param NumTimes how often a pressed key is sent.
   */
  void SendSpecialKey(WORD MKey, WORD NumTimes);
  static WORD StringToVKey(LPCTSTR KeyString, int &idx);
  void KeyboardEvent(BYTE VKey, BYTE ScanCode, LONG Flags);

public:

  void AllKeysUp();
  bool SendKeys(LPCTSTR KeysString, bool Wait = false);
  static bool AppActivate(HWND wnd);
  static bool AppActivate(LPCTSTR WindowTitle, LPCTSTR WindowClass = 0);
  void SetDelay(const DWORD delay) { m_nDelayAlways = delay; }
  void SetKeyDownDelay(const DWORD delay) { m_keyDownDelay = delay; }
  static CString VkString(BYTE VKey);
  CSendKeys();
};

#endif