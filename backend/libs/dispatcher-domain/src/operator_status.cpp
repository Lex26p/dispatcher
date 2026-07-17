#include <dispatcher/domain/operator_status.hpp>

namespace dispatcher::domain
{
    const char* to_string(OperatorStatus status) noexcept
    {
        switch (status)
        {
        case OperatorStatus::Active:
            return "active";

        case OperatorStatus::Disabled:
            return "disabled";

        case OperatorStatus::Locked:
            return "locked";

        case OperatorStatus::Expired:
            return "expired";
        }

        return "disabled";
    }

    bool can_sign_in(OperatorStatus status) noexcept
    {
        return status == OperatorStatus::Active;
    }

    bool is_active(OperatorStatus status) noexcept
    {
        return status == OperatorStatus::Active;
    }

    bool is_disabled(OperatorStatus status) noexcept
    {
        return status == OperatorStatus::Disabled;
    }

    bool is_locked(OperatorStatus status) noexcept
    {
        return status == OperatorStatus::Locked;
    }

    bool is_expired(OperatorStatus status) noexcept
    {
        return status == OperatorStatus::Expired;
    }
}