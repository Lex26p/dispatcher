#include <dispatcher/domain/operator_identity.hpp>

#include <utility>

namespace dispatcher::domain
{
    OperatorIdentity::OperatorIdentity(
        OperatorId operator_id,
        std::string username,
        OperatorRole role,
        OperatorStatus status,
        std::string display_name,
        std::string email
    )
        : operator_id_(std::move(operator_id))
        , username_(std::move(username))
        , role_(role)
        , status_(status)
        , display_name_(std::move(display_name))
        , email_(std::move(email))
    {
    }

    const OperatorId& OperatorIdentity::operator_id() const noexcept
    {
        return operator_id_;
    }

    const std::string& OperatorIdentity::username() const noexcept
    {
        return username_;
    }

    OperatorRole OperatorIdentity::role() const noexcept
    {
        return role_;
    }

    OperatorStatus OperatorIdentity::status() const noexcept
    {
        return status_;
    }

    const std::string& OperatorIdentity::display_name() const noexcept
    {
        return display_name_;
    }

    const std::string& OperatorIdentity::email() const noexcept
    {
        return email_;
    }

    bool OperatorIdentity::has_display_name() const noexcept
    {
        return !display_name_.empty();
    }

    bool OperatorIdentity::has_email() const noexcept
    {
        return !email_.empty();
    }

    bool OperatorIdentity::active() const noexcept
    {
        return is_active(status_);
    }

    bool OperatorIdentity::enabled() const noexcept
    {
        return can_sign_in();
    }

    bool OperatorIdentity::can_sign_in() const noexcept
    {
        return dispatcher::domain::can_sign_in(status_);
    }

    bool OperatorIdentity::can_view_runtime() const noexcept
    {
        return active()
            && dispatcher::domain::can_view_runtime(role_);
    }

    bool OperatorIdentity::can_acknowledge_alarms() const noexcept
    {
        return active()
            && dispatcher::domain::can_acknowledge_alarms(role_);
    }

    bool OperatorIdentity::can_shelve_alarms() const noexcept
    {
        return active()
            && dispatcher::domain::can_shelve_alarms(role_);
    }

    bool OperatorIdentity::can_manage_configuration() const noexcept
    {
        return active()
            && dispatcher::domain::can_manage_configuration(role_);
    }

    bool OperatorIdentity::can_administer_users() const noexcept
    {
        return active()
            && dispatcher::domain::can_administer_users(role_);
    }

    bool OperatorIdentity::privileged() const noexcept
    {
        return active()
            && dispatcher::domain::is_privileged_role(role_);
    }
}