#include <dispatcher/domain/operator_session_status.hpp>

namespace dispatcher::domain
{
    const char* to_string(OperatorSessionStatus status) noexcept
    {
        switch (status)
        {
        case OperatorSessionStatus::Active:
            return "active";

        case OperatorSessionStatus::SignedOut:
            return "signed_out";

        case OperatorSessionStatus::Expired:
            return "expired";

        case OperatorSessionStatus::NotFound:
            return "not_found";

        case OperatorSessionStatus::InvalidSession:
            return "invalid_session";
        }

        return "invalid_session";
    }

    bool is_active(OperatorSessionStatus status) noexcept
    {
        return status == OperatorSessionStatus::Active;
    }

    bool is_terminal(OperatorSessionStatus status) noexcept
    {
        switch (status)
        {
        case OperatorSessionStatus::SignedOut:
        case OperatorSessionStatus::Expired:
            return true;

        case OperatorSessionStatus::Active:
        case OperatorSessionStatus::NotFound:
        case OperatorSessionStatus::InvalidSession:
            return false;
        }

        return false;
    }

    bool is_failure(OperatorSessionStatus status) noexcept
    {
        switch (status)
        {
        case OperatorSessionStatus::NotFound:
        case OperatorSessionStatus::InvalidSession:
            return true;

        case OperatorSessionStatus::Active:
        case OperatorSessionStatus::SignedOut:
        case OperatorSessionStatus::Expired:
            return false;
        }

        return true;
    }
}