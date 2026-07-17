#include <dispatcher/alarm/alarm_suppression_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::alarm
{
    AlarmSuppressionResult AlarmSuppressionResult::applied(
        AlarmSuppressionRecord record
    )
    {
        return AlarmSuppressionResult(
            AlarmSuppressionStatus::Applied,
            std::move(record),
            {}
        );
    }

    AlarmSuppressionResult AlarmSuppressionResult::cleared(
        AlarmSuppressionRecord record
    )
    {
        return AlarmSuppressionResult(
            AlarmSuppressionStatus::Cleared,
            std::move(record),
            {}
        );
    }

    AlarmSuppressionResult AlarmSuppressionResult::failure(
        AlarmSuppressionStatus status,
        std::string message
    )
    {
        if (is_success(status))
        {
            status = AlarmSuppressionStatus::InvalidCommand;
        }

        return AlarmSuppressionResult(
            status,
            std::nullopt,
            std::move(message)
        );
    }

    bool AlarmSuppressionResult::success() const noexcept
    {
        return is_success(status_);
    }

    bool AlarmSuppressionResult::failed() const noexcept
    {
        return !success();
    }

    bool AlarmSuppressionResult::applied() const noexcept
    {
        return status_ == AlarmSuppressionStatus::Applied;
    }

    bool AlarmSuppressionResult::cleared() const noexcept
    {
        return status_ == AlarmSuppressionStatus::Cleared;
    }

    AlarmSuppressionStatus AlarmSuppressionResult::status() const noexcept
    {
        return status_;
    }

    const std::string& AlarmSuppressionResult::message() const noexcept
    {
        return message_;
    }

    bool AlarmSuppressionResult::has_message() const noexcept
    {
        return !message_.empty();
    }

    bool AlarmSuppressionResult::has_record() const noexcept
    {
        return record_.has_value();
    }

    const AlarmSuppressionRecord& AlarmSuppressionResult::record() const
    {
        if (!record_.has_value())
        {
            throw std::logic_error(
                "AlarmSuppressionResult does not contain a record"
            );
        }

        return record_.value();
    }

    AlarmSuppressionResult::AlarmSuppressionResult(
        AlarmSuppressionStatus status,
        std::optional<AlarmSuppressionRecord> record,
        std::string message
    )
        : status_(status)
        , record_(std::move(record))
        , message_(std::move(message))
    {
    }
}