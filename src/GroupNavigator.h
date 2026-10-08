#pragma once

class CAppState;
class CAppWindows;
class CDittoDb;

/**
 * @brief Moves between the groups: changes the current group in CAppState and refreshes the quick
 *        paste window.
 *
 * Owned by CAppServices (Groups()).
 */
class CGroupNavigator
{
public:
	/**
	 * @brief Creates the navigator.
	 * @param state The application state (the current and the saved group); must outlive this object.
	 * @param database The clip database (to read a group); must outlive this object.
	 * @param windows The application's windows (the refresh); must outlive this object.
	 */
	CGroupNavigator(CAppState& state, CDittoDb& database, CAppWindows& windows);

	CGroupNavigator(const CGroupNavigator&) = delete;
	CGroupNavigator& operator=(const CGroupNavigator&) = delete;

	/**
	 * @brief Goes back to the group saved by CAppState::SaveCurrentGroupState, when there is one.
	 * @return TRUE when the saved group was entered.
	 */
	BOOL TryEnterOldGroupState();

	/**
	 * @brief Makes a group (-1: the history) the current group and refreshes the view.
	 * @param lID The group's clip id, or -1.
	 * @param clearOldGroupState TRUE: forget the saved group first.
	 * @param saveCurrentGroupState TRUE: save the current group first.
	 * @return TRUE when the group is current (also when it already was); FALSE when lID is no group
	 *         or reading it failed (reported).
	 */
	BOOL EnterGroupID(long lID, BOOL clearOldGroupState = TRUE, BOOL saveCurrentGroupState = FALSE);

	/**
	 * @brief Sets the group new clips are saved to and updates the quick paste window's status.
	 * @param lID The group; a value <= 0 means none.
	 */
	void SetGroupDefaultID(long lID);

private:
	/**
	 * @brief EnterGroupID's database step: makes lID the current group when Main holds it as a group.
	 * @param lID The clip id of the group to enter.
	 * @return TRUE when the group was entered, FALSE when lID is missing or not a group.
	 * @throws CppSQLite3Exception When the query fails; OpenGroup reports it.
	 */
	BOOL EnterStoredGroup(long lID);

	/**
	 * @brief EnterGroupID's step: makes lID (-1 for the history) the current group.
	 * @param lID The group's clip id, or -1.
	 * @param bResult Receives TRUE when the group was entered.
	 * @return False when reading the group failed (reported); EnterGroupID then returns FALSE.
	 */
	bool OpenGroup(long lID, BOOL& bResult);

	/**
	 * @brief EnterGroupID's last step: refreshes the view when the group was entered and logs a slow switch.
	 * @param bResult TRUE when the group was entered.
	 * @param startTick GetTickCount64 at the start of EnterGroupID.
	 */
	void FinishEnterGroup(BOOL bResult, ULONGLONG startTick);

	/** @brief Refreshes the clip list and repaints the quick paste window's status now. */
	void RefreshAfterGroupChange();

	/** @brief The application state (not owned). */
	CAppState& m_state;
	/** @brief The clip database (not owned). */
	CDittoDb& m_database;
	/** @brief The application's windows (not owned). */
	CAppWindows& m_windows;
};
