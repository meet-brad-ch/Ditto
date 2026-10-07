#include "stdafx.h"
#include "ClipRepository.h"

#include <cstring>
#include <stdexcept>

CClipRepository::CClipRepository(CDittoDb& db) :
	m_db(db)
{
}

int CClipRepository::InsertClip(const ClipRecord& clip)
{
	CppSQLite3Statement insert = m_db.compileStatement(
		_T("INSERT into Main (lDate, mText, lShortCut, lDontAutoDelete, CRC, bIsGroup, lParentID, QuickPasteText, clipOrder, clipGroupOrder, globalShortCut, lastPasteDate, stickyClipOrder, stickyClipGroupOrder, MoveToGroupShortCut, GlobalMoveToGroupShortCut) ")
		_T("values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"));
	insert.bindInt64(1, clip.time);
	insert.bind(2, clip.description);
	insert.bind(3, clip.shortCut);
	insert.bind(4, clip.dontAutoDelete);
	insert.bind(5, static_cast<int>(clip.crc));
	insert.bind(6, clip.isGroup);
	insert.bind(7, clip.parentId);
	insert.bind(8, clip.quickPaste);
	insert.bind(9, clip.clipOrder);
	insert.bind(10, clip.clipGroupOrder);
	insert.bind(11, clip.globalShortCut);
	insert.bindInt64(12, clip.lastPasteDate);
	insert.bind(13, clip.stickyClipOrder);
	insert.bind(14, clip.stickyClipGroupOrder);
	insert.bind(15, clip.moveToGroupShortCut);
	insert.bind(16, clip.globalMoveToGroupShortCut);
	return static_cast<int>(m_db.InsertReturningId(insert));
}

void CClipRepository::UpdateClip(const ClipRecord& clip)
{
	CppSQLite3Statement update = m_db.compileStatement(_T("UPDATE Main SET lShortCut = ?, mText = ?, lParentID = ?, ")
		_T("lDontAutoDelete = ?, QuickPasteText = ?, clipOrder = ?, clipGroupOrder = ?, globalShortCut = ?, ")
		_T("stickyClipOrder = ?, stickyClipGroupOrder = ?, MoveToGroupShortCut = ?, GlobalMoveToGroupShortCut = ? ")
		_T("WHERE lID = ?;"));
	update.bind(1, clip.shortCut);
	update.bind(2, clip.description);
	update.bind(3, clip.parentId);
	update.bind(4, clip.dontAutoDelete);
	update.bind(5, clip.quickPaste);
	update.bind(6, clip.clipOrder);
	update.bind(7, clip.clipGroupOrder);
	update.bind(8, clip.globalShortCut);
	update.bind(9, clip.stickyClipOrder);
	update.bind(10, clip.stickyClipGroupOrder);
	update.bind(11, clip.moveToGroupShortCut);
	update.bind(12, clip.globalMoveToGroupShortCut);
	update.bind(13, clip.id);
	update.execDML();
}

void CClipRepository::UpdateDescription(int clipId, const CString& description)
{
	CppSQLite3Statement update = m_db.compileStatement(_T("UPDATE Main SET mText = ? WHERE lID = ?;"));
	update.bind(1, description);
	update.bind(2, clipId);
	update.execDML();
}

void CClipRepository::UpdateCrc(int clipId, DWORD crc)
{
	CppSQLite3Statement update = m_db.compileStatement(_T("UPDATE Main SET CRC = ? WHERE lID = ?;"));
	update.bind(1, static_cast<int>(crc));
	update.bind(2, clipId);
	update.execDML();
}

std::vector<int> CClipRepository::InsertFormats(int clipId, std::span<const FormatRecord> formats)
{
	std::vector<int> ids;
	CppSQLite3Statement insert = m_db.compileStatement(_T("insert into Data values (NULL, ?, ?, ?);"));
	for (const FormatRecord& format : formats)
	{
		insert.bind(1, clipId);
		insert.bind(2, format.name);
		insert.bind(3, reinterpret_cast<const unsigned char*>(format.data.data()), static_cast<int>(format.data.size()));
		ids.push_back(static_cast<int>(m_db.InsertReturningId(insert)));
		insert.reset();
	}
	return ids;
}

void CClipRepository::DeleteFormats(int clipId)
{
	CppSQLite3Statement remove = m_db.compileStatement(_T("DELETE FROM Data WHERE lParentID = ?;"));
	remove.bind(1, clipId);
	remove.execDML();
}

void CClipRepository::DeleteFormat(int dataId)
{
	CppSQLite3Statement remove = m_db.compileStatement(_T("DELETE FROM Data WHERE lID = ?;"));
	remove.bind(1, dataId);
	remove.execDML();
}

std::optional<ClipRecord> CClipRepository::LoadClip(int clipId)
{
	CppSQLite3Statement select = m_db.compileStatement(_T("SELECT * FROM Main WHERE lID = ?"));
	select.bind(1, clipId);
	CppSQLite3Query row = select.execQuery();
	if (row.eof())
	{
		return std::nullopt;
	}
	return ReadClip(row);
}

std::vector<FormatRecord> CClipRepository::LoadFormats(int clipId, const FormatFilter& filter)
{
	CString sql = _T("SELECT lID, lParentID, strClipBoardFormat, ooData FROM Data WHERE lParentID = ?");
	if (filter.names.empty() == false)
	{
		sql += _T(" AND strClipBoardFormat IN (?");
		for (std::size_t i = 1; i < filter.names.size(); i++)
		{
			sql += _T(", ?");
		}
		sql += _T(")");
	}
	if (filter.dataId)
	{
		sql += _T(" AND lID = ?");
	}
	// by Data.lID, so a CRC generated from the formats has the same order as the first time
	sql += _T(" ORDER BY lID desc");

	CppSQLite3Statement select = m_db.compileStatement(sql);
	int param = 1;
	select.bind(param++, clipId);
	for (const CString& name : filter.names)
	{
		select.bind(param++, name);
	}
	if (filter.dataId)
	{
		select.bind(param++, *filter.dataId);
	}

	std::vector<FormatRecord> formats;
	for (CppSQLite3Query row = select.execQuery(); row.eof() == false; row.nextRow())
	{
		FormatRecord format{};
		format.dataId = row.getIntField(0);
		format.parentId = row.getIntField(1);
		format.name = row.getStringField(2);
		int size = 0;
		const unsigned char* data = row.getBlobField(3, size);
		if (data == nullptr)
		{
			CString text;
			text.Format(_T("Clip %d has the format %s without data (Data row %d), left out"), clipId, format.name.GetString(), format.dataId);
			m_db.LogError(text);
			continue;
		}
		format.data.resize(static_cast<std::size_t>(size));
		std::memcpy(format.data.data(), data, format.data.size());
		formats.push_back(std::move(format));
	}
	return formats;
}

std::optional<std::vector<std::byte>> CClipRepository::LoadFormat(int clipId, const CString& name)
{
	FormatFilter filter{};
	filter.names.push_back(name);
	std::vector<FormatRecord> formats = LoadFormats(clipId, filter);
	if (formats.empty())
	{
		return std::nullopt;
	}
	return std::move(formats.front().data);
}

std::vector<CString> CClipRepository::LoadFormatNames(int clipId)
{
	CppSQLite3Statement select = m_db.compileStatement(_T("SELECT strClipBoardFormat FROM Data WHERE lParentID = ? ORDER BY lID desc"));
	select.bind(1, clipId);
	std::vector<CString> names;
	for (CppSQLite3Query row = select.execQuery(); row.eof() == false; row.nextRow())
	{
		names.emplace_back(row.getStringField(0));
	}
	return names;
}

std::optional<int> CClipRepository::FindByCrc(DWORD crc)
{
	CppSQLite3Statement select = m_db.compileStatement(_T("SELECT lID FROM Main WHERE CRC = ? LIMIT 1"));
	select.bind(1, static_cast<int>(crc));
	CppSQLite3Query row = select.execQuery();
	if (row.eof())
	{
		return std::nullopt;
	}
	return row.getIntField(0);
}

std::optional<double> CClipRepository::EdgeOrder(OrderColumn column, bool sticky, std::optional<int> parentId, bool highest)
{
	const CString name = ColumnName(column);
	CString sql;
	sql.Format(_T("SELECT %s FROM Main WHERE %s %s %s ORDER BY %s %s LIMIT 1"),
		name.GetString(),
		parentId ? _T("lParentID = ? AND") : _T(""),
		name.GetString(),
		sticky ? _T("<> -(2147483647)") : _T("notnull"),
		name.GetString(),
		highest ? _T("DESC") : _T("ASC"));
	CppSQLite3Statement select = m_db.compileStatement(sql);
	if (parentId)
	{
		select.bind(1, *parentId);
	}
	CppSQLite3Query row = select.execQuery();
	if (row.eof())
	{
		return std::nullopt;
	}
	return row.getFloatField(0);
}

std::optional<double> CClipRepository::NearestOrder(OrderColumn column, bool sticky, std::optional<int> parentId, double from, bool above)
{
	const CString name = ColumnName(column);
	CString sql;
	sql.Format(_T("SELECT %s FROM Main WHERE %s%s %s -(2147483647) AND %s %s ? ORDER BY %s %s LIMIT 1"),
		name.GetString(),
		parentId ? _T("lParentID = ? AND ") : _T(""),
		ColumnName(StickyColumnOf(column)),
		sticky ? _T("<>") : _T("="),
		name.GetString(),
		above ? _T(">") : _T("<"),
		name.GetString(),
		above ? _T("ASC") : _T("DESC"));
	CppSQLite3Statement select = m_db.compileStatement(sql);
	int param = 1;
	if (parentId)
	{
		select.bind(param++, *parentId);
	}
	select.bind(param, from);
	CppSQLite3Query row = select.execQuery();
	if (row.eof())
	{
		return std::nullopt;
	}
	return row.getFloatField(0);
}

std::optional<int> CClipRepository::TopStickyClipId(std::optional<int> parentId)
{
	const TCHAR* sql = parentId
		? _T("SELECT lID FROM Main WHERE lParentID = ? AND stickyClipGroupOrder <> -(2147483647) ORDER BY stickyClipGroupOrder DESC LIMIT 1")
		: _T("SELECT lID FROM Main WHERE stickyClipOrder <> -(2147483647) ORDER BY stickyClipOrder DESC LIMIT 1");
	CppSQLite3Statement select = m_db.compileStatement(sql);
	if (parentId)
	{
		select.bind(1, *parentId);
	}
	CppSQLite3Query row = select.execQuery();
	if (row.eof())
	{
		return std::nullopt;
	}
	return row.getIntField(0);
}

bool CClipRepository::SetOrder(int clipId, OrderColumn column, double order)
{
	CString sql;
	sql.Format(_T("UPDATE Main SET %s = ? WHERE lID = ?"), ColumnName(column));
	CppSQLite3Statement update = m_db.compileStatement(sql);
	update.bind(1, order);
	update.bind(2, clipId);
	return update.execDML() > 0;
}

const TCHAR* CClipRepository::ColumnName(OrderColumn column)
{
	switch (column)
	{
	case OrderColumn::Clip: return _T("clipOrder");
	case OrderColumn::ClipGroup: return _T("clipGroupOrder");
	case OrderColumn::StickyClip: return _T("stickyClipOrder");
	case OrderColumn::StickyClipGroup: return _T("stickyClipGroupOrder");
	}
	throw std::invalid_argument("unknown order column");
}

CClipRepository::OrderColumn CClipRepository::StickyColumnOf(OrderColumn column)
{
	const bool inGroup = column == OrderColumn::ClipGroup || column == OrderColumn::StickyClipGroup;
	return inGroup ? OrderColumn::StickyClipGroup : OrderColumn::StickyClip;
}

ClipRecord CClipRepository::ReadClip(CppSQLite3Query& row)
{
	ClipRecord clip{};
	clip.id = row.getIntField(_T("lID"));
	clip.time = row.getInt64Field(_T("lDate"));
	clip.description = row.getStringField(_T("mText"));
	clip.crc = static_cast<DWORD>(row.getIntField(_T("CRC")));
	clip.parentId = row.getIntField(_T("lParentID"));
	clip.dontAutoDelete = row.getIntField(_T("lDontAutoDelete"));
	clip.shortCut = row.getIntField(_T("lShortCut"));
	clip.isGroup = row.getIntField(_T("bIsGroup"));
	clip.quickPaste = row.getStringField(_T("QuickPasteText"));
	clip.clipOrder = row.getFloatField(_T("clipOrder"));
	clip.clipGroupOrder = row.getFloatField(_T("clipGroupOrder"));
	clip.globalShortCut = row.getIntField(_T("globalShortCut"));
	clip.lastPasteDate = row.getInt64Field(_T("lastPasteDate"));
	clip.stickyClipOrder = row.getFloatField(_T("stickyClipOrder"));
	clip.stickyClipGroupOrder = row.getFloatField(_T("stickyClipGroupOrder"));
	clip.moveToGroupShortCut = row.getIntField(_T("MoveToGroupShortCut"));
	clip.globalMoveToGroupShortCut = row.getIntField(_T("GlobalMoveToGroupShortCut"));
	return clip;
}
