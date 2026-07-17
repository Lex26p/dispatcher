#include <dispatcher/domain/operator_role.hpp>

namespace dispatcher::domain
{
    const char* to_string(OperatorRole role) noexcept
    {
        switch (role)
        {
        case OperatorRole::Viewer:
            return "viewer";

        case OperatorRole::Operator:
            return "operator";

        case OperatorRole::Supervisor:
            return "supervisor";

        case OperatorRole::Engineer:
            return "engineer";

        case OperatorRole::Administrator:
            return "administrator";

        case OperatorRole::Service:
            return "service";
        }

        return "viewer";
    }

    bool can_view_runtime(OperatorRole role) noexcept
    {
        switch (role)
        {
        case OperatorRole::Viewer:
        case OperatorRole::Operator:
        case OperatorRole::Supervisor:
        case OperatorRole::Engineer:
        case OperatorRole::Administrator:
        case OperatorRole::Service:
            return true;
        }

        return false;
    }

    bool can_acknowledge_alarms(OperatorRole role) noexcept
    {
        switch (role)
        {
        case OperatorRole::Operator:
        case OperatorRole::Supervisor:
        case OperatorRole::Engineer:
        case OperatorRole::Administrator:
            return true;

        case OperatorRole::Viewer:
        case OperatorRole::Service:
            return false;
        }

        return false;
    }

    bool can_shelve_alarms(OperatorRole role) noexcept
    {
        switch (role)
        {
        case OperatorRole::Operator:
        case OperatorRole::Supervisor:
        case OperatorRole::Engineer:
        case OperatorRole::Administrator:
            return true;

        case OperatorRole::Viewer:
        case OperatorRole::Service:
            return false;
        }

        return false;
    }

    bool can_manage_configuration(OperatorRole role) noexcept
    {
        switch (role)
        {
        case OperatorRole::Engineer:
        case OperatorRole::Administrator:
            return true;

        case OperatorRole::Viewer:
        case OperatorRole::Operator:
        case OperatorRole::Supervisor:
        case OperatorRole::Service:
            return false;
        }

        return false;
    }

    bool can_administer_users(OperatorRole role) noexcept
    {
        return role == OperatorRole::Administrator;
    }

    bool is_privileged_role(OperatorRole role) noexcept
    {
        switch (role)
        {
        case OperatorRole::Supervisor:
        case OperatorRole::Engineer:
        case OperatorRole::Administrator:
            return true;

        case OperatorRole::Viewer:
        case OperatorRole::Operator:
        case OperatorRole::Service:
            return false;
        }

        return false;
    }
}