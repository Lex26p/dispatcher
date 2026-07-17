#include <dispatcher/domain/operator_permission.hpp>

namespace dispatcher::domain
{
    const char* to_string(OperatorPermission permission) noexcept
    {
        switch (permission)
        {
        case OperatorPermission::ViewRuntime:
            return "view_runtime";

        case OperatorPermission::AcknowledgeAlarms:
            return "acknowledge_alarms";

        case OperatorPermission::ShelveAlarms:
            return "shelve_alarms";

        case OperatorPermission::ManageConfiguration:
            return "manage_configuration";

        case OperatorPermission::ImportConfiguration:
            return "import_configuration";

        case OperatorPermission::ExportConfiguration:
            return "export_configuration";

        case OperatorPermission::ManageOperators:
            return "manage_operators";

        case OperatorPermission::ViewAuditLog:
            return "view_audit_log";

        case OperatorPermission::ServiceAccess:
            return "service_access";
        }

        return "view_runtime";
    }

    bool is_alarm_permission(OperatorPermission permission) noexcept
    {
        switch (permission)
        {
        case OperatorPermission::AcknowledgeAlarms:
        case OperatorPermission::ShelveAlarms:
            return true;

        case OperatorPermission::ViewRuntime:
        case OperatorPermission::ManageConfiguration:
        case OperatorPermission::ImportConfiguration:
        case OperatorPermission::ExportConfiguration:
        case OperatorPermission::ManageOperators:
        case OperatorPermission::ViewAuditLog:
        case OperatorPermission::ServiceAccess:
            return false;
        }

        return false;
    }

    bool is_configuration_permission(OperatorPermission permission) noexcept
    {
        switch (permission)
        {
        case OperatorPermission::ManageConfiguration:
        case OperatorPermission::ImportConfiguration:
        case OperatorPermission::ExportConfiguration:
            return true;

        case OperatorPermission::ViewRuntime:
        case OperatorPermission::AcknowledgeAlarms:
        case OperatorPermission::ShelveAlarms:
        case OperatorPermission::ManageOperators:
        case OperatorPermission::ViewAuditLog:
        case OperatorPermission::ServiceAccess:
            return false;
        }

        return false;
    }

    bool is_administrative_permission(OperatorPermission permission) noexcept
    {
        switch (permission)
        {
        case OperatorPermission::ManageOperators:
        case OperatorPermission::ViewAuditLog:
        case OperatorPermission::ServiceAccess:
            return true;

        case OperatorPermission::ViewRuntime:
        case OperatorPermission::AcknowledgeAlarms:
        case OperatorPermission::ShelveAlarms:
        case OperatorPermission::ManageConfiguration:
        case OperatorPermission::ImportConfiguration:
        case OperatorPermission::ExportConfiguration:
            return false;
        }

        return false;
    }

    bool is_permission_allowed_for_role(
        OperatorRole role,
        OperatorPermission permission
    ) noexcept
    {
        switch (permission)
        {
        case OperatorPermission::ViewRuntime:
            return can_view_runtime(role);

        case OperatorPermission::AcknowledgeAlarms:
            return can_acknowledge_alarms(role);

        case OperatorPermission::ShelveAlarms:
            return can_shelve_alarms(role);

        case OperatorPermission::ManageConfiguration:
        case OperatorPermission::ImportConfiguration:
            return can_manage_configuration(role);

        case OperatorPermission::ExportConfiguration:
            return is_privileged_role(role);

        case OperatorPermission::ManageOperators:
            return can_administer_users(role);

        case OperatorPermission::ViewAuditLog:
            return is_privileged_role(role);

        case OperatorPermission::ServiceAccess:
            return role == OperatorRole::Service
                || role == OperatorRole::Administrator;
        }

        return false;
    }
}