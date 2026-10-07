/**
 * @file ClipSavePolicy.cpp
 * @brief Implements DittoCore::ClipSavePolicy.
 */
#include "ClipSavePolicy.h"

#include <utility>

namespace DittoCore
{
	ClipSavePolicy::ClipSavePolicy(ClipSaveSettings settings) :
		m_settings(std::move(settings))
	{
	}

	DuplicateCheck ClipSavePolicy::DuplicateCheckFor(std::uint32_t crc, std::uint32_t lastAddedCrc) const
	{
		if (!m_settings.allowDuplicates)
		{
			return DuplicateCheck::AnyByCrc;
		}
		if (!m_settings.allowBackToBackDuplicates && crc == lastAddedCrc)
		{
			return DuplicateCheck::LastAdded;
		}
		return DuplicateCheck::None;
	}

	bool ClipSavePolicy::TooLarge(std::uint64_t sizeInBytes) const
	{
		return m_settings.maxClipSizeInBytes > 0 && sizeInBytes > static_cast<std::uint64_t>(m_settings.maxClipSizeInBytes);
	}

	bool ClipSavePolicy::IgnoresDibFrom(const std::wstring& lowerCaseApp) const
	{
		return m_settings.ignoreDibFromApps.contains(lowerCaseApp);
	}

	const ClipSaveSettings& ClipSavePolicy::Settings() const
	{
		return m_settings;
	}
}
