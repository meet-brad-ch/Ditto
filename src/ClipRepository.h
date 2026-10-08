#pragma once

#include "ClipRecord.h"
#include "DittoDb.h"
#include "FormatRecord.h"

#include <optional>
#include <span>
#include <vector>

// The SQL of clips: the Main and Data tables, behind an injected database. Every value is bound,
// so texts are stored as they are and orders keep full precision. Database errors are thrown
// (CppSQLite3Exception); the callers decide how they are reported.
class CClipRepository
{
public:
	// A clip's position in a list; the column names are fixed here, never taken from outside
	enum class OrderColumn
	{
		Clip,
		ClipGroup,
		StickyClip,
		StickyClipGroup
	};

	// Which formats LoadFormats returns
	struct FormatFilter
	{
		// only these names; empty for all
		std::vector<CString> names{};
		// only this Data row; nullopt for all
		std::optional<int> dataId{};
	};

	// The hot keys of a clip; 0 for none
	struct ShortCuts
	{
		// pastes the clip
		int paste{};
		// moves the selected clips to this group
		int moveToGroup{};
	};

	// db: the connection; it must outlive the repository
	explicit CClipRepository(CDittoDb& db);

	// Inserts the Main row and returns its id
	int InsertClip(const ClipRecord& clip);
	// Rewrites the Main row of clip.id
	void UpdateClip(const ClipRecord& clip);
	void UpdateDescription(int clipId, const CString& description);
	void UpdateCrc(int clipId, DWORD crc);
	// A hot key belongs to one clip: takes each non-zero hot key of shortCuts away from every
	// clip other than clipId
	void ReleaseShortCuts(int clipId, const ShortCuts& shortCuts);
	// Inserts the formats of a clip and returns their Data ids, in the same order
	std::vector<int> InsertFormats(int clipId, std::span<const FormatRecord> formats);
	void DeleteFormats(int clipId);
	void DeleteFormat(int dataId);

	std::optional<ClipRecord> LoadClip(int clipId);
	// The formats of a clip, newest Data row first; a row saved without data is left out and logged
	std::vector<FormatRecord> LoadFormats(int clipId, const FormatFilter& filter);
	// The data of one format; nullopt when the clip has no such format or it has no data
	std::optional<std::vector<std::byte>> LoadFormat(int clipId, const CString& name);
	// The format names of a clip, newest Data row first
	std::vector<CString> LoadFormatNames(int clipId);

	// The id of a clip with this CRC, if there is one
	std::optional<int> FindByCrc(DWORD crc);

	// The highest or lowest order of a column, among a group's clips (parentId) or all clips;
	// among sticky clips only when sticky; nullopt when there is none
	std::optional<double> EdgeOrder(OrderColumn column, bool sticky, std::optional<int> parentId, bool highest);
	// The nearest order above (or below) from, among the clips of the same list that are sticky
	// (or not sticky) there; nullopt when there is none
	std::optional<double> NearestOrder(OrderColumn column, bool sticky, std::optional<int> parentId, double from, bool above);
	// The id of the top sticky clip of a group or the main list, if there is one
	std::optional<int> TopStickyClipId(std::optional<int> parentId);
	// Sets one order of a clip; returns whether a row changed
	bool SetOrder(int clipId, OrderColumn column, double order);

private:
	static const TCHAR* ColumnName(OrderColumn column);
	// The sticky column of the same list as column (main list or group)
	static OrderColumn StickyColumnOf(OrderColumn column);
	static ClipRecord ReadClip(CppSQLite3Query& row);

	CDittoDb& m_db;
};
