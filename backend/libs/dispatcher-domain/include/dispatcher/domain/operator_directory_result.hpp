#pragma once

#include <dispatcher/domain/operator_directory_status.hpp>

#include <string>

namespace dispatcher::domain
{
    class OperatorDirectoryResult
    {
    public:
        [[nodiscard]] static OperatorDirectoryResult success(
            OperatorDirectoryStatus status,
            std::string message = {}
        );

        [[nodiscard]] static OperatorDirectoryResult failure(
            OperatorDirectoryStatus status,
            std::string message = {},
            std::string field = {},
            std::string value = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] OperatorDirectoryStatus status() const noexcept;

        [[nodiscard]] const std::string& message() const noexcept;

        [[nodiscard]] const std::string& field() const noexcept;

        [[nodiscard]] const std::string& value() const noexcept;

        [[nodiscard]] bool has_message() const noexcept;

        [[nodiscard]] bool has_field() const noexcept;

        [[nodiscard]] bool has_value() const noexcept;

    private:
        OperatorDirectoryResult(
            OperatorDirectoryStatus status,
            std::string message,
            std::string field,
            std::string value
        );

        OperatorDirectoryStatus status_{ OperatorDirectoryStatus::InvalidIdentity };
        std::string message_;
        std::string field_;
        std::string value_;
    };
}