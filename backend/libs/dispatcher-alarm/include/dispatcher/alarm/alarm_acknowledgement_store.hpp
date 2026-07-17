#pragma once

#include <dispatcher/alarm/alarm_acknowledgement_record.hpp>
#include <dispatcher/domain/id_types.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace dispatcher::alarm
{
    class AlarmAcknowledgementStore
    {
    public:
        void append(AlarmAcknowledgementRecord record);

        [[nodiscard]] std::vector<AlarmAcknowledgementRecord> find_by_alarm_id(
            const dispatcher::domain::AlarmId& alarm_id
        ) const;

        [[nodiscard]] std::vector<AlarmAcknowledgementRecord> find_by_operator_id(
            const std::string& operator_id
        ) const;

        [[nodiscard]] const std::vector<AlarmAcknowledgementRecord>& records()
            const noexcept;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

        void clear() noexcept;

    private:
        std::vector<AlarmAcknowledgementRecord> records_;
    };
}