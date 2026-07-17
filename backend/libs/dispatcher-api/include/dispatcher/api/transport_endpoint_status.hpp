#pragma once

namespace dispatcher::api
{
    enum class TransportEndpointStatus
    {
        Registered,
        Removed,
        Found,

        NotFound,
        DuplicateEndpoint,
        InvalidEndpoint
    };

    [[nodiscard]] const char* to_string(
        TransportEndpointStatus status
    ) noexcept;

    [[nodiscard]] bool is_success(
        TransportEndpointStatus status
    ) noexcept;

    [[nodiscard]] bool is_failure(
        TransportEndpointStatus status
    ) noexcept;
}