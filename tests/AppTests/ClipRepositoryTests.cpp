/**
 * @file ClipRepositoryTests.cpp
 * @brief Tests of CClipRepository against an in-memory SQLite database.
 */
#include "stdafx.h"
#include "ClipRepository.h"
#include "TestDatabase.h"

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

namespace
{
	ClipRecord Clip(const CString& description, double clipOrder)
	{
		ClipRecord clip{};
		clip.description = description;
		clip.time = 1700000000LL;
		clip.clipOrder = clipOrder;
		return clip;
	}

	FormatRecord Format(const CString& name, const std::string& data)
	{
		FormatRecord format{};
		format.name = name;
		format.data.resize(data.size());
		std::memcpy(format.data.data(), data.data(), data.size());
		return format;
	}

	std::string Text(const std::vector<std::byte>& data)
	{
		return std::string(reinterpret_cast<const char*>(data.data()), data.size());
	}

	using Column = CClipRepository::OrderColumn;
}

// Regression: upstream doubled the quotes of the description and quick-paste text in memory
// to build its SQL, and printed the orders with %f (6 decimals).
TEST(ClipRepository, StoresClipExactly)
{
	TestDatabase test;
	CClipRepository repository(test.Db());
	ClipRecord clip = Clip(_T("it's \"quoted\""), 1.0000001);
	clip.quickPaste = _T("o'k");
	clip.parentId = 7;
	clip.crc = 0xDEADBEEF;
	clip.time = 5000000000LL;
	clip.stickyClipOrder = 2.5;

	const int id = repository.InsertClip(clip);
	const std::optional<ClipRecord> loaded = repository.LoadClip(id);

	ASSERT_TRUE(loaded.has_value());
	EXPECT_EQ(loaded->id, id);
	EXPECT_EQ(loaded->description, clip.description);
	EXPECT_EQ(loaded->quickPaste, clip.quickPaste);
	EXPECT_EQ(loaded->parentId, 7);
	EXPECT_EQ(loaded->crc, 0xDEADBEEFu);
	EXPECT_EQ(loaded->time, 5000000000LL);
	EXPECT_DOUBLE_EQ(loaded->clipOrder, 1.0000001);
	EXPECT_DOUBLE_EQ(loaded->stickyClipOrder, 2.5);
}

TEST(ClipRepository, UpdatesClip)
{
	TestDatabase test;
	CClipRepository repository(test.Db());
	ClipRecord clip = Clip(_T("first"), 1);
	clip.id = repository.InsertClip(clip);

	clip.description = _T("second");
	clip.shortCut = 65;
	clip.clipGroupOrder = 3.25;
	repository.UpdateClip(clip);
	repository.UpdateCrc(clip.id, 42);

	const ClipRecord loaded = repository.LoadClip(clip.id).value();
	EXPECT_EQ(loaded.description, _T("second"));
	EXPECT_EQ(loaded.shortCut, 65);
	EXPECT_DOUBLE_EQ(loaded.clipGroupOrder, 3.25);
	EXPECT_EQ(loaded.crc, 42u);

	repository.UpdateDescription(clip.id, _T("third"));
	EXPECT_EQ(repository.LoadClip(clip.id)->description, _T("third"));
}

// Regression: the copy properties dialog cleared other clips' move-to-group hot key by the
// clip's paste hot key, so a duplicate move-to-group key stayed and the paste key's owner lost
// its move-to-group key.
TEST(ClipRepository, ReleasesEachShortCutByItsOwnValue)
{
	TestDatabase test;
	CClipRepository repository(test.Db());
	ClipRecord kept = Clip(_T("kept"), 1);
	kept.shortCut = 65;
	kept.moveToGroupShortCut = 66;
	kept.id = repository.InsertClip(kept);
	ClipRecord samePaste = Clip(_T("same paste key"), 2);
	samePaste.shortCut = 65;
	samePaste.moveToGroupShortCut = 65;
	samePaste.id = repository.InsertClip(samePaste);
	ClipRecord sameMove = Clip(_T("same move key"), 3);
	sameMove.shortCut = 66;
	sameMove.moveToGroupShortCut = 66;
	sameMove.id = repository.InsertClip(sameMove);

	repository.ReleaseShortCuts(kept.id, { .paste = 65, .moveToGroup = 66 });

	const ClipRecord keptLoaded = repository.LoadClip(kept.id).value();
	const ClipRecord samePasteLoaded = repository.LoadClip(samePaste.id).value();
	const ClipRecord sameMoveLoaded = repository.LoadClip(sameMove.id).value();
	EXPECT_EQ(keptLoaded.shortCut, 65);
	EXPECT_EQ(keptLoaded.moveToGroupShortCut, 66);
	EXPECT_EQ(samePasteLoaded.shortCut, 0);
	EXPECT_EQ(samePasteLoaded.moveToGroupShortCut, 65);
	EXPECT_EQ(sameMoveLoaded.shortCut, 66);
	EXPECT_EQ(sameMoveLoaded.moveToGroupShortCut, 0);
}

TEST(ClipRepository, ReleasingNoShortCutsChangesNothing)
{
	TestDatabase test;
	CClipRepository repository(test.Db());
	ClipRecord other = Clip(_T("other"), 1);
	other.shortCut = 65;
	other.moveToGroupShortCut = 66;
	other.id = repository.InsertClip(other);

	repository.ReleaseShortCuts(other.id + 1, {});

	const ClipRecord loaded = repository.LoadClip(other.id).value();
	EXPECT_EQ(loaded.shortCut, 65);
	EXPECT_EQ(loaded.moveToGroupShortCut, 66);
}

TEST(ClipRepository, MissingClipIsEmpty)
{
	TestDatabase test;

	EXPECT_FALSE(CClipRepository(test.Db()).LoadClip(99).has_value());
}

TEST(ClipRepository, StoresFormatsNewestFirst)
{
	TestDatabase test;
	CClipRepository repository(test.Db());
	const int clipId = repository.InsertClip(Clip(_T("c"), 1));
	const std::vector<FormatRecord> formats{ Format(_T("CF_TEXT"), std::string("text\0", 5)), Format(_T("PNG"), "png") };

	const std::vector<int> ids = repository.InsertFormats(clipId, formats);
	const std::vector<FormatRecord> loaded = repository.LoadFormats(clipId, {});

	ASSERT_EQ(ids.size(), 2u);
	EXPECT_LT(ids[0], ids[1]);
	ASSERT_EQ(loaded.size(), 2u);
	EXPECT_EQ(loaded[0].name, _T("PNG"));
	EXPECT_EQ(loaded[0].dataId, ids[1]);
	EXPECT_EQ(Text(loaded[1].data), std::string("text\0", 5));
	EXPECT_EQ(repository.LoadFormatNames(clipId), (std::vector<CString>{ _T("PNG"), _T("CF_TEXT") }));
}

TEST(ClipRepository, FiltersFormatsByNameAndRow)
{
	TestDatabase test;
	CClipRepository repository(test.Db());
	const int clipId = repository.InsertClip(Clip(_T("c"), 1));
	const std::vector<FormatRecord> formats{ Format(_T("CF_TEXT"), "a"), Format(_T("PNG"), "b"), Format(_T("CF_UNICODETEXT"), "c") };
	const std::vector<int> ids = repository.InsertFormats(clipId, formats);

	CClipRepository::FormatFilter byName{};
	byName.names = { _T("CF_TEXT"), _T("CF_UNICODETEXT") };
	CClipRepository::FormatFilter byRow{};
	byRow.dataId = ids[1];

	EXPECT_EQ(repository.LoadFormats(clipId, byName).size(), 2u);
	ASSERT_EQ(repository.LoadFormats(clipId, byRow).size(), 1u);
	EXPECT_EQ(repository.LoadFormats(clipId, byRow)[0].name, _T("PNG"));
	EXPECT_EQ(Text(repository.LoadFormat(clipId, _T("PNG")).value()), "b");
	EXPECT_FALSE(repository.LoadFormat(clipId, _T("HTML Format")).has_value());
}

// Regression: upstream gave a format saved without data the previous format's memory handle,
// so two formats owned one block and it was freed twice.
TEST(ClipRepository, LeavesOutFormatWithoutData)
{
	TestDatabase test;
	CClipRepository repository(test.Db());
	const int clipId = repository.InsertClip(Clip(_T("c"), 1));
	repository.InsertFormats(clipId, std::vector<FormatRecord>{ Format(_T("CF_TEXT"), "a") });
	CString insertNull;
	insertNull.Format(_T("insert into Data values (NULL, %d, 'PNG', NULL);"), clipId);
	test.Db().execDML(insertNull);

	const std::vector<FormatRecord> loaded = repository.LoadFormats(clipId, {});

	ASSERT_EQ(loaded.size(), 1u);
	EXPECT_EQ(loaded[0].name, _T("CF_TEXT"));
	EXPECT_EQ(test.Log().size(), 1u);
	EXPECT_FALSE(repository.LoadFormat(clipId, _T("PNG")).has_value());
}

TEST(ClipRepository, DeletesFormats)
{
	TestDatabase test;
	CClipRepository repository(test.Db());
	const int clipId = repository.InsertClip(Clip(_T("c"), 1));
	const std::vector<int> ids = repository.InsertFormats(clipId, std::vector<FormatRecord>{ Format(_T("A"), "a"), Format(_T("B"), "b") });

	repository.DeleteFormat(ids[0]);
	EXPECT_EQ(repository.LoadFormatNames(clipId), (std::vector<CString>{ _T("B") }));

	repository.DeleteFormats(clipId);
	EXPECT_TRUE(repository.LoadFormatNames(clipId).empty());
}

TEST(ClipRepository, FindsClipByCrc)
{
	TestDatabase test;
	CClipRepository repository(test.Db());
	ClipRecord clip = Clip(_T("c"), 1);
	clip.crc = 0x80000001;
	const int id = repository.InsertClip(clip);

	EXPECT_EQ(repository.FindByCrc(0x80000001), id);
	EXPECT_FALSE(repository.FindByCrc(5).has_value());
}

TEST(ClipRepository, FindsEdgeOrders)
{
	TestDatabase test;
	CClipRepository repository(test.Db());
	repository.InsertClip(Clip(_T("a"), 1));
	repository.InsertClip(Clip(_T("b"), 5));
	ClipRecord inGroup = Clip(_T("g"), 100);
	inGroup.parentId = 3;
	inGroup.clipGroupOrder = 9;
	repository.InsertClip(inGroup);

	EXPECT_EQ(repository.EdgeOrder(Column::Clip, false, std::nullopt, true), 100.0);
	EXPECT_EQ(repository.EdgeOrder(Column::Clip, false, std::nullopt, false), 1.0);
	EXPECT_EQ(repository.EdgeOrder(Column::ClipGroup, false, 3, true), 9.0);
	EXPECT_FALSE(repository.EdgeOrder(Column::ClipGroup, false, 4, true).has_value());
	EXPECT_FALSE(repository.EdgeOrder(Column::StickyClip, true, std::nullopt, true).has_value());
}

TEST(ClipRepository, FindsNearestOrderAmongTheSameKind)
{
	TestDatabase test;
	CClipRepository repository(test.Db());
	repository.InsertClip(Clip(_T("a"), 1));
	repository.InsertClip(Clip(_T("b"), 2));
	ClipRecord sticky = Clip(_T("s"), 3);
	sticky.stickyClipOrder = 10;
	repository.InsertClip(sticky);
	repository.InsertClip(Clip(_T("c"), 4));

	// the sticky clip (order 3) is not a neighbour of the non-sticky ones
	EXPECT_EQ(repository.NearestOrder(Column::Clip, false, std::nullopt, 2, true), 4.0);
	EXPECT_EQ(repository.NearestOrder(Column::Clip, false, std::nullopt, 2, false), 1.0);
	EXPECT_FALSE(repository.NearestOrder(Column::Clip, false, std::nullopt, 4, true).has_value());
}

TEST(ClipRepository, FindsTopStickyAndClearsIt)
{
	TestDatabase test;
	CClipRepository repository(test.Db());
	ClipRecord low = Clip(_T("low"), 1);
	low.stickyClipOrder = 1;
	ClipRecord high = Clip(_T("high"), 2);
	high.stickyClipOrder = 2;
	repository.InsertClip(low);
	const int highId = repository.InsertClip(high);

	EXPECT_EQ(repository.TopStickyClipId(std::nullopt), highId);
	EXPECT_TRUE(repository.SetOrder(highId, Column::StickyClip, -2147483647.0));
	EXPECT_NE(repository.TopStickyClipId(std::nullopt), highId);
	EXPECT_FALSE(repository.SetOrder(999, Column::StickyClip, 1));
}
