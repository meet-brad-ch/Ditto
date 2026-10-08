#pragma once

#include <powrprof.h>
#include <memory>

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
	 * @brief The suspend/resume notification callback: on resume, posts ReopenDatabase to the window given to Start.
	 * @param Context The window's HWND (DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS::Context); not the manager, so a
	 *        registration that could not be removed never reaches a destroyed manager.
	 * @param Type The power broadcast type (PBT_*).
	 * @param Setting Unused.
	 * @return 0.
	 */
	static ULONG CALLBACK PowerChanged(PVOID Context, ULONG Type, PVOID Setting);

	HPOWERNOTIFY m_registrationHandle;

	/**
	 * @brief The registration parameters (PowerChanged with the window as context); live as long as the
	 *        registration: Close hands them over to the process when the registration cannot be removed.
	 */
	std::unique_ptr<DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS> m_subscribeParameters{};
};
