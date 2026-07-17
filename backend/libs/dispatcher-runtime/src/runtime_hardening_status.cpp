#include <dispatcher/runtime/runtime_hardening_status.hpp>

namespace dispatcher::runtime
{
    const char* to_string(RuntimeHardeningStatus status) noexcept
    {
        switch (status)
        {
        case RuntimeHardeningStatus::Unknown:
            return "unknown";

        case RuntimeHardeningStatus::Passing:
            return "passing";

        case RuntimeHardeningStatus::Warning:
            return "warning";

        case RuntimeHardeningStatus::Failing:
            return "failing";
        }

        return "unknown";
    }

    bool is_known(RuntimeHardeningStatus status) noexcept
    {
        return status != RuntimeHardeningStatus::Unknown;
    }

    bool is_passing(RuntimeHardeningStatus status) noexcept
    {
        return status == RuntimeHardeningStatus::Passing;
    }

    bool is_warning(RuntimeHardeningStatus status) noexcept
    {
        return status == RuntimeHardeningStatus::Warning;
    }

    bool is_failing(RuntimeHardeningStatus status) noexcept
    {
        return status == RuntimeHardeningStatus::Failing;
    }

    bool allows_release(RuntimeHardeningStatus status) noexcept
    {
        return status == RuntimeHardeningStatus::Passing
            || status == RuntimeHardeningStatus::Warning;
    }

    bool requires_action(RuntimeHardeningStatus status) noexcept
    {
        return status == RuntimeHardeningStatus::Unknown
            || status == RuntimeHardeningStatus::Warning
            || status == RuntimeHardeningStatus::Failing;
    }
}