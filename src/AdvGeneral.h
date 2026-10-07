#pragma once
#include "afxpropertygridctrl.h"
#include "DialogResizer.h"
#include <afxcoll.h>

#include <array>
#include <memory>
#include <utility>

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
	/** @brief A numeric grid setting: written when its long value changed. */
	struct LongSetting
	{
		/** @brief The property's data, a SETTING_* id. */
		int id{};
		/** @brief Stores the new value in the options. */
		void (*write)(long value) = nullptr;
	};

	/** @brief A True/False grid setting: written when its text changed. */
	struct BoolSetting
	{
		/** @brief The property's data, a SETTING_* id. */
		int id{};
		/** @brief Stores the new value (TRUE when the text is "True") in the options. */
		void (*write)(BOOL value) = nullptr;
	};

	/** @brief A text grid setting: written when its text changed. */
	struct TextSetting
	{
		/** @brief The property's data, a SETTING_* id. */
		int id{};
		/** @brief Stores the new text in the options. */
		void (*write)(LPCTSTR value) = nullptr;
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
	 * @param id The property's SETTING_* id.
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
	 * @param prop The property; its data is the SETTING_* id.
	 */
	void WriteSetting(CMFCPropertyGridProperty* prop);

	/**
	 * @brief Writes a changed transparency percentage; a value outside 1..100 is stored as 100.
	 * @param newValue The property's new value.
	 */
	static void WriteTransparencyPercent(long newValue);

	/**
	 * @brief Writes a changed regex filter or regex process-name filter (the numbered settings).
	 * @param id The property's SETTING_* id; other ids are ignored.
	 * @param newValue The property's value.
	 * @param origValue The property's original value.
	 */
	static void WriteRegexSetting(int id, const VARIANT& newValue, const VARIANT& origValue);

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
