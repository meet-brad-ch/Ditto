/**
 * @file DittoDbTests.cpp
 * @brief Tests of CDittoDb (the lock) and CDittoDbTransaction (rollback and nesting).
 */
#include "stdafx.h"
#include "ClipRepository.h"
#include "DittoDbTransaction.h"
#include "TestDatabase.h"

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <set>
#include <thread>
#include <vector>

namespace
{
	int CountClips(CDittoDb& db)
	{
		return db.execScalar(_T("SELECT COUNT(*) FROM Main"));
	}

	ClipRecord Clip(const CString& description)
	{
		ClipRecord clip{};
		clip.description = description;
		return clip;
	}
}

TEST(DittoDbTransaction, CommitKeepsTheRows)
{
	TestDatabase test;
	{
		CDittoDbTransaction transaction(test.Db());
		CClipRepository(test.Db()).InsertClip(Clip(_T("a")));
		transaction.Commit();
	}

	EXPECT_EQ(CountClips(test.Db()), 1);
}

TEST(DittoDbTransaction, RollsBackWhenNotCommitted)
{
	TestDatabase test;
	{
		CDittoDbTransaction transaction(test.Db());
		CClipRepository(test.Db()).InsertClip(Clip(_T("a")));
	}

	EXPECT_EQ(CountClips(test.Db()), 0);
	EXPECT_EQ(test.Db().TransactionDepth(), 0);
}

// Regression: copying clips to a group opened a transaction and saved each clip with AddToDB,
// which opens its own; SQLite cannot nest BEGIN, so the copy failed. Inner transactions are
// savepoints now.
TEST(DittoDbTransaction, NestsInsideAnotherTransaction)
{
	TestDatabase test;
	CClipRepository repository(test.Db());
	{
		CDittoDbTransaction outer(test.Db());
		{
			CDittoDbTransaction kept(test.Db());
			repository.InsertClip(Clip(_T("kept")));
			kept.Commit();
		}
		{
			CDittoDbTransaction dropped(test.Db());
			repository.InsertClip(Clip(_T("dropped")));
		}
		EXPECT_EQ(test.Db().TransactionDepth(), 1);
		outer.Commit();
	}

	EXPECT_EQ(CountClips(test.Db()), 1);
	EXPECT_EQ(test.Db().execScalar(_T("SELECT COUNT(*) FROM Main WHERE mText = 'kept'")), 1);
}

TEST(DittoDbTransaction, OuterRollbackDropsCommittedInnerWork)
{
	TestDatabase test;
	{
		CDittoDbTransaction outer(test.Db());
		CDittoDbTransaction inner(test.Db());
		CClipRepository(test.Db()).InsertClip(Clip(_T("a")));
		inner.Commit();
	}

	EXPECT_EQ(CountClips(test.Db()), 0);
}

// Each thread's insert must get its own row's id; upstream read lastRowId after the insert
// without a lock, so another thread's insert in between gave the wrong id.
TEST(DittoDb, InsertsFromThreadsGetTheirOwnIds)
{
	TestDatabase test;
	CClipRepository repository(test.Db());
	constexpr int Threads = 4;
	constexpr int PerThread = 100;
	std::vector<std::future<bool>> results;
	for (int t = 0; t < Threads; t++)
	{
		results.push_back(std::async(std::launch::async, [&repository, t]() {
			for (int i = 0; i < PerThread; i++)
			{
				CString description;
				description.Format(_T("%d-%d"), t, i);
				const int id = repository.InsertClip(Clip(description));
				if (repository.LoadClip(id).value().description != description)
				{
					return false;
				}
			}
			return true;
		}));
	}

	for (std::future<bool>& result : results)
	{
		EXPECT_TRUE(result.get());
	}
	EXPECT_EQ(CountClips(test.Db()), Threads * PerThread);
}

// A statement from another thread waits for an open transaction instead of running inside it
// (and being rolled back with it).
TEST(DittoDb, StatementWaitsForAnotherThreadsTransaction)
{
	TestDatabase test;
	std::promise<void> transactionOpen;
	std::promise<void> finishTransaction;
	std::future<void> finish = finishTransaction.get_future();

	std::thread owner([&]() {
		CDittoDbTransaction transaction(test.Db());
		CClipRepository(test.Db()).InsertClip(Clip(_T("rolled back")));
		transactionOpen.set_value();
		finish.wait();
		// not committed: rolled back here
	});
	transactionOpen.get_future().wait();

	std::future<int> other = std::async(std::launch::async, [&]() {
		return test.Db().execDML(_T("INSERT INTO Main (mText) VALUES ('other');"));
	});
	EXPECT_EQ(other.wait_for(std::chrono::milliseconds(200)), std::future_status::timeout);

	finishTransaction.set_value();
	owner.join();
	EXPECT_EQ(other.get(), 1);
	EXPECT_EQ(test.Db().execScalar(_T("SELECT COUNT(*) FROM Main WHERE mText = 'other'")), 1);
	EXPECT_EQ(test.Db().execScalar(_T("SELECT COUNT(*) FROM Main WHERE mText = 'rolled back'")), 0);
}

// Regression: a prepared statement (compileStatement) ran without the connection lock, so from
// another thread it ran inside an open transaction and was rolled back with it.
TEST(DittoDb, PreparedStatementWaitsForAnotherThreadsTransaction)
{
	TestDatabase test;
	// compiled before the transaction: the test is about the step, not the compile
	CppSQLite3Statement insert = test.Db().compileStatement(_T("INSERT INTO Main (mText) VALUES ('other');"));
	std::promise<void> transactionOpen;
	std::promise<void> finishTransaction;
	std::future<void> finish = finishTransaction.get_future();

	std::thread owner([&]() {
		CDittoDbTransaction transaction(test.Db());
		CClipRepository(test.Db()).InsertClip(Clip(_T("rolled back")));
		transactionOpen.set_value();
		finish.wait();
		// not committed: rolled back here
	});
	transactionOpen.get_future().wait();

	std::future<int> other = std::async(std::launch::async, [&]() {
		return insert.execDML();
	});
	EXPECT_EQ(other.wait_for(std::chrono::milliseconds(200)), std::future_status::timeout);

	finishTransaction.set_value();
	owner.join();
	EXPECT_EQ(other.get(), 1);
	EXPECT_EQ(test.Db().execScalar(_T("SELECT COUNT(*) FROM Main WHERE mText = 'other'")), 1);
	EXPECT_EQ(test.Db().execScalar(_T("SELECT COUNT(*) FROM Main WHERE mText = 'rolled back'")), 0);
}
