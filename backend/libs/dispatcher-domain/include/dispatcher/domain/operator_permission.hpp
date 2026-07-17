#pragma once

#include <dispatcher/domain/operator_role.hpp>

namespace dispatcher::domain
{
    enum class OperatorPermission
    {
        ViewRuntime,

        AcknowledgeAlarms,
        ShelveAlarms,

        ManageConfiguration,
        ImportConfiguration,
        ExportConfiguration,

        ManageOperators,
        ViewAuditLog,

        ServiceAccess
    };

    [[nodiscard]] const char* to_string(
        OperatorPermission permission
    ) noexcept;

    [[nodiscard]] bool is_alarm_permission(
        OperatorPermission permission
    ) noexcept;

    [[nodiscard]] bool is_configuration_permission(
        OperatorPermission permission
    ) noexcept;

    [[nodiscard]] bool is_administrative_permission(
        OperatorPermission permission
    ) noexcept;

    [[nodiscard]] bool is_permission_allowed_for_role(
        OperatorRole role,
        OperatorPermission permission
    ) noexcept;
}