#include <dispatcher/storage/alarm_acknowledgement_query_result.hpp>
#include <dispatcher/storage/alarm_acknowledgement_storage.hpp>
#include <dispatcher/storage/alarm_acknowledgement_storage_query.hpp>
#include <dispatcher/storage/storage_result.hpp>
#include <dispatcher/storage/storage_status.hpp>

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

namespace
{
    class FakeAlarmAcknowledgementStorage final
        : public dispatcher::storage::AlarmAcknowledgementStorage
    {
    public:
        [[nodiscard]] dispatcher::storage::StorageResult append(
            const dispatcher::alarm::AlarmAcknowledgementRecord& record
        ) override
        {
            (void)record;

            ++append_count_;

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::StorageResult append_batch(
            const std::vector<dispatcher::alarm::AlarmAcknowledgementRecord>&
            records
        ) override
        {
            append_batch_count_ += records.size();

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::AlarmAcknowledgementQueryResult query(
            const dispatcher::storage::AlarmAcknowledgementStorageQuery& query
        ) const override
        {
            last_query_ = query;

            return dispatcher::storage::AlarmAcknowledgementQueryResult::success(
                {}
            );
        }

        [[nodiscard]] dispatcher::storage::StorageResult remove_by_alarm(
            const dispatcher::domain::AlarmId& alarm_id
        ) override
        {
            last_removed_alarm_id_ = alarm_id;

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::StorageResult remove_by_operator(
            const std::string& operator_id
        ) override
        {
            last_removed_operator_id_ = operator_id;

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

        [[nodiscard]] const std::optional<dispatcher::domain::AlarmId>&
            last_removed_alarm_id() const noexcept
        {
            return last_removed_alarm_id_;
        }

        [[nodiscard]] const std::optional<std::string>&
            last_removed_operator_id() const noexcept
        {
            return last_removed_operator_id_;
        }

        [[nodiscard]] const std::optional<
            dispatcher::storage::AlarmAcknowledgementStorageQuery
        >&
            last_query() const noexcept
        {
            return last_query_;
        }

    private:
        std::size_t append_count_{ 0 };
        std::size_t append_batch_count_{ 0 };
        bool clear_called_{ false };

        std::optional<dispatcher::domain::AlarmId> last_removed_alarm_id_;
        std::optional<std::string> last_removed_operator_id_;

        mutable std::optional<
            dispatcher::storage::AlarmAcknowledgementStorageQuery
        > last_query_;
    };
}

TEST(AlarmAcknowledgementStorageQueryTests, DefaultQueryIsUnbounded)
{
    const dispatcher::storage::AlarmAcknowledgementStorageQuery query;

    EXPECT_FALSE(query.has_alarm_id());
    EXPECT_FALSE(query.has_operator_id());
    EXPECT_FALSE(query.has_from());
    EXPECT_FALSE(query.has_to());
    EXPECT_FALSE(query.has_limit());
    EXPECT_FALSE(query.has_time_range());
    EXPECT_FALSE(query.has_bounded_time_range());
    EXPECT_FALSE(query.requests_latest_only());
}

TEST(AlarmAcknowledgementStorageQueryTests, QueryPredicatesReflectConfiguredFields)
{
    dispatcher::storage::AlarmAcknowledgementStorageQuery query;

    query.alarm_id = dispatcher::domain::AlarmId{ "alarm-1" };
    query.operator_id = "operator-1";
    query.from =
        dispatcher::storage::AlarmAcknowledgementStorageQuery::Clock::now();
    query.to =
        dispatcher::storage::AlarmAcknowledgementStorageQuery::Clock::now();
    query.limit = 100;
    query.latest_only = true;

    EXPECT_TRUE(query.has_alarm_id());
    EXPECT_TRUE(query.has_operator_id());
    EXPECT_TRUE(query.has_from());
    EXPECT_TRUE(query.has_to());
    EXPECT_TRUE(query.has_limit());
    EXPECT_TRUE(query.has_time_range());
    EXPECT_TRUE(query.has_bounded_time_range());
    EXPECT_TRUE(query.requests_latest_only());
}

TEST(AlarmAcknowledgementStorageQueryTests, EmptyOperatorIdIsNotConsideredConfigured)
{
    dispatcher::storage::AlarmAcknowledgementStorageQuery query;

    query.operator_id = "";

    EXPECT_FALSE(query.has_operator_id());
}

TEST(AlarmAcknowledgementQueryResultTests, SuccessResultWorks)
{
    const auto result =
        dispatcher::storage::AlarmAcknowledgementQueryResult::success({});

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::storage::StorageStatus::Success
    );

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.record_count(), 0);
    EXPECT_TRUE(result.records().empty());
}

TEST(AlarmAcknowledgementQueryResultTests, FailureResultCapturesError)
{
    const auto result =
        dispatcher::storage::AlarmAcknowledgementQueryResult::failure(
            dispatcher::storage::StorageStatus::BackendUnavailable,
            "alarm_acknowledgement.query",
            "alarm-1",
            "backend is down"
        );

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::storage::StorageStatus::BackendUnavailable
    );

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.record_count(), 0);

    EXPECT_EQ(result.error().operation, "alarm_acknowledgement.query");
    EXPECT_EQ(result.error().key, "alarm-1");
    EXPECT_EQ(result.error().message, "backend is down");
}

TEST(AlarmAcknowledgementStorageInterfaceTests, InterfaceCanBeImplemented)
{
    FakeAlarmAcknowledgementStorage storage;

    const dispatcher::storage::AlarmAcknowledgementStorageQuery query{
        .alarm_id = dispatcher::domain::AlarmId{"alarm-1"},
        .operator_id = "operator-1",
        .limit = 10,
        .latest_only = true
    };

    const auto query_result = storage.query(query);

    EXPECT_TRUE(query_result.ok());
    EXPECT_TRUE(query_result.empty());

    ASSERT_TRUE(storage.last_query().has_value());
    EXPECT_TRUE(storage.last_query()->has_alarm_id());
    EXPECT_TRUE(storage.last_query()->has_operator_id());
    EXPECT_TRUE(storage.last_query()->has_limit());
    EXPECT_TRUE(storage.last_query()->requests_latest_only());

    const auto remove_by_alarm_result = storage.remove_by_alarm(
        dispatcher::domain::AlarmId{ "alarm-1" }
    );

    EXPECT_TRUE(remove_by_alarm_result.ok());

    ASSERT_TRUE(storage.last_removed_alarm_id().has_value());
    EXPECT_EQ(
        storage.last_removed_alarm_id()->value(),
        "alarm-1"
    );

    const auto remove_by_operator_result = storage.remove_by_operator(
        "operator-1"
    );

    EXPECT_TRUE(remove_by_operator_result.ok());

    ASSERT_TRUE(storage.last_removed_operator_id().has_value());
    EXPECT_EQ(
        storage.last_removed_operator_id().value(),
        "operator-1"
    );

    const auto clear_result = storage.clear();

    EXPECT_TRUE(clear_result.ok());
    EXPECT_TRUE(storage.clear_called());
}