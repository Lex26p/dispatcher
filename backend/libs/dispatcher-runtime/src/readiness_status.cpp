#include <dispatcher/runtime/readiness_status.hpp>

namespace dispatcher::runtime
{
    const char* to_string(ReadinessStatus status) noexcept
    {
        switch (status)
        {
        case ReadinessStatus::Unknown:
            return "unknown";

        case ReadinessStatus::Ready:
            return "ready";

        case ReadinessStatus::NotReady:
            return "not_ready";
        }

        return "unknown";
    }

    bool is_known(ReadinessStatus status) noexcept
    {
        return status != ReadinessStatus::Unknown;
    }

    bool is_ready(ReadinessStatus status) noexcept
    {
        return status == ReadinessStatus::Ready;
    }

    bool is_not_ready(ReadinessStatus status) noexcept
    {
        return status == ReadinessStatus::NotReady;
    }
}