#include <dispatcher/alarm/alarm_runtime_event.hpp>
#include <dispatcher/alarm/alarm_transition_type.hpp>
#include <dispatcher/storage/alarm_event_query_result.hpp>
#include <dispatcher/storage/alarm_event_storage.hpp>
#include <dispatcher/storage/alarm_event_storage_query.hpp>
#include <dispatcher/storage/storage_result.hpp>
#include <dispatcher/storage/storage_status.hpp>

#include <gtest/gtest.h>

#include <optional>
#include <vector>

namespace
{
    class FakeAlarmEventStorage final
        : public dispatcher::storage::AlarmEventStorage
    {
    public:
        [[nodiscard]] dispatcher::storage::StorageResult append(
            const dispatcher::alarm::AlarmRuntimeEvent& event
        ) override
        {
            (void)event;

            ++append_count_;

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::StorageResult append_batch(
            const std::vector<dispatcher::alarm::AlarmRuntimeEvent>& events
        ) override
        {
            append_batch_count_ += events.size();

            return dispatcher::storage::StorageResult::success();
        }

        [[nodiscard]] dispatcher::storage::AlarmEventQueryResult query(
            const dispatcher::storage::AlarmEventStorageQuery& query
        ) const override
        {
            last_query_ = query;

            return dispatcher::storage::AlarmEventQueryResult::success({});
        }

        [[nodiscard]] dispatcher::storage::StorageResult remove_by_alarm(
            const dispatcher::domain::AlarmId& alarm_id
        ) override
        {
            last_removed_alarm_id_ = alarm_id;

            return dispatcher::storage::StorageResult::success();
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

        [[nodiscard]] const std::optional<dispatcher::domain::AlarmId>&
            last_removed_alarm_id() const noexcept
        {
            return last_removed_alarm_id_;
        }

        [[nodiscard]] const std::optional<dispatcher::domain::TagId>&
            last_removed_tag_id() const noexcept
        {
            return last_removed_tag_id_;
        }

        [[nodiscard]] const std::optional<dispatcher::storage::AlarmEventStorageQuery>&
            last_query() const noexcept
        {
            return last_query_;
        }

    private:
        std::size_t append_count_{ 0 };
        std::size_t append_batch_count_{ 0 };
        bool clear_called_{ false };

        std::optional<dispatcher::domain::AlarmId> last_removed_alarm_id_;
        std::optional<dispatcher::domain::TagId> last_removed_tag_id_;
        mutable std::optional<dispatcher::storage::AlarmEventStorageQuery>
            last_query_;
    };
}

TEST(AlarmEventStorageQueryTests, DefaultQueryIsUnbounded)
{
    const dispatcher::storage::AlarmEventStorageQuery query;

    EXPECT_FALSE(query.has_alarm_id());
    EXPECT_FALSE(query.has_tag_id());
    EXPECT_FALSE(query.has_transition_type());
    EXPECT_FALSE(query.has_from());
    EXPECT_FALSE(query.has_to());
    EXPECT_FALSE(query.has_limit());
    EXPECT_FALSE(query.has_time_range());
    EXPECT_FALSE(query.has_bounded_time_range());
    EXPECT_FALSE(query.requests_latest_only());
}

TEST(AlarmEventStorageQueryTests, QueryPredicatesReflectConfiguredFields)
{
    dispatcher::storage::AlarmEventStorageQuery query;

    query.alarm_id = dispatcher::domain::AlarmId{ "alarm-1" };
    query.tag_id = dispatcher::domain::TagId{ "tag-1" };
    query.transition_type =
        dispatcher::alarm::AlarmTransitionType::Activated;
    query.from = dispatcher::storage::AlarmEventStorageQuery::Clock::now();
    query.to = dispatcher::storage::AlarmEventStorageQuery::Clock::now();
    query.limit = 100;
    query.latest_only = true;

    EXPECT_TRUE(query.has_alarm_id());
    EXPECT_TRUE(query.has_tag_id());
    EXPECT_TRUE(query.has_transition_type());
    EXPECT_TRUE(query.has_from());
    EXPECT_TRUE(query.has_to());
    EXPECT_TRUE(query.has_limit());
    EXPECT_TRUE(query.has_time_range());
    EXPECT_TRUE(query.has_bounded_time_range());
    EXPECT_TRUE(query.requests_latest_only());
}

TEST(AlarmEventQueryResultTests, SuccessResultWorks)
{
    const auto result =
        dispatcher::storage::AlarmEventQueryResult::success({});

    EXPECT_TRUE(result.ok());
    EXPECT_FALSE(result.failed());

    EXPECT_EQ(
        result.status(),
        dispatcher::storage::StorageStatus::Success
    );

    EXPECT_TRUE(result.empty());
    EXPECT_EQ(result.event_count(), 0);
    EXPECT_TRUE(result.events().empty());
}

TEST(AlarmEventQueryResultTests, FailureResultCapturesError)
{
    const auto result = dispatcher::storage::AlarmEventQueryResult::failure(
        dispatcher::storage::StorageStatus::BackendUnavailable,
        "alarm_event.query",
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
    EXPECT_EQ(result.event_count(), 0);

    EXPECT_EQ(result.error().operation, "alarm_event.query");
    EXPECT_EQ(result.error().key, "alarm-1");
    EXPECT_EQ(result.error().message, "backend is down");
}

TEST(AlarmEventStorageInterfaceTests, InterfaceCanBeImplemented)
{
    FakeAlarmEventStorage storage;

    const dispatcher::storage::AlarmEventStorageQuery query{
        .alarm_id = dispatcher::domain::AlarmId{"alarm-1"},
        .tag_id = dispatcher::domain::TagId{"tag-1"},
        .transition_type =
            dispatcher::alarm::AlarmTransitionType::Activated,
        .limit = 10,
        .latest_only = true
    };

    const auto query_result = storage.query(query);

    EXPECT_TRUE(query_result.ok());
    EXPECT_TRUE(query_result.empty());

    ASSERT_TRUE(storage.last_query().has_value());
    EXPECT_TRUE(storage.last_query()->has_alarm_id());
    EXPECT_TRUE(storage.last_query()->has_tag_id());
    EXPECT_TRUE(storage.last_query()->has_transition_type());
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

    const auto remove_by_tag_result = storage.remove_by_tag(
        dispatcher::domain::TagId{ "tag-1" }
    );

    EXPECT_TRUE(remove_by_tag_result.ok());

    ASSERT_TRUE(storage.last_removed_tag_id().has_value());
    EXPECT_EQ(
        storage.last_removed_tag_id()->value(),
        "tag-1"
    );

    const auto clear_result = storage.clear();

    EXPECT_TRUE(clear_result.ok());
    EXPECT_TRUE(storage.clear_called());
}