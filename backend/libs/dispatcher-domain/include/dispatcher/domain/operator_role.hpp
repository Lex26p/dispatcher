#pragma once

namespace dispatcher::domain
{
    enum class OperatorRole
    {
        Viewer,
        Operator,
        Supervisor,
        Engineer,
        Administrator,
        Service
    };

    [[nodiscard]] const char* to_string(
        OperatorRole role
    ) noexcept;

    [[nodiscard]] bool can_view_runtime(
        OperatorRole role
    ) noexcept;

    [[nodiscard]] bool can_acknowledge_alarms(
        OperatorRole role
    ) noexcept;

    [[nodiscard]] bool can_shelve_alarms(
        OperatorRole role
    ) noexcept;

    [[nodiscard]] bool can_manage_configuration(
        OperatorRole role
    ) noexcept;

    [[nodiscard]] bool can_administer_users(
        OperatorRole role
    ) noexcept;

    [[nodiscard]] bool is_privileged_role(
        OperatorRole role
    ) noexcept;
}