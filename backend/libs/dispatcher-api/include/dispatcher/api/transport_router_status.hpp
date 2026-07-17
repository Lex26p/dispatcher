#pragma once

namespace dispatcher::api
{
    enum class TransportRouterStatus
    {
        Handled,
        InvalidRequest,
        EndpointNotFound,
        HandlerNotFound,
        HandlerFailed
    };

    [[nodiscard]] const char* to_string(
        TransportRouterStatus status
    ) noexcept;

    [[nodiscard]] bool is_success(
        TransportRouterStatus status
    ) noexcept;

    [[nodiscard]] bool is_failure(
        TransportRouterStatus status
    ) noexcept;
}