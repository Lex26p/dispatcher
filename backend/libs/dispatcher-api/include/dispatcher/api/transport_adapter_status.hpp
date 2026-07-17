#pragma once

namespace dispatcher::api
{
    enum class TransportAdapterStatus
    {
        Stopped,
        Starting,
        Running,
        Stopping,
        Failed,
        Disabled
    };

    [[nodiscard]] const char* to_string(
        TransportAdapterStatus status
    ) noexcept;

    [[nodiscard]] bool is_running(
        TransportAdapterStatus status
    ) noexcept;

    [[nodiscard]] bool is_stopped(
        TransportAdapterStatus status
    ) noexcept;

    [[nodiscard]] bool is_transitioning(
        TransportAdapterStatus status
    ) noexcept;

    [[nodiscard]] bool is_failure(
        TransportAdapterStatus status
    ) noexcept;

    [[nodiscard]] bool accepts_requests(
        TransportAdapterStatus status
    ) noexcept;
}