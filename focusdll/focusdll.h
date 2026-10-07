#ifdef __cplusplus
extern "C" {
#endif


// The DLL's exports. Only the DLL itself (built with FOCUS_EXPORTS in every configuration)
// includes this header, so the functions are declared dllexport.

__declspec(dllexport) DWORD WINAPI MonitorFocusChanges(HWND hWnd,UINT message);
__declspec(dllexport) DWORD WINAPI StopMonitoringFocusChanges();

__declspec(dllexport) DWORD WINAPI MonitorKeyboardChanges(HWND hWnd,UINT message);
__declspec(dllexport) DWORD WINAPI StopMonitoringKeyboardChanges();

__declspec(dllexport) HWND  WINAPI GetCurrentFocus();

__declspec(dllexport) void  WINAPI SetCaptureKeys(bool bCapture);
__declspec(dllexport) bool  WINAPI GetCaptureKeys();


#ifdef __cplusplus
}
#endif
