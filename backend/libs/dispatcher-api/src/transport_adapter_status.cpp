#include <dispatcher/api/transport_adapter_status.hpp>

namespace dispatcher::api
{
    const char* to_string(TransportAdapterStatus status) noexcept
    {
        switch (status)
        {
        case TransportAdapterStatus::Stopped:
            return "stopped";

        case TransportAdapterStatus::Starting:
            return "starting";

        case TransportAdapterStatus::Running:
            return "running";

        case TransportAdapterStatus::Stopping:
            return "stopping";

        case TransportAdapterStatus::Failed:
            return "failed";

        case TransportAdapterStatus::Disabled:
            return "disabled";
        }

        return "failed";
    }

    bool is_running(TransportAdapterStatus status) noexcept
    {
        return status == TransportAdapterStatus::Running;
    }

    bool is_stopped(TransportAdapterStatus status) noexcept
    {
        return status == TransportAdapterStatus::Stopped
            || status == TransportAdapterStatus::Disabled;
    }

    bool is_transitioning(TransportAdapterStatus status) noexcept
    {
        return status == TransportAdapterStatus::Starting
            || status == TransportAdapterStatus::Stopping;
    }

    bool is_failure(TransportAdapterStatus status) noexcept
    {
        return status == TransportAdapterStatus::Failed;
    }

    bool accepts_requests(TransportAdapterStatus status) noexcept
    {
        return status == TransportAdapterStatus::Running;
    }
}