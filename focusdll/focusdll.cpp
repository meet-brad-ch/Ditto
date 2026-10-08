#include <windows.h>
#include <assert.h>
#include "focusdll.h"

#pragma data_seg(".shared")
HHOOK hHook = NULL;           //
HWND hFocusWnd = NULL;        // window that last gained the focus
HWND hNotifyWnd = NULL;       // window to send message to when focus changes
UINT uMessage = 0;            // wm_message to send in the above case
HHOOK g_hKeyboardHook = NULL; //
bool g_CaptureKeys = FALSE;
UINT g_uKeyboardMessage = 0;
HWND g_hKeyboardNotifyWnd = NULL;
#pragma data_seg()
#pragma comment(linker, "/SECTION:.shared,RWS")

/** @brief focus.dll's own module (its handle is per process, so it is not in the shared segment). */
class CFocusDllModule
{
public:
	/**
	 * @brief The module handle of focus.dll (the module that contains this code), for SetWindowsHookEx.
	 * @return The handle DllMain receives as hInstance; NULL if Windows cannot resolve it (SetWindowsHookEx then fails).
	 */
	static HINSTANCE Handle()
	{
		HMODULE module{};
		if (!::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
								  reinterpret_cast<LPCWSTR>(&CFocusDllModule::Handle), &module))
		{
			return NULL;
		}
		return module;
	}
};

/** @brief The kinds of keyboard event that FromLParam reads from a key message's lParam. */
class KeyEventType
{
public:
	/** @brief The event kinds, mutually exclusive (FromLParam returns exactly one; used only in this file). */
	enum : BYTE
	{
		/** @brief Key-down event. */
		KeyDown = 1,
		/** @brief Key-up event. */
		KeyUp = 2,
		/** @brief Key-repeat event: the key is held down for long enough. */
		KeyRepeat = 3,
	};

	/**
	 * @brief Reads the kind of keyboard event from a key message's lParam (bits 31 and 30, see WM_KEYDOWN).
	 * @param lParam The key message's lParam.
	 * @return KeyUp, KeyRepeat or KeyDown.
	 */
	static BYTE FromLParam(LPARAM lParam)
	{
		// Reference: WM_KEYDOWN on MSDN
		if (lParam & 0x80000000) // check bit 31 for up/down
		{
			return KeyUp;
		}
		else
		{
			if (lParam & 0x40000000) // check bit 30 for previous up/down
				return KeyRepeat;    // It was pressed down before this key-down event, so it's a key-repeat for sure
			else
				return KeyDown;
		}
	}
};

BOOL WINAPI DllMain(HINSTANCE /*hInstance*/, DWORD dwReason, LPVOID /*lpReserved*/)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_THREAD_ATTACH:
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}

static LRESULT WINAPI HookProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code == HCBT_SETFOCUS)
	{
		//Focus has changed, record new focus window
		hFocusWnd = (HWND)wParam;

		//And if notification requested, send a windows message.
		if (hNotifyWnd)
			PostMessage(hNotifyWnd, uMessage, wParam, lParam);
	}

	return CallNextHookEx(hHook, code, wParam, lParam);
}

LRESULT CALLBACK KeyboardProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode != HC_ACTION)
		return ::CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);

	const BYTE KEYEVENT = KeyEventType::FromLParam(lParam);

	if (g_CaptureKeys && KEYEVENT == KeyEventType::KeyDown)
	{
		if (g_hKeyboardNotifyWnd)
		{
			PostMessage(g_hKeyboardNotifyWnd, g_uKeyboardMessage, wParam, lParam);

			//Don't send this message on
			return 1;
		}
	}


	return ::CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}


__declspec(dllexport) DWORD WINAPI MonitorFocusChanges(HWND hWnd, UINT message)
{
	if (hHook)
	{
		UnhookWindowsHookEx(hHook);
	}

	hHook = SetWindowsHookEx(WH_CBT, HookProc, CFocusDllModule::Handle(), 0);

	hNotifyWnd = hWnd;
	uMessage = message;

	return TRUE;
}

__declspec(dllexport) DWORD WINAPI StopMonitoringFocusChanges()
{
	if (hHook)
		UnhookWindowsHookEx(hHook);

	hHook = NULL;
	hFocusWnd = NULL;

	return TRUE;
}

__declspec(dllexport) DWORD WINAPI MonitorKeyboardChanges(HWND hWnd, UINT message)
{
	if (g_hKeyboardHook)
	{
		UnhookWindowsHookEx(g_hKeyboardHook);
	}

	g_hKeyboardHook = ::SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, CFocusDllModule::Handle(), 0);

	g_uKeyboardMessage = message;
	g_hKeyboardNotifyWnd = hWnd;

	return TRUE;
}

__declspec(dllexport) DWORD WINAPI StopMonitoringKeyboardChanges()
{
	if (g_hKeyboardHook)
		UnhookWindowsHookEx(g_hKeyboardHook);

	g_hKeyboardHook = NULL;
	hFocusWnd = NULL;

	return TRUE;
}

__declspec(dllexport) HWND WINAPI GetCurrentFocus()
{
	return hFocusWnd;
}

__declspec(dllexport) void WINAPI SetCaptureKeys(bool bCapture)
{
	g_CaptureKeys = bCapture;
}

__declspec(dllexport) bool WINAPI GetCaptureKeys()
{
	return g_CaptureKeys;
}
