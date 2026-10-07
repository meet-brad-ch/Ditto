#pragma once

// One row of the Main table: a clip's description, shortcuts and positions. CClip converts to
// and from it for CClipRepository.
struct ClipRecord
{
	int id{ -1 };
	int parentId{ -1 };
	CString description{};
	__int64 time{};
	int shortCut{};
	int dontAutoDelete{};
	DWORD crc{};
	BOOL isGroup{};
	CString quickPaste{};
	double clipOrder{};
	double clipGroupOrder{};
	BOOL globalShortCut{};
	__int64 lastPasteDate{};
	// -(2147483647) when the clip is not sticky
	double stickyClipOrder{ -2147483647.0 };
	double stickyClipGroupOrder{ -2147483647.0 };
	int moveToGroupShortCut{};
	BOOL globalMoveToGroupShortCut{};
};
