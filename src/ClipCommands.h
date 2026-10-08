#pragma once

class CAppState;
class CAppWindows;
class CClip;
class CClipContext;
class CClipEditThread;
class CClipIDs;
class CGetSetOptions;
class CMultiLanguage;
class CRegisteredClipboardFormats;

/**
 * @brief The user's commands on whole clips that the main frame and the quick paste window share:
 *        edit clips in an editor, import exported clips.
 *
 * Owned by CAppServices (ClipCommands()).
 */
class CClipCommands
{
public:
	/**
	 * @brief Creates the commands.
	 * @param settings The application's settings (editors, edit folder, import folder); must outlive this object.
	 * @param language The UI texts; must outlive this object.
	 * @param clipboardFormats The registered clipboard formats; must outlive this object.
	 * @param state The application state (the taskbar-icon users); must outlive this object.
	 * @param windows The application's windows (the internal editor, the taskbar icon); must outlive this object.
	 * @param editThread The watcher of edited clip files; must outlive this object.
	 * @param clipContext The services of the clips the commands load and import; must outlive this object.
	 */
	CClipCommands(CGetSetOptions& settings, CMultiLanguage& language, const CRegisteredClipboardFormats& clipboardFormats, CAppState& state, CAppWindows& windows, CClipEditThread& editThread, CClipContext& clipContext);

	CClipCommands(const CClipCommands&) = delete;
	CClipCommands& operator=(const CClipCommands&) = delete;

	/**
	 * @brief Edits up to 20 clips: each is written to a file and opened in its editor (text and RTF
	 *        clips without an editor set go to the internal editor).
	 * @param Ids The clips; -1 for a new clip.
	 * @param bShowError Not used.
	 * @param forceTextEdit True: edit RTF clips as text.
	 * @return True when an external editor was launched.
	 */
	bool EditItems(CClipIDs& Ids, bool bShowError, bool forceTextEdit);

	/**
	 * @brief Asks for an exported clip file (.dto) and imports its clips; shows the result.
	 * @param hWnd The owner of the dialogs and message boxes.
	 * @return False when the user cancelled the file dialog.
	 */
	bool ImportClips(HWND hWnd);

private:
	/** @brief How EditItems edits one clip: the file kind, its extension and the editor. */
	struct ClipEditTarget
	{
		/** @brief The clip is written as a unicode text file. */
		bool unicodeFile{};
		/** @brief The clip is written as an ANSI text file. */
		bool asciFile{};
		/** @brief The clip is written as an RTF file. */
		bool rtfFile{};
		/** @brief The clip is written as an image file. */
		bool imageFile{};
		/** @brief The editor set in the options; empty for the system mapping (or the internal editor for text). */
		CString exePath{};
		/** @brief The file extension (txt, rtf, png or bmp). */
		CString extension{};
	};

	/**
	 * @brief EditItems' step for one clip: writes it to a file and opens the editor (or the internal editor).
	 * @param id The clip id; -1 for a new clip.
	 * @param forceTextEdit True: edit RTF clips as text.
	 * @param lastFileCheckId In/out: the next number tried for a new clip's file name.
	 * @return True when an external editor was launched.
	 */
	bool EditItem(int id, bool forceTextEdit, int& lastFileCheckId);

	/**
	 * @brief Chooses the file kind and editor of a clip from its formats.
	 * @param clip The clip, its formats loaded.
	 * @param id The clip id; -1 for a new clip.
	 * @param forceTextEdit True: edit RTF clips as text.
	 * @param target Receives the choice.
	 * @return False when the clip has no editable format.
	 */
	bool ChooseClipEditTarget(CClip& clip, int id, bool forceTextEdit, ClipEditTarget& target);

	/**
	 * @brief Opens a text or RTF clip in the internal editor when no external editor is set.
	 * @param target The clip's edit target.
	 * @param id The clip id.
	 * @return True when the internal editor took the clip.
	 */
	bool EditInInternalEditor(const ClipEditTarget& target, int id);

	/**
	 * @brief The file a clip is edited in; a new clip gets the first free NewClip_n name.
	 * @param id The clip id; negative for a new clip.
	 * @param extension The file extension.
	 * @param lastFileCheckId In/out: the next number tried for a new clip's file name.
	 * @return The file path (empty when no free name was found).
	 */
	CString MakeEditFilePath(int id, const CString& extension, int& lastFileCheckId);

	/**
	 * @brief Opens a clip's file in the editor (or with the system mapping).
	 * @param exePath The editor; empty for the system mapping.
	 * @param savePath The clip's file.
	 * @param id The clip id (for the log).
	 * @return False when ShellExecuteEx failed.
	 */
	bool LaunchClipEditor(const CString& exePath, const CString& savePath, int id);

	/**
	 * @brief ImportClips' file dialog: asks for the exported clip file and remembers its folder.
	 * @param hWnd The owner of the dialog.
	 * @param filePath Receives the chosen file.
	 * @return False when the user cancelled.
	 */
	bool AskImportFile(HWND hWnd, CString& filePath);

	/**
	 * @brief ImportClips' import step: imports the file and shows the result or the error.
	 * @param hWnd The owner of the message boxes.
	 * @param filePath The exported clip file.
	 */
	void ImportFile(HWND hWnd, const CString& filePath);

	/** @brief The application's settings (not owned). */
	CGetSetOptions& m_settings;
	/** @brief The UI texts (not owned). */
	CMultiLanguage& m_language;
	/** @brief The registered clipboard formats (not owned). */
	const CRegisteredClipboardFormats& m_clipboardFormats;
	/** @brief The application state (not owned). */
	CAppState& m_state;
	/** @brief The application's windows (not owned). */
	CAppWindows& m_windows;
	/** @brief The watcher of edited clip files (not owned). */
	CClipEditThread& m_editThread;
	/** @brief The services of the clips the commands load and import (not owned). */
	CClipContext& m_clipContext;
};
