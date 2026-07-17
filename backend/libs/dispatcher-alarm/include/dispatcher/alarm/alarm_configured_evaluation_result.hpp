#pragma once

#include <dispatcher/alarm/alarm_evaluation_batch_result.hpp>

#include <cstdint>

namespace dispatcher::alarm
{
    class AlarmConfiguredEvaluationResult
    {
    public:
        [[nodiscard]] AlarmEvaluationBatchResult& batch_result() noexcept
        {
            return batch_result_;
        }

        [[nodiscard]] const AlarmEvaluationBatchResult& batch_result()
            const noexcept
        {
            return batch_result_;
        }

        [[nodiscard]] std::uint64_t configured_alarm_count() const noexcept
        {
            return configured_alarm_count_;
        }

        [[nodiscard]] std::uint64_t missing_condition_count() const noexcept
        {
            return missing_condition_count_;
        }

        [[nodiscard]] std::uint64_t suppressed_alarm_count() const noexcept
        {
            return suppressed_alarm_count_;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return configured_alarm_count_ == 0
                && missing_condition_count_ == 0
                && suppressed_alarm_count_ == 0
                && batch_result_.empty();
        }

        [[nodiscard]] bool has_missing_conditions() const noexcept
        {
            return missing_condition_count_ > 0;
        }

        [[nodiscard]] bool has_suppressed_alarms() const noexcept
        {
            return suppressed_alarm_count_ > 0;
        }

        void record_configured_alarm() noexcept
        {
            ++configured_alarm_count_;
        }

        void record_missing_condition() noexcept
        {
            ++missing_condition_count_;
        }

        void record_suppressed_alarm() noexcept
        {
            ++suppressed_alarm_count_;
        }

    private:
        AlarmEvaluationBatchResult batch_result_;
        std::uint64_t configured_alarm_count_{ 0 };
        std::uint64_t missing_condition_count_{ 0 };
        std::uint64_t suppressed_alarm_count_{ 0 };
    };
}