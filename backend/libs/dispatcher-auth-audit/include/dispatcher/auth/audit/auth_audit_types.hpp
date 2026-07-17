#pragma once

#include <chrono>
#include <map>
#include <string>

namespace dispatcher::auth::audit
{
    enum class AuthAuditActorType
    {
        system,
        operator_user,
        service,
        anonymous
    };

    enum class AuthAuditAction
    {
        unknown,
        runtime_read,
        runtime_control,
        alarm_acknowledge,
        alarm_shelve,
        alarm_unshelve,
        configuration_import,
        configuration_export,
        notification_send,
        authorization_check,
        login,
        logout
    };

    enum class AuthAuditOutcome
    {
        success,
        denied,
        failed
    };

    enum class AuthAuditSeverity
    {
        info,
        warning,
        critical
    };

    enum class AuthAuditRecordStatus
    {
        accepted,
        failed,
        skipped
    };

    struct AuthAuditActor
    {
        std::string actor_id{};
        std::string display_name{};

        AuthAuditActorType actor_type{
            AuthAuditActorType::anonymous
        };
    };

    struct AuthAuditResource
    {
        std::string resource_type{};
        std::string resource_id{};
        std::string display_name{};
    };

    struct AuthAuditEvent
    {
        std::string event_id{};
        std::string correlation_id{};
        std::string source{};

        AuthAuditActor actor{};

        AuthAuditAction action{
            AuthAuditAction::unknown
        };

        AuthAuditOutcome outcome{
            AuthAuditOutcome::success
        };

        AuthAuditSeverity severity{
            AuthAuditSeverity::info
        };

        AuthAuditResource resource{};

        std::string reason{};
        std::string diagnostic_message{};

        std::map<std::string, std::string> attributes{};

        std::chrono::system_clock::time_point occurred_at{
            std::chrono::system_clock::now()
        };
    };

    struct AuthAuditRecordResult
    {
        AuthAuditRecordStatus status{
            AuthAuditRecordStatus::accepted
        };

        std::string provider_record_id{};
        std::string error_message{};
        std::string diagnostic_message{};

        std::chrono::system_clock::time_point completed_at{
            std::chrono::system_clock::now()
        };

        [[nodiscard]] static AuthAuditRecordResult accepted(
            std::string provider_record_id = {},
            std::string diagnostic_message = {}
        )
        {
            AuthAuditRecordResult result;

            result.status =
                AuthAuditRecordStatus::accepted;

            result.provider_record_id =
                std::move(
                    provider_record_id
                );

            result.diagnostic_message =
                std::move(
                    diagnostic_message
                );

            result.completed_at =
                std::chrono::system_clock::now();

            return result;
        }

        [[nodiscard]] static AuthAuditRecordResult failed(
            std::string error_message,
            std::string diagnostic_message = {}
        )
        {
            AuthAuditRecordResult result;

            result.status =
                AuthAuditRecordStatus::failed;

            result.error_message =
                std::move(
                    error_message
                );

            result.diagnostic_message =
                std::move(
                    diagnostic_message
                );

            result.completed_at =
                std::chrono::system_clock::now();

            return result;
        }

        [[nodiscard]] static AuthAuditRecordResult skipped(
            std::string diagnostic_message
        )
        {
            AuthAuditRecordResult result;

            result.status =
                AuthAuditRecordStatus::skipped;

            result.diagnostic_message =
                std::move(
                    diagnostic_message
                );

            result.completed_at =
                std::chrono::system_clock::now();

            return result;
        }

        [[nodiscard]] bool success() const noexcept
        {
            return status == AuthAuditRecordStatus::accepted;
        }

        [[nodiscard]] bool failure() const noexcept
        {
            return status == AuthAuditRecordStatus::failed;
        }
    };
}