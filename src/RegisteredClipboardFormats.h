#pragma once

/**
 * @brief The clipboard formats Ditto registers by name; registered once, when the object is created.
 *
 * Owned by CAppServices (ClipboardFormats()). The names are the ones Windows and other programs use,
 * so they never change.
 */
class CRegisteredClipboardFormats
{
public:
	/** @brief Registers the formats (RegisterClipboardFormat; PNG through CClipboardFormats::GetFormatID). */
	CRegisteredClipboardFormats();

	CRegisteredClipboardFormats(const CRegisteredClipboardFormats&) = delete;
	CRegisteredClipboardFormats& operator=(const CRegisteredClipboardFormats&) = delete;

	/**
	 * @brief "Rich Text Format".
	 * @return The format id.
	 */
	CLIPFORMAT Rtf() const;

	/**
	 * @brief "HTML Format".
	 * @return The format id.
	 */
	CLIPFORMAT Html() const;

	/**
	 * @brief "PNG".
	 * @return The format id.
	 */
	CLIPFORMAT Png() const;

	/**
	 * @brief "Ditto Ping Format": the clipboard viewer's check that it still gets clipboard changes.
	 * @return The format id.
	 */
	CLIPFORMAT Ping() const;

	/**
	 * @brief "Clipboard Viewer Ignore": clipboard content that clipboard viewers do not save.
	 * @return The format id.
	 */
	CLIPFORMAT IgnoreClipboard() const;

	/**
	 * @brief "Ditto Delay Saving Data".
	 * @return The format id.
	 */
	CLIPFORMAT DelaySavingData() const;

	/**
	 * @brief "Ditto File Data".
	 * @return The format id.
	 */
	CLIPFORMAT DittoFileData() const;

	/**
	 * @brief "ExcludeClipboardContentFromMonitorProcessing" (Windows' clipboard-format convention).
	 * @return The format id.
	 */
	CLIPFORMAT ExcludeClipboardContentFromMonitorProcessing() const;

	/**
	 * @brief "CanIncludeInClipboardHistory" (Windows' clipboard-format convention).
	 * @return The format id.
	 */
	CLIPFORMAT CanIncludeInClipboardHistory() const;

private:
	/**
	 * @brief Registers one clipboard format by name.
	 * @param name The format name.
	 * @return The format id (registered formats lie in 0xC000..0xFFFF, so they fit in a CLIPFORMAT).
	 */
	static CLIPFORMAT Register(const TCHAR* name);

	/** @brief "Rich Text Format". */
	CLIPFORMAT m_rtf{};
	/** @brief "HTML Format". */
	CLIPFORMAT m_html{};
	/** @brief "Ditto Ping Format". */
	CLIPFORMAT m_ping{};
	/** @brief "Clipboard Viewer Ignore". */
	CLIPFORMAT m_ignoreClipboard{};
	/** @brief "Ditto Delay Saving Data". */
	CLIPFORMAT m_delaySavingData{};
	/** @brief "Ditto File Data". */
	CLIPFORMAT m_dittoFileData{};
	/** @brief "PNG". */
	CLIPFORMAT m_png{};
	/** @brief "ExcludeClipboardContentFromMonitorProcessing". */
	CLIPFORMAT m_excludeFromMonitorProcessing{};
	/** @brief "CanIncludeInClipboardHistory". */
	CLIPFORMAT m_canIncludeInClipboardHistory{};
};
