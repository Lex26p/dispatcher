#pragma once

namespace dispatcher::http
{
    enum class HttpServerStatus
    {
        Unknown,
        Stopped,
        Starting,
        Running,
        Stopping,
        Failed
    };

    [[nodiscard]] const char* to_string(
        HttpServerStatus status
    ) noexcept;

    [[nodiscard]] bool is_known(
        HttpServerStatus status
    ) noexcept;

    [[nodiscard]] bool is_stopped(
        HttpServerStatus status
    ) noexcept;

    [[nodiscard]] bool is_starting(
        HttpServerStatus status
    ) noexcept;

    [[nodiscard]] bool is_running(
        HttpServerStatus status
    ) noexcept;

    [[nodiscard]] bool is_stopping(
        HttpServerStatus status
    ) noexcept;

    [[nodiscard]] bool is_failed(
        HttpServerStatus status
    ) noexcept;

    [[nodiscard]] bool is_terminal(
        HttpServerStatus status
    ) noexcept;

    [[nodiscard]] bool accepts_start(
        HttpServerStatus status
    ) noexcept;

    [[nodiscard]] bool accepts_stop(
        HttpServerStatus status
    ) noexcept;
}