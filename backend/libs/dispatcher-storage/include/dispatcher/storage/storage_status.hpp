#pragma once

namespace dispatcher::storage
{
    enum class StorageStatus
    {
        Success,
        NotFound,
        AlreadyExists,
        Conflict,
        ValidationError,
        SerializationError,
        IoError,
        BackendUnavailable,
        Timeout,
        UnsupportedOperation,
        UnknownError
    };

    [[nodiscard]] const char* to_string(StorageStatus status) noexcept;

    [[nodiscard]] bool is_success(StorageStatus status) noexcept;

    [[nodiscard]] bool is_failure(StorageStatus status) noexcept;
}