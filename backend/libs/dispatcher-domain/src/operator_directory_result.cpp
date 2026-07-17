#include <dispatcher/domain/operator_directory_result.hpp>

#include <utility>

namespace dispatcher::domain
{
    OperatorDirectoryResult OperatorDirectoryResult::success(
        OperatorDirectoryStatus status,
        std::string message
    )
    {
        if (!is_success(status))
        {
            status = OperatorDirectoryStatus::Added;
        }

        return OperatorDirectoryResult(
            status,
            std::move(message),
            {},
            {}
        );
    }

    OperatorDirectoryResult OperatorDirectoryResult::failure(
        OperatorDirectoryStatus status,
        std::string message,
        std::string field,
        std::string value
    )
    {
        if (!is_failure(status))
        {
            status = OperatorDirectoryStatus::InvalidIdentity;
        }

        return OperatorDirectoryResult(
            status,
            std::move(message),
            std::move(field),
            std::move(value)
        );
    }

    bool OperatorDirectoryResult::ok() const noexcept
    {
        return is_success(status_);
    }

    bool OperatorDirectoryResult::failed() const noexcept
    {
        return !ok();
    }

    OperatorDirectoryStatus OperatorDirectoryResult::status() const noexcept
    {
        return status_;
    }

    const std::string& OperatorDirectoryResult::message() const noexcept
    {
        return message_;
    }

    const std::string& OperatorDirectoryResult::field() const noexcept
    {
        return field_;
    }

    const std::string& OperatorDirectoryResult::value() const noexcept
    {
        return value_;
    }

    bool OperatorDirectoryResult::has_message() const noexcept
    {
        return !message_.empty();
    }

    bool OperatorDirectoryResult::has_field() const noexcept
    {
        return !field_.empty();
    }

    bool OperatorDirectoryResult::has_value() const noexcept
    {
        return !value_.empty();
    }

    OperatorDirectoryResult::OperatorDirectoryResult(
        OperatorDirectoryStatus status,
        std::string message,
        std::string field,
        std::string value
    )
        : status_(status)
        , message_(std::move(message))
        , field_(std::move(field))
        , value_(std::move(value))
    {
    }
}