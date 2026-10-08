#pragma once

#include "Options.h"

/**
 * @brief The composition root: owns the application's services and hands them out.
 *
 * CCP_MainApp owns the one instance (theApp.Services()). Classes that an owner creates get the
 * services they need from that owner (constructor or function parameters); MFC windows,
 * dialogs and threads that the framework creates take them from theApp.Services().
 */
class CAppServices
{
public:
	/** @brief Creates the services; the settings start over the registry until LoadSettings runs. */
	CAppServices();

	CAppServices(const CAppServices&) = delete;
	CAppServices& operator=(const CAppServices&) = delete;

	/**
	 * @brief The application's settings.
	 * @return The settings, valid as long as this object.
	 */
	CGetSetOptions& Settings();

private:
	/// The application's settings.
	CGetSetOptions m_settings{};
};
