#pragma once

#include <dispatcher/domain/id_types.hpp>
#include <dispatcher/domain/operator_role.hpp>
#include <dispatcher/domain/operator_status.hpp>

#include <string>

namespace dispatcher::domain
{
    class OperatorIdentity
    {
    public:
        OperatorIdentity(
            OperatorId operator_id,
            std::string username,
            OperatorRole role,
            OperatorStatus status = OperatorStatus::Active,
            std::string display_name = {},
            std::string email = {}
        );

        [[nodiscard]] const OperatorId& operator_id() const noexcept;

        [[nodiscard]] const std::string& username() const noexcept;

        [[nodiscard]] OperatorRole role() const noexcept;

        [[nodiscard]] OperatorStatus status() const noexcept;

        [[nodiscard]] const std::string& display_name() const noexcept;

        [[nodiscard]] const std::string& email() const noexcept;

        [[nodiscard]] bool has_display_name() const noexcept;

        [[nodiscard]] bool has_email() const noexcept;

        [[nodiscard]] bool active() const noexcept;

        [[nodiscard]] bool enabled() const noexcept;

        [[nodiscard]] bool can_sign_in() const noexcept;

        [[nodiscard]] bool can_view_runtime() const noexcept;

        [[nodiscard]] bool can_acknowledge_alarms() const noexcept;

        [[nodiscard]] bool can_shelve_alarms() const noexcept;

        [[nodiscard]] bool can_manage_configuration() const noexcept;

        [[nodiscard]] bool can_administer_users() const noexcept;

        [[nodiscard]] bool privileged() const noexcept;

    private:
        OperatorId operator_id_;
        std::string username_;
        OperatorRole role_{ OperatorRole::Viewer };
        OperatorStatus status_{ OperatorStatus::Disabled };
        std::string display_name_;
        std::string email_;
    };
}