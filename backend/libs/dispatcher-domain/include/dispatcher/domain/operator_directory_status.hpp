#pragma once

namespace dispatcher::domain
{
    enum class OperatorDirectoryStatus
    {
        Added,
        Removed,
        NotFound,
        DuplicateOperatorId,
        DuplicateUsername,
        InvalidIdentity
    };

    [[nodiscard]] const char* to_string(
        OperatorDirectoryStatus status
    ) noexcept;

    [[nodiscard]] bool is_success(
        OperatorDirectoryStatus status
    ) noexcept;

    [[nodiscard]] bool is_failure(
        OperatorDirectoryStatus status
    ) noexcept;
}