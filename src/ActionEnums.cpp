#include "stdafx.h"
#include "ActionEnums.h"
#include "Accels.h"
#include "Misc.h"
#include "CP_Main.h"

#include <algorithm>
#include <array>

ActionEnums::ActionEnums()
{
}


ActionEnums::~ActionEnums()
{
}

const std::array<ActionEnums::ActionDescription, 117> ActionEnums::s_descriptions{ {
	{ SHOWDESCRIPTION, "View Full Description" },
	{ NEXTDESCRIPTION, "Next Full Description" },
	{ PREVDESCRIPTION, "Previous Full Description" },
	{ SHOWMENU, "Show Context Menu" },
	{ SYSTEM_MENU, "Show System Context Menu" },
	{ NEWGROUP, "New Group" },
	{ NEWGROUPSELECTION, "New Group Selection" },
	{ TOGGLEFILELOGGING, "Toggle On File Logging" },
	{ TOGGLEOUTPUTDEBUGSTRING, "Toggle OutputDebugString Logging" },
	{ CLOSEWINDOW, "Close Window" },
	{ NEXTTABCONTROL, "NEXTTABCONTROL" },
	{ PREVTABCONTROL, "PREVTABCONTROL" },
	{ SHOWGROUPS, "View Groups" },
	{ NEWCLIP, "New Clip" },
	{ EDITCLIP, "Edit Clip" },
	{ MODIFIER_ACTVE_SELECTIONUP, "MODIFIER_ACTVE_SELECTIONUP" },
	{ MODIFIER_ACTVE_SELECTIONDOWN, "MODIFIER_ACTVE_SELECTIONDOWN" },
	{ MODIFIER_ACTVE_MOVEFIRST, "MODIFIER_ACTVE_MOVEFIRST" },
	{ MODIFIER_ACTVE_MOVELAST, "MODIFIER_ACTVE_MOVELAST" },
	{ CANCELFILTER, "Cancel Filter" },
	{ HOMELIST, "HOMELIST" },
	{ BACKGRROUP, "Back Group" },
	{ TOGGLESHOWPERSISTANT, "Toggle Show Persistent" },
	{ PASTE_SELECTED, "Paste Selected" },
	{ DELETE_SELECTED, "Delete Selected" },
	{ CLIP_PROPERTIES, "Clip Properties" },
	{ PASTE_SELECTED_PLAIN_TEXT, "Paste Selected Plain Text" },
	{ MOVE_CLIP_TO_GROUP, "Move Clip To Group" },
	{ ELEVATE_PRIVlEGES, "Option - Elevate Privileges" },
	{ SHOW_IN_TASKBAR, "Option - Show In TaskBar" },
	{ COMPARE_SELECTED_CLIPS, "Compare Selected Clips" },
	{ SELECT_LEFT_SIDE_COMPARE, "Select Left File For Compare" },
	{ SELECT_RIGHT_SITE_AND_DO_COMPARE, "Select Right File And Do Compare" },
	{ EXPORT_TO_TEXT_FILE, "Export To Text File" },
	{ EXPORT_TO_QR_CODE, "Export To QR Code" },
	{ EXPORT_TO_BITMAP_FILE, "Export To Image File" },
	{ SAVE_CURRENT_CLIPBOARD, "Save Current Clipboard" },
	{ MOVE_CLIP_UP, "Move Clip Up" },
	{ MOVE_CLIP_DOWN, "Move Clip Down" },
	{ MOVE_CLIP_TOP, "Move Clip Top" },
	{ FILTER_ON_SELECTED_CLIP, "Filter On Selected Clip" },
	{ PASTE_UPPER_CASE, "Paste Upper Case" },
	{ PASTE_LOWER_CASE, "Paste Lower Case" },
	{ PASTE_CAPITALiZE, "Paste Capitalize" },
	{ PASTE_SENTENCE_CASE, "Paste Sentence Case" },
	{ PASTE_REMOVE_LINE_FEEDS, "Paste Remove Line Feeds" },
	{ PASTE_ADD_ONE_LINE_FEED, "Paste Add One Line Feed" },
	{ PASTE_ADD_TWO_LINE_FEEDS, "Paste Add Two Line Feeds" },
	{ PASTE_TYPOGLYCEMIA, "Paste Typoglycemia" },
	{ PASTE_POSITION_1, "Paste Position 1" },
	{ PASTE_POSITION_2, "Paste Position 2" },
	{ PASTE_POSITION_3, "Paste Position 3" },
	{ PASTE_POSITION_4, "Paste Position 4" },
	{ PASTE_POSITION_5, "Paste Position 5" },
	{ PASTE_POSITION_6, "Paste Position 6" },
	{ PASTE_POSITION_7, "Paste Position 7" },
	{ PASTE_POSITION_8, "Paste Position 8" },
	{ PASTE_POSITION_9, "Paste Position 9" },
	{ PASTE_POSITION_10, "Paste Position 10" },
	{ CONFIG_SHOW_FIRST_TEN_TEXT, "Option - Show text for first ten copy hot keys" },
	{ CONFIG_SHOW_CLIP_WAS_PASTED, "Option - Show indicator a clip has been pasted" },
	{ TOGGLE_LAST_GROUP_TOGGLE, "Toggle Last Group Toggle" },
	{ MAKE_TOP_STICKY, "Make Top Sticky Clip" },
	{ MAKE_LAST_STICKY, "Make Last Sticky Clip" },
	{ REMOVE_STICKY, "Remove Sticky Setting" },
	{ PASTE_ADD_CURRENT_TIME, "Paste Add Current Time" },
	{ IMPORT_CLIP, "Import Clip" },
	{ GLOBAl_HOTKEYS, "Global HotKeys" },
	{ DELETE_CLIP_DATA, "Delete Clip Data" },
	{ REPLACE_TOP_STICKY_CLIP, "Replace Top Sticky Clip" },
	{ SAVE_CF_HDROP_FIlE_DATA, "Save copied file (cf_hdrop) contents into Ditto" },
	{ TOGGLE_CLIPBOARD_CONNECTION, "Toggle clipboard connection" },
	{ MOVE_SELECTION_UP, "Move Selection Up" },
	{ MOVE_SELECTION_DOWN, "Move Selection Down" },
	{ TOGGLE_DESCRIPTION_WORD_WRAP, "Toggle Description Word Wrap" },
	{ APPLY_LAST_SEARCH, "Apply Last Search" },
	{ TOGGLE_SEARCH_METHOD, "Toggle Search Method" },
	{ MOVE_CLIP_LAST, "Move Clip Last" },
	{ PASTE_DONT_MOVE_CLIP, "Paste, Don't Change Clip Order" },
	{ PASTE_TRIM_WHITE_SPACE, "Paste, Trim White Space" },
	{ PASTE_POSIXIFY_PATHS, "Paste, Posixify Paths" },
	{ TRANSPARENCY_NONE, "Set Transparency None" },
	{ TRANSPARENCY_5, "Set Transparency 5%" },
	{ TRANSPARENCY_10, "Set Transparency 10%" },
	{ TRANSPARENCY_15, "Set Transparency 15%" },
	{ TRANSPARENCY_20, "Set Transparency 20%" },
	{ TRANSPARENCY_25, "Set Transparency 25%" },
	{ TRANSPARENCY_30, "Set Transparency 30%" },
	{ TRANSPARENCY_35, "Set Transparency 35%" },
	{ TRANSPARENCY_40, "Set Transparency 40%" },
	{ TRANSPARENCY_TOGGLE, "Toggle Transparency Enabled" },
	{ TRANSPARENCY_INCREASE, "Increase Transparency %" },
	{ TRANSPARENCY_DECREASE, "Decrease Transparency %" },
	{ SLUGIFY, "Slugify" },
	{ INVERT_CASE, "Invert Case" },
	{ COPY_SELECTION, "Copy Selection" },
	{ FORCE_CLOSE_WINDOW, "Force Close Window" },
	{ REFRESH_LIST, "Refresh List" },
	{ DELETE_ALL_NON_USED_CLIPS, "Delete all non used clips" },
	{ SET_DRAG_FILE_NAME, "Set Drag File Name" },
	{ PASTE_CAMEL_CASE, "Paste CamelCase" },
	{ PASTE_MULTI_IMAGE_HORIZONTAL, "Paste Muliple Images Horizontally" },
	{ PASTE_MULTI_IMAGE_VERTICAL, "Paste Muliple Images Vertically" },
	{ ASCII_TEXT_ONLY, "Asci Text Only" },
	{ PASTE_POSITION_1_PLAIN_TEXT, "Paste Position 1 Plain Text Only" },
	{ PASTE_POSITION_2_PLAIN_TEXT, "Paste Position 2 Plain Text Only" },
	{ PASTE_POSITION_3_PLAIN_TEXT, "Paste Position 3 Plain Text Only" },
	{ PASTE_POSITION_4_PLAIN_TEXT, "Paste Position 4 Plain Text Only" },
	{ PASTE_POSITION_5_PLAIN_TEXT, "Paste Position 5 Plain Text Only" },
	{ PASTE_POSITION_6_PLAIN_TEXT, "Paste Position 6 Plain Text Only" },
	{ PASTE_POSITION_7_PLAIN_TEXT, "Paste Position 7 Plain Text Only" },
	{ PASTE_POSITION_8_PLAIN_TEXT, "Paste Position 8 Plain Text Only" },
	{ PASTE_POSITION_9_PLAIN_TEXT, "Paste Position 9 Plain Text Only" },
	{ PASTE_POSITION_10_PLAIN_TEXT, "Paste Position 10 Plain Text Only" },
	{ GENERATE_GUID, "Generate GUID" },
	{ PASTE_AS_IMAGE, "Paste as Image" },
	{ SHOW_STARRED_CLIPS, "Show Starred Clips" },
} };

const std::array<ActionEnums::DefaultShortcut, 31> ActionEnums::s_defaultShortcutsFirst{ {
	{ ActionEnums::SHOWDESCRIPTION, VK_F3 },
	{ ActionEnums::NEXTDESCRIPTION, 'N' },
	{ ActionEnums::PREVDESCRIPTION, 'P' },
	{ ActionEnums::NEWGROUP, CAccels::MakeKey(VK_F7, HOTKEYF_CONTROL) },
	{ ActionEnums::NEWGROUPSELECTION, VK_F7 },
	{ ActionEnums::SHOWGROUPS, CAccels::MakeKey('G', HOTKEYF_CONTROL) },
	{ ActionEnums::NEWCLIP, CAccels::MakeKey('N', HOTKEYF_CONTROL) },
	{ ActionEnums::EDITCLIP, CAccels::MakeKey('E', HOTKEYF_CONTROL) },
	{ ActionEnums::CANCELFILTER, CAccels::MakeKey('C', HOTKEYF_ALT) },
	{ ActionEnums::TOGGLESHOWPERSISTANT, CAccels::MakeKey(VK_SPACE, HOTKEYF_CONTROL) },
	{ ActionEnums::CLIP_PROPERTIES, CAccels::MakeKey(VK_RETURN, HOTKEYF_ALT) },
	{ ActionEnums::PASTE_SELECTED_PLAIN_TEXT, CAccels::MakeKey(VK_RETURN, HOTKEYF_SHIFT) },
	{ ActionEnums::COMPARE_SELECTED_CLIPS, CAccels::MakeKey(VK_F2, HOTKEYF_CONTROL) },
	{ ActionEnums::PASTE_SELECTED, VK_RETURN },
	{ ActionEnums::SHOWMENU, CMouseKey::RightClick },
	{ PASTE_POSITION_1, CAccels::MakeKey('1', HOTKEYF_CONTROL) },
	{ PASTE_POSITION_2, CAccels::MakeKey('2', HOTKEYF_CONTROL) },
	{ PASTE_POSITION_3, CAccels::MakeKey('3', HOTKEYF_CONTROL) },
	{ PASTE_POSITION_4, CAccels::MakeKey('4', HOTKEYF_CONTROL) },
	{ PASTE_POSITION_5, CAccels::MakeKey('5', HOTKEYF_CONTROL) },
	{ PASTE_POSITION_6, CAccels::MakeKey('6', HOTKEYF_CONTROL) },
	{ PASTE_POSITION_7, CAccels::MakeKey('7', HOTKEYF_CONTROL) },
	{ PASTE_POSITION_8, CAccels::MakeKey('8', HOTKEYF_CONTROL) },
	{ PASTE_POSITION_9, CAccels::MakeKey('9', HOTKEYF_CONTROL) },
	{ PASTE_POSITION_10, CAccels::MakeKey('0', HOTKEYF_CONTROL) },
	{ CLOSEWINDOW, VK_ESCAPE },
	{ FORCE_CLOSE_WINDOW, CAccels::MakeKey(VK_ESCAPE, HOTKEYF_SHIFT) },
	{ TOGGLE_DESCRIPTION_WORD_WRAP, 'W' },
	{ COPY_SELECTION, CAccels::MakeKey('C', HOTKEYF_CONTROL) },
	{ REFRESH_LIST, VK_F5 },
	{ SET_DRAG_FILE_NAME, VK_F4 },
} };

const std::array<ActionEnums::DefaultShortcut, 11> ActionEnums::s_defaultShortcutsSecond{ {
	{ ActionEnums::PASTE_SELECTED, CMouseKey::DoubleClick },
	{ PASTE_POSITION_1, CAccels::MakeKey(VK_NUMPAD1, HOTKEYF_CONTROL) },
	{ PASTE_POSITION_2, CAccels::MakeKey(VK_NUMPAD2, HOTKEYF_CONTROL) },
	{ PASTE_POSITION_3, CAccels::MakeKey(VK_NUMPAD3, HOTKEYF_CONTROL) },
	{ PASTE_POSITION_4, CAccels::MakeKey(VK_NUMPAD4, HOTKEYF_CONTROL) },
	{ PASTE_POSITION_5, CAccels::MakeKey(VK_NUMPAD5, HOTKEYF_CONTROL) },
	{ PASTE_POSITION_6, CAccels::MakeKey(VK_NUMPAD6, HOTKEYF_CONTROL) },
	{ PASTE_POSITION_7, CAccels::MakeKey(VK_NUMPAD7, HOTKEYF_CONTROL) },
	{ PASTE_POSITION_8, CAccels::MakeKey(VK_NUMPAD8, HOTKEYF_CONTROL) },
	{ PASTE_POSITION_9, CAccels::MakeKey(VK_NUMPAD9, HOTKEYF_CONTROL) },
	{ PASTE_POSITION_10, CAccels::MakeKey(VK_NUMPAD0, HOTKEYF_CONTROL) },
} };

CString ActionEnums::EnumDescription(ActionEnumValues value, CMultiLanguage& language)
{
	CString val = _T("");

	for (const ActionDescription& description : s_descriptions)
	{
		if (description.action == value)
		{
			val = description.text;
			break;
		}
	}

	CString translatedValue = language.GetQuickPasteKeyboardString(value, val);

	if (translatedValue != _T(""))
	{
		return translatedValue;
	}

	return val;
}

int ActionEnums::GetDefaultShortCutKeyA(ActionEnumValues value, int pos)
{
	switch (pos)
	{
	case 0:
		return FindDefaultShortcut(s_defaultShortcutsFirst, value);
	case 1:
		return FindDefaultShortcut(s_defaultShortcutsSecond, value);
	}

	return -1;
}

int ActionEnums::FindDefaultShortcut(std::span<const DefaultShortcut> shortcuts, ActionEnumValues value)
{
	for (const DefaultShortcut& shortcut : shortcuts)
	{
		if (shortcut.action == value)
		{
			return shortcut.key;
		}
	}

	return -1;
}

int ActionEnums::GetDefaultShortCutKeyB(ActionEnumValues /*value*/, int pos)
{
	switch (pos)
	{
	case 0:
		break;
	}

	return -1;
}

bool ActionEnums::UserConfigurable(ActionEnumValues value)
{
	// Actions bound internally, never offered in the shortcut editor
	static const std::array internalOnly{
		NEXTTABCONTROL, PREVTABCONTROL,
		MODIFIER_ACTVE_SELECTIONUP, MODIFIER_ACTVE_SELECTIONDOWN, MODIFIER_ACTVE_MOVEFIRST, MODIFIER_ACTVE_MOVELAST,
		BACKGRROUP, DELETE_SELECTED, TOGGLEFILELOGGING, TOGGLEOUTPUTDEBUGSTRING, HOMELIST
	};

	const bool internal{ std::find(internalOnly.begin(), internalOnly.end(), value) != internalOnly.end() };
	return !internal && !Removed(value);
}

bool ActionEnums::Removed(ActionEnumValues value)
{
	static const std::array removed{
		SEND_TO_FRIEND_1, SEND_TO_FRIEND_2, SEND_TO_FRIEND_3, SEND_TO_FRIEND_4, SEND_TO_FRIEND_5,
		SEND_TO_FRIEND_6, SEND_TO_FRIEND_7, SEND_TO_FRIEND_8, SEND_TO_FRIEND_9, SEND_TO_FRIEND_10,
		SEND_TO_FRIEND_11, SEND_TO_FRIEND_12, SEND_TO_FRIEND_13, SEND_TO_FRIEND_14, SEND_TO_FRIEND_15,
		PROMPT_SEND_TO_FRIEND, EXPORT_TO_GOOGLE_TRANSLATE, EXPORT_TO_WEB_SEARCH,
		EMAILTO_BODY, EMAILTO_ATTACH_EXPORT, EMAILTO_ATTACH_CONTENT, GMAIL,
		PASTE_SCRIPT
	};

	return std::find(removed.begin(), removed.end(), value) != removed.end();
}

bool ActionEnums::ToolTipAction(ActionEnumValues value)
{
	switch (value)
	{
	case ActionEnums::NEXTDESCRIPTION:
	case ActionEnums::PREVDESCRIPTION:
	case ActionEnums::TOGGLESHOWPERSISTANT:
	case ActionEnums::TOGGLE_DESCRIPTION_WORD_WRAP:
	case ActionEnums::CLOSEWINDOW:
	case ActionEnums::SHOWDESCRIPTION:

		return true;
	}

	return false;
}
