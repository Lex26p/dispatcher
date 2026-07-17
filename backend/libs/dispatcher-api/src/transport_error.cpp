#include <dispatcher/api/transport_error.hpp>

#include <utility>

namespace dispatcher::api
{
    TransportError::TransportError(
        std::string code,
        std::string message,
        std::string field,
        std::string detail
    )
        : code_(std::move(code))
        , message_(std::move(message))
        , field_(std::move(field))
        , detail_(std::move(detail))
    {
    }

    TransportError TransportError::none()
    {
        return TransportError{};
    }

    TransportError TransportError::invalid_request(
        std::string message,
        std::string field,
        std::string detail
    )
    {
        return TransportError(
            "invalid_request",
            std::move(message),
            std::move(field),
            std::move(detail)
        );
    }

    TransportError TransportError::unauthorized(
        std::string message,
        std::string detail
    )
    {
        return TransportError(
            "unauthorized",
            std::move(message),
            {},
            std::move(detail)
        );
    }

    TransportError TransportError::forbidden(
        std::string message,
        std::string detail
    )
    {
        return TransportError(
            "forbidden",
            std::move(message),
            {},
            std::move(detail)
        );
    }

    TransportError TransportError::not_found(
        std::string message,
        std::string detail
    )
    {
        return TransportError(
            "not_found",
            std::move(message),
            {},
            std::move(detail)
        );
    }

    TransportError TransportError::conflict(
        std::string message,
        std::string detail
    )
    {
        return TransportError(
            "conflict",
            std::move(message),
            {},
            std::move(detail)
        );
    }

    TransportError TransportError::internal_error(
        std::string message,
        std::string detail
    )
    {
        return TransportError(
            "internal_error",
            std::move(message),
            {},
            std::move(detail)
        );
    }

    const std::string& TransportError::code() const noexcept
    {
        return code_;
    }

    const std::string& TransportError::message() const noexcept
    {
        return message_;
    }

    const std::string& TransportError::field() const noexcept
    {
        return field_;
    }

    const std::string& TransportError::detail() const noexcept
    {
        return detail_;
    }

    bool TransportError::empty() const noexcept
    {
        return code_.empty()
            && message_.empty()
            && field_.empty()
            && detail_.empty();
    }

    bool TransportError::has_code() const noexcept
    {
        return !code_.empty();
    }

    bool TransportError::has_message() const noexcept
    {
        return !message_.empty();
    }

    bool TransportError::has_field() const noexcept
    {
        return !field_.empty();
    }

    bool TransportError::has_detail() const noexcept
    {
        return !detail_.empty();
    }
}