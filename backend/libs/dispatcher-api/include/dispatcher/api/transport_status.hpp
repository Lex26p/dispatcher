#pragma once

#include <cstdint>

namespace dispatcher::api
{
    enum class TransportStatus
    {
        Ok,
        Created,
        Accepted,

        BadRequest,
        Unauthorized,
        Forbidden,
        NotFound,
        Conflict,
        UnprocessableEntity,

        InternalError,
        Unavailable
    };

    [[nodiscard]] const char* to_string(
        TransportStatus status
    ) noexcept;

    [[nodiscard]] std::uint16_t http_status_code(
        TransportStatus status
    ) noexcept;

    [[nodiscard]] std::uint16_t grpc_status_code(
        TransportStatus status
    ) noexcept;

    [[nodiscard]] bool is_success(
        TransportStatus status
    ) noexcept;

    [[nodiscard]] bool is_client_error(
        TransportStatus status
    ) noexcept;

    [[nodiscard]] bool is_server_error(
        TransportStatus status
    ) noexcept;

    [[nodiscard]] bool is_failure(
        TransportStatus status
    ) noexcept;
}