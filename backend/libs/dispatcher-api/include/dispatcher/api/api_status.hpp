#pragma once

namespace dispatcher::api
{
    enum class ApiStatus
    {
        Success,
        Accepted,
        NoContent,

        BadRequest,
        Unauthorized,
        Forbidden,
        NotFound,
        Conflict,
        ValidationError,

        RuntimeRejected,
        StorageError,
        Timeout,

        UnsupportedOperation,
        InternalError
    };

    [[nodiscard]] const char* to_string(ApiStatus status) noexcept;

    [[nodiscard]] bool is_success(ApiStatus status) noexcept;

    [[nodiscard]] bool is_failure(ApiStatus status) noexcept;

    [[nodiscard]] bool is_client_error(ApiStatus status) noexcept;

    [[nodiscard]] bool is_server_error(ApiStatus status) noexcept;
}