#pragma once

#include <powrprof.h>

class CPowerManager
{
public:
	CPowerManager();
	~CPowerManager(void);

	void Start(HWND hWnd);
	void Close();

protected:
	HPOWERNOTIFY m_registrationHandle;
};
