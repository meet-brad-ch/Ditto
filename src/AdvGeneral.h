#pragma once
#include "afxpropertygridctrl.h"
#include "DialogResizer.h"
#include <afxcoll.h>

#include <array>
#include <memory>
#include <utility>

class CGetSetOptions;

class CAdvGeneral : public CDialogEx
{
	DECLARE_DYNAMIC(CAdvGeneral)

public:
	CAdvGeneral(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAdvGeneral();

// Dialog Data
	enum { IDD = IDD_ADV_OPTIONS };

	CDialogResizer m_Resize;
	bool m_mouseDownOnCaption{};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	/**
	 * @brief Creates a property grid item for AddProperty/AddSubItem, which take ownership of it.
	 * @tparam T the property type.
	 * @tparam Args the constructor argument types.
	 * @param args the constructor arguments.
	 * @return the new item; the caller hands it to the property grid at once.
	 */
	template <typename T, typename... Args>
	static T* MakeGridProperty(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...).release(); // ownership: the property grid
	}

	void AddTrueFalse(CMFCPropertyGridProperty * pGroupTest, CString desc, BOOL value, int settingId);
	void Search(bool fromSelection);

	CEdit m_editFilter;

	afx_msg void OnEnChangeAdvFilter();

	DECLARE_MESSAGE_MAP()
public:
	CMFCPropertyGridCtrl m_propertyGrid;
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBnClickedBtCompactAndRepair();
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedButtonNextMatch();
	afx_msg void OnBnClickedButtonCopyScripts2();

private:
	/** @brief The setting ids: each grid property's data (the values never change; 62, 83, 86, 87, 101, 102 and 107 are unused). */
	enum : int
	{
		/** @brief Amount of text to save for description. */
		SettingDescSize = 1,
		/** @brief Display icon in system tray. */
		SettingShowTaskbarIcon = 2,
		/** @brief Save multi-pastes. */
		SettingSaveMultiPaste = 3,
		/** @brief Hide Ditto on hot key if Ditto is visible. */
		SettingHideOnHotkeyIfVisible = 4,
		/** @brief Paste clip in active window after selection. */
		SettingPasteInActiveWindow = 5,
		/** @brief Maximum clip size in bytes (0 for no limit). */
		SettingMaxClipSize = 6,
		/** @brief Multi-paste clip separator ([LF] = line feed). */
		SettingClipSeparator = 7,
		/** @brief Ensure Ditto is always connected to the clipboard. */
		SettingEnsureConnected = 8,
		/** @brief On copy play the sound. */
		SettingCopyPlaySound = 9,
		/** @brief Show text for first ten copy hot keys. */
		SettingTextFirstTen = 10,
		/** @brief Show leading whitespace. */
		SettingShowLeadingWhitespace = 11,
		/** @brief Text lines per clip. */
		SettingLinesPerRow = 12,
		/** @brief Transparency enabled. */
		SettingEnableTransparency = 13,
		/** @brief Show thumbnails (for CF_DIB and PNG types). */
		SettingDrawThumbnails = 14,
		/** @brief Draw RTF text in list (for RTF types). */
		SettingDrawRtf = 15,
		/** @brief Find as you type. */
		SettingFindAsType = 16,
		/** @brief Ensure entire window is visible. */
		SettingEnsureWindowIsVisible = 17,
		/** @brief Show clips that are in groups in main list. */
		SettingShowGroupClipsInList = 18,
		/** @brief Prompt when deleting clips. */
		SettingPromptOnDelete = 19,
		/** @brief Always show scroll bar. */
		SettingAlwaysShowScrollBar = 20,
		/** @brief Elevated privileges to paste into elevated apps. */
		SettingPasteAsAdmin = 21,
		/** @brief Show in taskbar. */
		SettingShowInTaskbar = 22,
		/** @brief Show indicator a clip has been pasted. */
		SettingShowClipPasted = 23,
		/** @brief Diff application path. */
		SettingDiffApp = 24,
		/** @brief Transparency percentage. */
		SettingTransparency = 25,
		/** @brief Update clip order on paste. */
		SettingUpdateOrderOnPaste = 26,
		/** @brief Allow duplicates. */
		SettingAllowDuplicates = 27,
		/** @brief Regex filter 1 (the first of the 15 consecutive regex filter ids). */
		SettingRegexFiltering1 = 28,
		/** @brief Regex filter 2. */
		SettingRegexFiltering2 = 29,
		/** @brief Regex filter 3. */
		SettingRegexFiltering3 = 30,
		/** @brief Regex filter 4. */
		SettingRegexFiltering4 = 31,
		/** @brief Regex filter 5. */
		SettingRegexFiltering5 = 32,
		/** @brief Regex filter 6. */
		SettingRegexFiltering6 = 33,
		/** @brief Regex filter 7. */
		SettingRegexFiltering7 = 34,
		/** @brief Regex filter 8. */
		SettingRegexFiltering8 = 35,
		/** @brief Regex filter 9. */
		SettingRegexFiltering9 = 36,
		/** @brief Regex filter 10. */
		SettingRegexFiltering10 = 37,
		/** @brief Regex filter 11. */
		SettingRegexFiltering11 = 38,
		/** @brief Regex filter 12. */
		SettingRegexFiltering12 = 39,
		/** @brief Regex filter 13. */
		SettingRegexFiltering13 = 40,
		/** @brief Regex filter 14. */
		SettingRegexFiltering14 = 41,
		/** @brief Regex filter 15 (the last regex filter id). */
		SettingRegexFiltering15 = 42,
		/** @brief Regex filter 1's process name (the first of the 15 consecutive process name ids). */
		SettingRegexFilteringByProcessName1 = 43,
		/** @brief Regex filter 2's process name. */
		SettingRegexFilteringByProcessName2 = 44,
		/** @brief Regex filter 3's process name. */
		SettingRegexFilteringByProcessName3 = 45,
		/** @brief Regex filter 4's process name. */
		SettingRegexFilteringByProcessName4 = 46,
		/** @brief Regex filter 5's process name. */
		SettingRegexFilteringByProcessName5 = 47,
		/** @brief Regex filter 6's process name. */
		SettingRegexFilteringByProcessName6 = 48,
		/** @brief Regex filter 7's process name. */
		SettingRegexFilteringByProcessName7 = 49,
		/** @brief Regex filter 8's process name. */
		SettingRegexFilteringByProcessName8 = 50,
		/** @brief Regex filter 9's process name. */
		SettingRegexFilteringByProcessName9 = 51,
		/** @brief Regex filter 10's process name. */
		SettingRegexFilteringByProcessName10 = 52,
		/** @brief Regex filter 11's process name. */
		SettingRegexFilteringByProcessName11 = 53,
		/** @brief Regex filter 12's process name. */
		SettingRegexFilteringByProcessName12 = 54,
		/** @brief Regex filter 13's process name. */
		SettingRegexFilteringByProcessName13 = 55,
		/** @brief Regex filter 14's process name. */
		SettingRegexFilteringByProcessName14 = 56,
		/** @brief Regex filter 15's process name (the last process name id). */
		SettingRegexFilteringByProcessName15 = 57,
		/** @brief Show startup tooltip message. */
		SettingShowStartupMessage = 58,
		/** @brief Tooltip display time (ms). */
		SettingTooltipTimeout = 59,
		/** @brief Selected index. */
		SettingSelectedIndex = 60,
		/** @brief Save clipboard delay (ms). */
		SettingClipboardSaveDelay = 61,
		/** @brief Multi-paste in reverse order. */
		SettingMultipasteReverseOrder = 63,
		/** @brief Default paste string. */
		SettingDefaultPasteString = 64,
		/** @brief Default copy string. */
		SettingDefaultCopyString = 65,
		/** @brief Default cut string. */
		SettingDefaultCutString = 66,
		/** @brief Revert to top level group on close. */
		SettingRevertToTopLevelGroup = 67,
		/** @brief Update clip order on ctrl-c. */
		SettingUpdateOrderOnCtrlC = 68,
		/** @brief Tooltip maximum display lines. */
		SettingTooltipLines = 69,
		/** @brief Tooltip display characters. */
		SettingTooltipCharacters = 70,
		/** @brief Activate window delay. */
		SettingActivateWindowDelay = 71,
		/** @brief Double shortcut keystroke timeout. */
		SettingDoubleKeystrokeTimeout = 72,
		/** @brief Send keys delay (ms). */
		SettingSendKeysDelay = 73,
		/** @brief First ten hot keys start index. */
		SettingFirstTenHotkeysStart = 74,
		/** @brief First ten hot keys font size. */
		SettingFirstTenHotkeysFontSize = 75,
		/** @brief Open to group same as active exe. */
		SettingOpenToGroupAsActiveExe = 76,
		/** @brief Add file drop when dragging clips. */
		SettingAddCfHdropOnDrag = 77,
		/** @brief Copy and save clipboard delay (ms). */
		SettingCopySaveDelay = 78,
		/** @brief Editor default font size. */
		SettingEditorFontSize = 79,
		/** @brief Move selection on open hot key. */
		SettingMoveSelectionOnOpenHotkey = 80,
		/** @brief Allow back to back duplicates (if allowing duplicates). */
		SettingAllowBackToBackDuplicates = 81,
		/** @brief Maintain search view. */
		SettingMaintainSearchView = 82,
		/** @brief Write debug to file. */
		SettingDebugToFile = 84,
		/** @brief Write debug to OutputDebugString. */
		SettingDebugToOutputString = 85,
		/** @brief Ignore copies faster than (ms). */
		SettingIgnoreFalseCopiesDelay = 88,
		/** @brief Refresh view after paste. */
		SettingRefreshViewAfterPaste = 89,
		/** @brief Slugify separator. */
		SettingSlugifySeparator = 90,
		/** @brief Fast thumbnails (true = fast / low quality). */
		SettingFastThumbnailMode = 91,
		/** @brief Clipboard restore delay after copy buffer sent paste (ms). */
		SettingClipboardRestoreAfterCopyBufferDelay = 92,
		/** @brief Support all types ignoring the supported type list. */
		SettingSupportAllTypes = 93,
		/** @brief Ignore CF_DIB when a clip is detected as text content. */
		SettingIgnoreAnnoyingCfDib = 94,
		/** @brief Regex case insensitive search. */
		SettingRegexCaseInsensitive = 95,
		/** @brief Draw swatch for hex, RGB, and HSL colors. */
		SettingDrawCopiedColorCode = 96,
		/** @brief Center window below cursor or caret. */
		SettingCenterWindowBelowCursorCaret = 97,
		/** @brief Text editor path (empty for system mapping). */
		SettingTextEditorPath = 98,
		/** @brief RTF editor path. */
		SettingRtfEditorPath = 99,
		/** @brief Update description on clip edit. */
		SettingUpdateDescOnClipEdit = 100,
		/** @brief Diff save compare files as utf8. */
		SettingUseUtf8ForDiff = 103,
		/** @brief Image editor path (empty for system mapping). */
		SettingImageEditorPath = 104,
		/** @brief Clip edit save delay after load. */
		SettingClipEditSaveDelayAfterLoad = 105,
		/** @brief Clip edit save delay after save. */
		SettingClipEditSaveDelayAfterSave = 106,
		/** @brief Do not hide Ditto window on deactivate. */
		SettingDoNotHideOnDeactivate = 108,
		/** @brief Hide taskbar icon when Ditto window closes. */
		SettingHideTaskbarIconOnClose = 109,
		/** @brief Use modern scroll bar. */
		SettingUseModernScrollbar = 110,
		/** @brief Enforce clipboard ignore formats. */
		SettingEnforceClipboardIgnoreFormats = 111,
	};

	/** @brief A numeric grid setting: written when its long value changed. */
	struct LongSetting
	{
		/** @brief The property's data, a Setting* id. */
		int id{};
		/** @brief Stores the new value in the options (the settings, the value). */
		void (*write)(CGetSetOptions& settings, long value) = nullptr;
	};

	/** @brief A True/False grid setting: written when its text changed. */
	struct BoolSetting
	{
		/** @brief The property's data, a Setting* id. */
		int id{};
		/** @brief Stores the new value (TRUE when the text is "True") in the options (the settings, the value). */
		void (*write)(CGetSetOptions& settings, BOOL value) = nullptr;
	};

	/** @brief A text grid setting: written when its text changed. */
	struct TextSetting
	{
		/** @brief The property's data, a Setting* id. */
		int id{};
		/** @brief Stores the new text in the options (the settings, the text). */
		void (*write)(CGetSetOptions& settings, LPCTSTR value) = nullptr;
	};

	/** @brief The numeric settings OnBnClickedOk writes, by property id. */
	static const std::array<LongSetting, 20> s_longSettings;
	/** @brief The True/False settings OnBnClickedOk writes, by property id. */
	static const std::array<BoolSetting, 43> s_boolSettings;
	/** @brief The text settings OnBnClickedOk writes, by property id (the regex filters apart). */
	static const std::array<TextSetting, 11> s_textSettings;

	/**
	 * @brief Finds the setting of a property id in a table.
	 * @tparam Setting The table's record type.
	 * @tparam Count The table's size.
	 * @param settings The table.
	 * @param id The property's Setting* id.
	 * @return The setting, or nullptr when the table has none for the id.
	 */
	template <typename Setting, size_t Count>
	static const Setting* FindSetting(const std::array<Setting, Count>& settings, int id)
	{
		for (const Setting& setting : settings)
		{
			if (setting.id == id)
			{
				return &setting;
			}
		}
		return nullptr;
	}

	/**
	 * @brief Writes one grid property's value to the options when the user changed it.
	 * @param prop The property; its data is the Setting* id.
	 */
	void WriteSetting(CMFCPropertyGridProperty* prop);

	/**
	 * @brief The application's settings.
	 * @return theApp.Services().Settings().
	 */
	CGetSetOptions& Settings() const;

	/**
	 * @brief Writes a changed transparency percentage; a value outside 1..100 is stored as 100.
	 * @param settings The settings written to.
	 * @param newValue The property's new value.
	 */
	static void WriteTransparencyPercent(CGetSetOptions& settings, long newValue);

	/**
	 * @brief Writes a changed regex filter or regex process-name filter (the numbered settings).
	 * @param settings The settings written to.
	 * @param id The property's Setting* id; other ids are ignored.
	 * @param newValue The property's value.
	 * @param origValue The property's original value.
	 */
	static void WriteRegexSetting(CGetSetOptions& settings, int id, const VARIANT& newValue, const VARIANT& origValue);

	/**
	 * @brief Selects a search match in the grid and scrolls it into view.
	 * @param pProp The top-level group holding the match.
	 * @param row The match's row in the group.
	 * @param pSubItem The matching property.
	 */
	void ShowSearchMatch(CMFCPropertyGridProperty* pProp, int row, CMFCPropertyGridProperty* pSubItem);

	/**
	 * @brief Searches one top-level group for the filter text and selects the first match after the selection.
	 * @param pProp The top-level group.
	 * @param filterText The lower-case filter text.
	 * @param fromSelection True: start after the selected property.
	 * @param selection The selected property, or nullptr.
	 * @param foundSelection In: the selection was passed already; out: updated while scanning.
	 */
	void SearchGroup(CMFCPropertyGridProperty* pProp, const CString& filterText, bool fromSelection, CMFCPropertyGridProperty* selection, bool& foundSelection);
};
