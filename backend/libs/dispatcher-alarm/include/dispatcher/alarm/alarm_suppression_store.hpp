#pragma once

#include <dispatcher/alarm/alarm_suppression_command.hpp>
#include <dispatcher/alarm/alarm_suppression_record.hpp>
#include <dispatcher/alarm/alarm_suppression_result.hpp>
#include <dispatcher/alarm/alarm_suppression_runtime_snapshot.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dispatcher::alarm
{
    class AlarmSuppressionStore
    {
    public:
        using Clock = std::chrono::system_clock;
        using TimePoint = Clock::time_point;

        [[nodiscard]] AlarmSuppressionResult apply(
            const AlarmSuppressionCommand& command,
            TimePoint now = Clock::now()
        );

        [[nodiscard]] AlarmSuppressionResult clear(
            const dispatcher::domain::AlarmId& alarm_id,
            TimePoint now = Clock::now()
        );

        [[nodiscard]] const AlarmSuppressionRecord* find_by_alarm_id(
            const dispatcher::domain::AlarmId& alarm_id,
            TimePoint now = Clock::now()
        ) const noexcept;

        [[nodiscard]] bool is_active(
            const dispatcher::domain::AlarmId& alarm_id,
            TimePoint now = Clock::now()
        ) const noexcept;

        [[nodiscard]] std::vector<AlarmSuppressionRecord> active_records(
            TimePoint now = Clock::now()
        ) const;

        [[nodiscard]] const std::vector<AlarmSuppressionRecord>& records()
            const noexcept;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] std::size_t remove_expired(
            TimePoint now = Clock::now()
        );

        [[nodiscard]] AlarmSuppressionRuntimeSnapshot runtime_snapshot(
            TimePoint now = Clock::now()
        ) const noexcept;

        void reset_statistics();

        void clear_all();

    private:
        using RecordIterator = std::vector<AlarmSuppressionRecord>::iterator;
        using ConstRecordIterator =
            std::vector<AlarmSuppressionRecord>::const_iterator;

        [[nodiscard]] RecordIterator find_iterator(
            const dispatcher::domain::AlarmId& alarm_id
        ) noexcept;

        [[nodiscard]] ConstRecordIterator find_iterator(
            const dispatcher::domain::AlarmId& alarm_id
        ) const noexcept;

        std::uint64_t applied_count_{ 0 };
        std::uint64_t cleared_count_{ 0 };
        std::uint64_t rejected_count_{ 0 };
        std::uint64_t expired_removed_count_{ 0 };

        std::vector<AlarmSuppressionRecord> records_;
    };
}