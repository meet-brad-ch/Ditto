#pragma once

#include "Misc.h"

#include <atomic>

class CGetSetOptions;

/**
 * @brief The clip saved last (its CRC and id): CClip's duplicate check compares a new clip with it.
 */
class CLastAddedClip
{
public:
	/**
	 * @brief The CRC of the clip saved last.
	 * @return The CRC; 0 before the first save or after ClearCrc.
	 */
	DWORD Crc() const;

	/**
	 * @brief The id of the clip saved last.
	 * @return The clip id; -1 before the first save.
	 */
	int Id() const;

	/**
	 * @brief Records a clip that was just saved.
	 * @param crc The clip's CRC.
	 * @param id The clip's id.
	 */
	void Record(DWORD crc, int id);

	/** @brief Forgets the CRC (the id stays): the next clip is not taken as a copy of the last one. */
	void ClearCrc();

private:
	/** @brief The CRC of the clip saved last. */
	DWORD m_crc{};
	/** @brief The id of the clip saved last. */
	int m_id{-1};
};

/**
 * @brief The application-wide state: the current group, the window flags, the start-up time, the
 *        pending copy reason and save-to group, the clip saved last and the taskbar-icon users.
 *
 * Owned by CAppServices (State()). The data members keep the names they had on CCP_MainApp.
 */
class CAppState
{
public:
	/**
	 * @brief Creates the start-up state (the history is the current group).
	 * @param settings The application's settings (the copy-reason and save-to-group timeouts); must
	 *        outlive this object.
	 */
	explicit CAppState(CGetSetOptions& settings);

	CAppState(const CAppState&) = delete;
	CAppState& operator=(const CAppState&) = delete;

	/** @brief The group new clips are saved to (0: none). */
	long m_GroupDefaultID{0};
	/** @brief The current group (-1: the history). */
	long m_GroupID{-1};
	/** @brief The current group's parent. */
	long m_GroupParentID{0};
	/** @brief The current group's description. */
	CString m_GroupText{_T("History")};

	/** @brief The group saved by SaveCurrentGroupState (-2: none). */
	long m_oldGroupID{-2};
	/** @brief The saved group's parent (-2: none). */
	long m_oldGroupParentID{-2};
	/** @brief The saved group's description. */
	CString m_oldGroupText{};

	/** @brief The clip id given the focus by CQPasteWnd::FillList (-1: none). */
	long m_FocusID{-1};
	/** @brief True while the quick paste window shows. */
	bool m_bShowingQuickPaste{};
	/** @brief The status text shown in the quick paste window's caption. */
	CString m_Status{};
	/** @brief True: RefreshView and RefreshClipInUI post their message; false: they send it. */
	bool m_bAsynchronousRefreshView{true};
	/** @brief True when the clip database lies on a network share (set when the database is opened). */
	bool m_databaseOnNetworkShare{};
	/** @brief When Ditto started. */
	COleDateTime m_oldtStartUp{};
	/** @brief True from the end of the main window's creation until BeforeMainClose. */
	bool m_bAppRunning{};
	/** @brief True from BeforeMainClose on. */
	bool m_bAppExiting{};

	/** @brief Saves the current group, so that TryEnterOldGroupState can go back to it. */
	void SaveCurrentGroupState();

	/** @brief Forgets the saved group. */
	void ClearOldGroupState();

	/**
	 * @brief The current group's id.
	 * @return m_GroupID.
	 */
	long GetValidGroupID() const;

	/**
	 * @brief Sets the group the next copied clip is saved to (for the save-to-group timeout).
	 * @param groupId The group.
	 */
	void SetActiveGroupId(int groupId);

	/**
	 * @brief Takes the group set by SetActiveGroupId when it was set within the timeout, and forgets it.
	 * @return The group, or -1 when none was set or the timeout passed.
	 */
	int GetActiveGroupId();

	/**
	 * @brief Sets why the next copy happens (for the copy-reason timeout).
	 * @param copyReason The reason.
	 */
	void SetCopyReason(CopyReasonEnum::CopyReason copyReason);

	/**
	 * @brief Takes the reason set by SetCopyReason when it was set within the timeout, and forgets it.
	 * @return The reason, or COPY_TO_UNKOWN when none was set or the timeout passed.
	 */
	CopyReasonEnum::CopyReason GetCopyReason();

	/**
	 * @brief The clip saved last.
	 * @return The record; valid as long as this object.
	 */
	CLastAddedClip& LastAddedClip();

	/**
	 * @brief Counts one more user of the main window's taskbar icon (CShowTaskBarIcon).
	 * @return The number of users after the increment.
	 */
	long AddTaskbarIconUser();

	/**
	 * @brief Counts one user of the main window's taskbar icon less (CShowTaskBarIcon).
	 * @return The number of users after the decrement.
	 */
	long ReleaseTaskbarIconUser();

private:
	/** @brief The application's settings (not owned). */
	CGetSetOptions& m_settings;
	/** @brief The group set by SetActiveGroupId (-1: none). */
	int m_activeGroupId{-1};
	/** @brief GetTickCount64 when SetActiveGroupId ran. */
	ULONGLONG m_activeGroupStartTime{};
	/** @brief The reason set by SetCopyReason. */
	CopyReasonEnum::CopyReason m_copyReason{CopyReasonEnum::COPY_TO_UNKOWN};
	/** @brief GetTickCount64 when SetCopyReason ran. */
	ULONGLONG m_copyReasonStartTime{};
	/** @brief The clip saved last. */
	CLastAddedClip m_lastAddedClip{};
	/** @brief The users of the main window's taskbar icon (dialogs that show it while they are open). */
	std::atomic<long> m_taskbarIconUsers{0};
};
