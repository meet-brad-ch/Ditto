#pragma once

class CTrayWnd : public CWnd
{
	DECLARE_DYNAMIC(CTrayWnd)

public:
	CTrayWnd();
	virtual ~CTrayWnd();

	/**
	 * @brief The registered "TaskbarCreated" message Explorer broadcasts when the taskbar is (re)created.
	 * @return The message id, registered on the first call; a reference to it, as ON_REGISTERED_MESSAGE takes its address.
	 */
	static const UINT& TaskbarCreatedMessage();

protected:
	DECLARE_MESSAGE_MAP()
	LRESULT OnTaskBarCreated(WPARAM wParam, LPARAM lParam);
};
