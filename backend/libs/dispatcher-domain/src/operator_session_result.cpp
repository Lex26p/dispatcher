#include <dispatcher/domain/operator_session_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::domain
{
    OperatorSessionResult OperatorSessionResult::success(
        OperatorSession session,
        std::string message
    )
    {
        return OperatorSessionResult(
            session.status(),
            std::move(session),
            std::move(message),
            {},
            {}
        );
    }

    OperatorSessionResult OperatorSessionResult::failure(
        OperatorSessionStatus status,
        std::string message,
        std::string field,
        std::string value
    )
    {
        if (!is_failure(status))
        {
            status = OperatorSessionStatus::InvalidSession;
        }

        return OperatorSessionResult(
            status,
            std::nullopt,
            std::move(message),
            std::move(field),
            std::move(value)
        );
    }

    bool OperatorSessionResult::ok() const noexcept
    {
        return !is_failure(status_)
            && session_.has_value();
    }

    bool OperatorSessionResult::failed() const noexcept
    {
        return !ok();
    }

    OperatorSessionStatus OperatorSessionResult::status() const noexcept
    {
        return status_;
    }

    bool OperatorSessionResult::has_session() const noexcept
    {
        return session_.has_value();
    }

    const OperatorSession& OperatorSessionResult::session() const
    {
        if (!session_.has_value())
        {
            throw std::logic_error(
                "OperatorSessionResult does not contain a session"
            );
        }

        return session_.value();
    }

    const std::string& OperatorSessionResult::message() const noexcept
    {
        return message_;
    }

    const std::string& OperatorSessionResult::field() const noexcept
    {
        return field_;
    }

    const std::string& OperatorSessionResult::value() const noexcept
    {
        return value_;
    }

    bool OperatorSessionResult::has_message() const noexcept
    {
        return !message_.empty();
    }

    bool OperatorSessionResult::has_field() const noexcept
    {
        return !field_.empty();
    }

    bool OperatorSessionResult::has_value() const noexcept
    {
        return !value_.empty();
    }

    OperatorSessionResult::OperatorSessionResult(
        OperatorSessionStatus status,
        std::optional<OperatorSession> session,
        std::string message,
        std::string field,
        std::string value
    )
        : status_(status)
        , session_(std::move(session))
        , message_(std::move(message))
        , field_(std::move(field))
        , value_(std::move(value))
    {
    }
}