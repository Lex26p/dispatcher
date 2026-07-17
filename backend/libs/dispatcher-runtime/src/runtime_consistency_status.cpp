#include <dispatcher/runtime/runtime_consistency_status.hpp>

namespace dispatcher::runtime
{
    const char* to_string(RuntimeConsistencyStatus status) noexcept
    {
        switch (status)
        {
        case RuntimeConsistencyStatus::Unknown:
            return "unknown";

        case RuntimeConsistencyStatus::Consistent:
            return "consistent";

        case RuntimeConsistencyStatus::Inconsistent:
            return "inconsistent";
        }

        return "unknown";
    }

    bool is_known(RuntimeConsistencyStatus status) noexcept
    {
        return status != RuntimeConsistencyStatus::Unknown;
    }

    bool is_consistent(RuntimeConsistencyStatus status) noexcept
    {
        return status == RuntimeConsistencyStatus::Consistent;
    }

    bool is_inconsistent(RuntimeConsistencyStatus status) noexcept
    {
        return status == RuntimeConsistencyStatus::Inconsistent;
    }

    bool requires_action(RuntimeConsistencyStatus status) noexcept
    {
        return status == RuntimeConsistencyStatus::Unknown
            || status == RuntimeConsistencyStatus::Inconsistent;
    }
}