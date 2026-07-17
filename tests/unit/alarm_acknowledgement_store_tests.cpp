#include <dispatcher/alarm/alarm_acknowledgement_record.hpp>
#include <dispatcher/alarm/alarm_acknowledgement_result.hpp>
#include <dispatcher/alarm/alarm_acknowledgement_store.hpp>
#include <dispatcher/alarm/alarm_state.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>

namespace
{
    dispatcher::alarm::AlarmAcknowledgementRecord make_record(
        std::string alarm_id,
        std::string operator_id,
        dispatcher::alarm::AlarmAcknowledgementStatus status,
        std::optional<std::uint64_t> event_sequence
    )
    {
        return dispatcher::alarm::AlarmAcknowledgementRecord(
            dispatcher::domain::AlarmId{ std::move(alarm_id) },
            std::move(operator_id),
            "Checked and acknowledged",
            status,
            dispatcher::alarm::AlarmState::Active,
            status == dispatcher::alarm::AlarmAcknowledgementStatus::Acknowledged
            ? dispatcher::alarm::AlarmState::Acknowledged
            : dispatcher::alarm::AlarmState::Active,
            dispatcher::alarm::AlarmAcknowledgementRecord::Clock::now(),
            event_sequence
        );
    }
}

TEST(AlarmAcknowledgementStoreTests, RecordStoresValues)
{
    const auto timestamp =
        dispatcher::alarm::AlarmAcknowledgementRecord::Clock::now();

    const dispatcher::alarm::AlarmAcknowledgementRecord record(
        dispatcher::domain::AlarmId{ "alarm-1" },
        "operator-1",
        "Checked field device",
        dispatcher::alarm::AlarmAcknowledgementStatus::Acknowledged,
        dispatcher::alarm::AlarmState::Active,
        dispatcher::alarm::AlarmState::Acknowledged,
        timestamp,
        42
    );

    EXPECT_EQ(record.alarm_id(), dispatcher::domain::AlarmId{ "alarm-1" });
    EXPECT_EQ(record.operator_id(), "operator-1");
    EXPECT_EQ(record.comment(), "Checked field device");
    EXPECT_EQ(
        record.status(),
        dispatcher::alarm::AlarmAcknowledgementStatus::Acknowledged
    );
    EXPECT_TRUE(record.acknowledged());
    EXPECT_FALSE(record.skipped());
    EXPECT_EQ(record.previous_state(), dispatcher::alarm::AlarmState::Active);
    EXPECT_EQ(
        record.new_state(),
        dispatcher::alarm::AlarmState::Acknowledged
    );
    EXPECT_EQ(record.timestamp(), timestamp);

    ASSERT_TRUE(record.event_sequence().has_value());
    EXPECT_EQ(*record.event_sequence(), 42);
}

TEST(AlarmAcknowledgementStoreTests, AppendStoresRecords)
{
    dispatcher::alarm::AlarmAcknowledgementStore store;

    EXPECT_TRUE(store.empty());
    EXPECT_EQ(store.size(), 0);

    store.append(
        make_record(
            "alarm-1",
            "operator-1",
            dispatcher::alarm::AlarmAcknowledgementStatus::Acknowledged,
            1
        )
    );

    EXPECT_FALSE(store.empty());
    EXPECT_EQ(store.size(), 1);

    ASSERT_EQ(store.records().size(), 1);
    EXPECT_EQ(
        store.records()[0].alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-1" }
    );
}

TEST(AlarmAcknowledgementStoreTests, FindByAlarmIdReturnsMatchingRecords)
{
    dispatcher::alarm::AlarmAcknowledgementStore store;

    store.append(
        make_record(
            "alarm-1",
            "operator-1",
            dispatcher::alarm::AlarmAcknowledgementStatus::Acknowledged,
            1
        )
    );

    store.append(
        make_record(
            "alarm-2",
            "operator-1",
            dispatcher::alarm::AlarmAcknowledgementStatus::NotActive,
            std::nullopt
        )
    );

    store.append(
        make_record(
            "alarm-1",
            "operator-2",
            dispatcher::alarm::AlarmAcknowledgementStatus::AlreadyAcknowledged,
            std::nullopt
        )
    );

    const auto records = store.find_by_alarm_id(
        dispatcher::domain::AlarmId{ "alarm-1" }
    );

    ASSERT_EQ(records.size(), 2);

    EXPECT_EQ(
        records[0].alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-1" }
    );

    EXPECT_EQ(
        records[1].alarm_id(),
        dispatcher::domain::AlarmId{ "alarm-1" }
    );
}

TEST(AlarmAcknowledgementStoreTests, FindByOperatorIdReturnsMatchingRecords)
{
    dispatcher::alarm::AlarmAcknowledgementStore store;

    store.append(
        make_record(
            "alarm-1",
            "operator-1",
            dispatcher::alarm::AlarmAcknowledgementStatus::Acknowledged,
            1
        )
    );

    store.append(
        make_record(
            "alarm-2",
            "operator-2",
            dispatcher::alarm::AlarmAcknowledgementStatus::NotActive,
            std::nullopt
        )
    );

    store.append(
        make_record(
            "alarm-3",
            "operator-1",
            dispatcher::alarm::AlarmAcknowledgementStatus::UnknownAlarm,
            std::nullopt
        )
    );

    const auto records = store.find_by_operator_id("operator-1");

    ASSERT_EQ(records.size(), 2);

    EXPECT_EQ(records[0].operator_id(), "operator-1");
    EXPECT_EQ(records[1].operator_id(), "operator-1");
}

TEST(AlarmAcknowledgementStoreTests, ClearRemovesRecords)
{
    dispatcher::alarm::AlarmAcknowledgementStore store;

    store.append(
        make_record(
            "alarm-1",
            "operator-1",
            dispatcher::alarm::AlarmAcknowledgementStatus::Acknowledged,
            1
        )
    );

    ASSERT_FALSE(store.empty());

    store.clear();

    EXPECT_TRUE(store.empty());
    EXPECT_EQ(store.size(), 0);
}