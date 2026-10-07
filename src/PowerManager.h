#pragma once

#include <powrprof.h>

class CPowerManager
{
public:
	CPowerManager();
	~CPowerManager(void);
	CPowerManager(const CPowerManager&) = delete;
	CPowerManager& operator=(const CPowerManager&) = delete;

	void Start(HWND hWnd);
	void Close();

protected:
	/**
	 * @brief The suspend/resume notification callback: on resume, posts ReopenDatabase to the manager's window.
	 * @param Context The CPowerManager that registered (DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS::Context).
	 * @param Type The power broadcast type (PBT_*).
	 * @param Setting Unused.
	 * @return 0.
	 */
	static ULONG CALLBACK PowerChanged(PVOID Context, ULONG Type, PVOID Setting);

	HPOWERNOTIFY m_registrationHandle;

	/** @brief The window PowerChanged posts ReopenDatabase to (set by Start). */
	HWND m_notifyHwnd{};

	/** @brief The registration parameters (PowerChanged with this manager as context); live as long as the registration. */
	DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS m_subscribeParameters{};
};
