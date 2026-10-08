#pragma once

class CGetSetOptions;
class CLastAddedClip;
class CDittoDb;
class CRegisteredClipboardFormats;
class CAppWindows;

/**
 * @brief The services a clip works with: CClip, CClip_ImportExport and the clip-id operations
 *        (CClipIDs) get them through this one object.
 *
 * Owned by CAppServices (ClipContext()); it only holds references to services that CAppServices
 * also owns and creates before it, so it never outlives them.
 */
class CClipContext
{
public:
	/**
	 * @brief Links the services.
	 * @param settings The application's settings (the save settings, the multi-paste options).
	 * @param lastAdded The clip saved last (CAppState::LastAddedClip): the duplicate check reads it,
	 *        a save records the clip there.
	 * @param database The clip database connection.
	 * @param formats The clipboard formats Ditto registers by name.
	 * @param windows The application's windows (the main window handle, the list updates).
	 * All of them must outlive this object.
	 */
	CClipContext(CGetSetOptions& settings, CLastAddedClip& lastAdded, CDittoDb& database,
				 const CRegisteredClipboardFormats& formats, CAppWindows& windows) :
		m_settings(settings),
		m_lastAdded(lastAdded),
		m_database(database),
		m_formats(formats),
		m_windows(windows)
	{
	}

	CClipContext(const CClipContext&) = delete;
	CClipContext& operator=(const CClipContext&) = delete;

	/**
	 * @brief The application's settings.
	 * @return The settings (not owned).
	 */
	CGetSetOptions& Settings() const { return m_settings; }

	/**
	 * @brief The clip saved last.
	 * @return The record (not owned).
	 */
	CLastAddedClip& LastAdded() const { return m_lastAdded; }

	/**
	 * @brief The clip database connection.
	 * @return The connection (not owned).
	 */
	CDittoDb& Database() const { return m_database; }

	/**
	 * @brief The clipboard formats Ditto registers by name.
	 * @return The formats (not owned).
	 */
	const CRegisteredClipboardFormats& Formats() const { return m_formats; }

	/**
	 * @brief The application's windows.
	 * @return The windows (not owned).
	 */
	CAppWindows& Windows() const { return m_windows; }

private:
	/** @brief The application's settings (not owned). */
	CGetSetOptions& m_settings;
	/** @brief The clip saved last (not owned). */
	CLastAddedClip& m_lastAdded;
	/** @brief The clip database connection (not owned). */
	CDittoDb& m_database;
	/** @brief The registered clipboard formats (not owned). */
	const CRegisteredClipboardFormats& m_formats;
	/** @brief The application's windows (not owned). */
	CAppWindows& m_windows;
};
