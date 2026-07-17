#pragma once

#include <dispatcher/alarm/alarm_evaluation_result.hpp>
#include <dispatcher/alarm/alarm_transition_type.hpp>

#include <cstdint>

namespace dispatcher::alarm
{
    class AlarmRuntimeStatistics
    {
    public:
        void record(
            const AlarmEvaluationResult& evaluation_result,
            bool event_stored
        ) noexcept
        {
            ++total_count_;

            switch (evaluation_result.status())
            {
            case AlarmEvaluationStatus::Evaluated:
                ++evaluated_count_;
                break;

            case AlarmEvaluationStatus::DisabledAlarm:
                ++disabled_alarm_count_;
                break;

            case AlarmEvaluationStatus::TagMismatch:
                ++tag_mismatch_count_;
                break;

            case AlarmEvaluationStatus::UnsupportedValueType:
                ++unsupported_value_type_count_;
                break;
            }

            switch (evaluation_result.transition_type())
            {
            case AlarmTransitionType::None:
                if (evaluation_result.evaluated())
                {
                    ++no_transition_count_;
                }
                break;

            case AlarmTransitionType::Activated:
                ++activated_count_;
                break;

            case AlarmTransitionType::Acknowledged:
                ++acknowledged_count_;
                break;

            case AlarmTransitionType::Cleared:
                ++cleared_count_;
                break;
            }

            if (event_stored)
            {
                ++stored_event_count_;
            }
        }

        [[nodiscard]] std::uint64_t total_count() const noexcept
        {
            return total_count_;
        }

        [[nodiscard]] std::uint64_t evaluated_count() const noexcept
        {
            return evaluated_count_;
        }

        [[nodiscard]] std::uint64_t skipped_count() const noexcept
        {
            return disabled_alarm_count_
                + tag_mismatch_count_
                + unsupported_value_type_count_;
        }

        [[nodiscard]] std::uint64_t disabled_alarm_count() const noexcept
        {
            return disabled_alarm_count_;
        }

        [[nodiscard]] std::uint64_t tag_mismatch_count() const noexcept
        {
            return tag_mismatch_count_;
        }

        [[nodiscard]] std::uint64_t unsupported_value_type_count() const noexcept
        {
            return unsupported_value_type_count_;
        }

        [[nodiscard]] std::uint64_t activated_count() const noexcept
        {
            return activated_count_;
        }

        [[nodiscard]] std::uint64_t acknowledged_count() const noexcept
        {
            return acknowledged_count_;
        }

        [[nodiscard]] std::uint64_t cleared_count() const noexcept
        {
            return cleared_count_;
        }

        [[nodiscard]] std::uint64_t no_transition_count() const noexcept
        {
            return no_transition_count_;
        }

        [[nodiscard]] std::uint64_t stored_event_count() const noexcept
        {
            return stored_event_count_;
        }

        void reset() noexcept
        {
            total_count_ = 0;
            evaluated_count_ = 0;
            disabled_alarm_count_ = 0;
            tag_mismatch_count_ = 0;
            unsupported_value_type_count_ = 0;
            activated_count_ = 0;
            acknowledged_count_ = 0;
            cleared_count_ = 0;
            no_transition_count_ = 0;
            stored_event_count_ = 0;
        }

    private:
        std::uint64_t total_count_{ 0 };
        std::uint64_t evaluated_count_{ 0 };

        std::uint64_t disabled_alarm_count_{ 0 };
        std::uint64_t tag_mismatch_count_{ 0 };
        std::uint64_t unsupported_value_type_count_{ 0 };

        std::uint64_t activated_count_{ 0 };
        std::uint64_t acknowledged_count_{ 0 };
        std::uint64_t cleared_count_{ 0 };
        std::uint64_t no_transition_count_{ 0 };

        std::uint64_t stored_event_count_{ 0 };
    };
}