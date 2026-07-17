#include <dispatcher/storage/history_sample_query_result.hpp>
#include <dispatcher/storage/history_storage.hpp>
#include <dispatcher/storage/history_storage_query.hpp>
#include <dispatcher/storage/storage_result.hpp>
#include <dispatcher/storage/storage_status.hpp>

#include <gtest/gtest.h>

#include <optional>
#include <vector>

namespace
{
    class FakeHistoryStorage final : public dispatcher::storage::HistoryStorage
    {
    public:
        [[nodiscard]] dispatcher::storage::StorageResult append(
            const dispatcher::history::HistorySample& sample
        ) override
        {
            (void)sample;

            ++append_count_;

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::StorageResult append_batch(
            const std::vector<dispatcher::history::HistorySample>& samples
        ) override
        {
            append_batch_count_ += samples.size();

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::HistorySampleQueryResult query(
            const dispatcher::storage::HistoryStorageQuery& query
        ) const override
        {
            last_query_ = query;

            return dispatcher::storage::HistorySampleQueryResult::success({});
        }

        [[nodiscard]] dispatcher::storage::StorageResult remove_by_tag(
            const dispatcher::domain::TagId& tag_id
        ) override
        {
            last_removed_tag_id_ = tag_id;

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::StorageResult clear() override
        {
            clear_called_ = true;

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] std::size_t append_count() const noexcept
        {
            return append_count_;
        }

        [[nodiscard]] std::size_t append_batch_count() const noexcept
        {
            return append_batch_count_;
        }

        [[nodiscard]] bool clear_called() const noexcept
        {
            return clear_called_;
        }

        [[nodiscard]] const std::optional<dispatcher::domain::TagId>&
            last_removed_tag_id() const noexcept
        {
            return last_removed_tag_id_;
        }

        [[nodiscard]] const std::optional<dispatcher::storage::HistoryStorageQuery>&
            last_query() const noexcept
        {
            return last_query_;
        }

    private:
        std::size_t append_count_{ 0 };
        std::size_t append_batch_count_{ 0 };
        bool clear_called_{ false };

        std::optional<dispatcher::domain::TagId> last_removed_tag_id_;
        mutable std::optional<dispatcher::storage::HistoryStorageQuery> last_query_;
    };
}

TEST(HistoryStorageQueryTests, DefaultQueryIsUnbounded)
{
    const dispatcher::storage::HistoryStorageQuery query;

    EXPECT_FALSE(query.has_tag());
    EXPECT_FALSE(query.has_from());
    EXPECT_FALSE(query.has_to());
    EXPECT_FALSE(query.has_limit());
    EXPECT_FALSE(query.has_time_range());
    EXPECT_FALSE(query.has_bounded_time_range());
    EXPECT_FALSE(query.requests_latest_only());
}

TEST(HistoryStorageQueryTests, QueryPredicatesReflectConfiguredFields)
{
    dispatcher::storage::HistoryStorageQuery query;

    query.tag_id = dispatcher::domain::TagId{ "tag-1" };
    query.from = dispatcher::storage::HistoryStorageQuery::Clock::now();
    query.to = dispatcher::storage::HistoryStorageQuery::Clock::now();
    query.limit = 100;
    query.latest_only = true;

    EXPECT_TRUE(query.has_tag());
    EXPECT_TRUE(query.has_from());
    EXPECT_TRUE(query.has_to());
    EXPECT_TRUE(query.has_limit());
    EXPECT_TRUE(query.has_time_range());
    EXPECT_TRUE(query.has_bounded_time_range());
    EXPECT_TRUE(query.requests_latest_only());
}

TEST(HistorySampleQueryResultTests, SuccessResultWorks)
{
    const auto result =
        dispatcher::storage::HistorySampleQueryResult::success({});

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::storage::StorageStatus::Success
    );

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.sample_count(), 0);
    EXPECT_TRUE(result.samples().empty());
}

TEST(HistorySampleQueryResultTests, FailureResultCapturesError)
{
    const auto result = dispatcher::storage::HistorySampleQueryResult::failure(
        dispatcher::storage::StorageStatus::BackendUnavailable,
        "history.query",
        "tag-1",
        "backend is down"
    );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::storage::StorageStatus::BackendUnavailable
    );

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.sample_count(), 0);

    EXPECT_EQ(result.error().operation, "history.query");
    EXPECT_EQ(result.error().key, "tag-1");
    EXPECT_EQ(result.error().message, "backend is down");
}

TEST(HistoryStorageInterfaceTests, InterfaceCanBeImplemented)
{
    FakeHistoryStorage storage;

    const dispatcher::storage::HistoryStorageQuery query{
        .tag_id = dispatcher::domain::TagId{"tag-1"},
        .limit = 10,
        .latest_only = true
    };

    const auto query_result = storage.query(query);

    EXPECT_TRUE(query_result.ok());
    EXPECT_TRUE(query_result.empty());

    ASSERT_TRUE(storage.last_query().has_value());
    EXPECT_TRUE(storage.last_query()->has_tag());
    EXPECT_TRUE(storage.last_query()->has_limit());
    EXPECT_TRUE(storage.last_query()->requests_latest_only());

    const auto remove_result = storage.remove_by_tag(
        dispatcher::domain::TagId{ "tag-1" }
    );

    EXPECT_TRUE(remove_result.ok());

    ASSERT_TRUE(storage.last_removed_tag_id().has_value());
    EXPECT_EQ(
        storage.last_removed_tag_id()->value(),
        "tag-1"
    );

    const auto clear_result = storage.clear();

    EXPECT_TRUE(clear_result.ok());
    EXPECT_TRUE(storage.clear_called());
}