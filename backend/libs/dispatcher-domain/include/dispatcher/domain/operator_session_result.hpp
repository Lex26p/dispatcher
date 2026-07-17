#pragma once

#include <dispatcher/domain/operator_session.hpp>
#include <dispatcher/domain/operator_session_status.hpp>

#include <optional>
#include <string>

namespace dispatcher::domain
{
    class OperatorSessionResult
    {
    public:
        [[nodiscard]] static OperatorSessionResult success(
            OperatorSession session,
            std::string message = {}
        );

        [[nodiscard]] static OperatorSessionResult failure(
            OperatorSessionStatus status,
            std::string message = {},
            std::string field = {},
            std::string value = {}
        );

        [[nodiscard]] bool ok() const noexcept;

        [[nodiscard]] bool failed() const noexcept;

        [[nodiscard]] OperatorSessionStatus status() const noexcept;

        [[nodiscard]] bool has_session() const noexcept;

        [[nodiscard]] const OperatorSession& session() const;

        [[nodiscard]] const std::string& message() const noexcept;

        [[nodiscard]] const std::string& field() const noexcept;

        [[nodiscard]] const std::string& value() const noexcept;

        [[nodiscard]] bool has_message() const noexcept;

        [[nodiscard]] bool has_field() const noexcept;

        [[nodiscard]] bool has_value() const noexcept;

    private:
        OperatorSessionResult(
            OperatorSessionStatus status,
            std::optional<OperatorSession> session,
            std::string message,
            std::string field,
            std::string value
        );

        OperatorSessionStatus status_{ OperatorSessionStatus::InvalidSession };
        std::optional<OperatorSession> session_;
        std::string message_;
        std::string field_;
        std::string value_;
    };
}