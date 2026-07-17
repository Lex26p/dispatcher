#include <dispatcher/http/http_server_status.hpp>

namespace dispatcher::http
{
    const char* to_string(HttpServerStatus status) noexcept
    {
        switch (status)
        {
        case HttpServerStatus::Unknown:
            return "unknown";

        case HttpServerStatus::Stopped:
            return "stopped";

        case HttpServerStatus::Starting:
            return "starting";

        case HttpServerStatus::Running:
            return "running";

        case HttpServerStatus::Stopping:
            return "stopping";

        case HttpServerStatus::Failed:
            return "failed";
        }

        return "unknown";
    }

    bool is_known(HttpServerStatus status) noexcept
    {
        return status != HttpServerStatus::Unknown;
    }

    bool is_stopped(HttpServerStatus status) noexcept
    {
        return status == HttpServerStatus::Stopped;
    }

    bool is_starting(HttpServerStatus status) noexcept
    {
        return status == HttpServerStatus::Starting;
    }

    bool is_running(HttpServerStatus status) noexcept
    {
        return status == HttpServerStatus::Running;
    }

    bool is_stopping(HttpServerStatus status) noexcept
    {
        return status == HttpServerStatus::Stopping;
    }

    bool is_failed(HttpServerStatus status) noexcept
    {
        return status == HttpServerStatus::Failed;
    }

    bool is_terminal(HttpServerStatus status) noexcept
    {
        return status == HttpServerStatus::Stopped
            || status == HttpServerStatus::Failed;
    }

    bool accepts_start(HttpServerStatus status) noexcept
    {
        return status == HttpServerStatus::Stopped
            || status == HttpServerStatus::Failed
            || status == HttpServerStatus::Unknown;
    }

    bool accepts_stop(HttpServerStatus status) noexcept
    {
        return status == HttpServerStatus::Running
            || status == HttpServerStatus::Starting;
    }
}