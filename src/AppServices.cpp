#include "stdafx.h"
#include "AppServices.h"

CAppServices::CAppServices() = default;

CGetSetOptions& CAppServices::Settings()
{
	return m_settings;
}
