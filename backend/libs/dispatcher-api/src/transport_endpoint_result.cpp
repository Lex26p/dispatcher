#include <dispatcher/api/transport_endpoint_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::api
{
    TransportEndpointResult TransportEndpointResult::success(
        TransportEndpointStatus status,
        TransportEndpoint endpoint,
        std::string message
    )
    {
        if (is_failure(status))
        {
            status = TransportEndpointStatus::Found;
        }

        return TransportEndpointResult(
            status,
            std::move(endpoint),
            std::move(message),
            {},
            {}
        );
    }

    TransportEndpointResult TransportEndpointResult::failure(
        TransportEndpointStatus status,
        std::string message,
        std::string field,
        std::string value
    )
    {
        if (is_success(status))
        {
            status = TransportEndpointStatus::InvalidEndpoint;
        }

        return TransportEndpointResult(
            status,
            std::nullopt,
            std::move(message),
            std::move(field),
            std::move(value)
        );
    }

    bool TransportEndpointResult::ok() const noexcept
    {
        return is_success(status_)
            && endpoint_.has_value();
    }

    bool TransportEndpointResult::failed() const noexcept
    {
        return !ok();
    }

    TransportEndpointStatus TransportEndpointResult::status() const noexcept
    {
        return status_;
    }

    bool TransportEndpointResult::has_endpoint() const noexcept
    {
        return endpoint_.has_value();
    }

    const TransportEndpoint& TransportEndpointResult::endpoint() const
    {
        if (!endpoint_.has_value())
        {
            throw std::logic_error(
                "TransportEndpointResult does not contain an endpoint"
            );
        }

        return endpoint_.value();
    }

    const std::string& TransportEndpointResult::message() const noexcept
    {
        return message_;
    }

    const std::string& TransportEndpointResult::field() const noexcept
    {
        return field_;
    }

    const std::string& TransportEndpointResult::value() const noexcept
    {
        return value_;
    }

    bool TransportEndpointResult::has_message() const noexcept
    {
        return !message_.empty();
    }

    bool TransportEndpointResult::has_field() const noexcept
    {
        return !field_.empty();
    }

    bool TransportEndpointResult::has_value() const noexcept
    {
        return !value_.empty();
    }

    TransportEndpointResult::TransportEndpointResult(
        TransportEndpointStatus status,
        std::optional<TransportEndpoint> endpoint,
        std::string message,
        std::string field,
        std::string value
    )
        : status_(status)
        , endpoint_(std::move(endpoint))
        , message_(std::move(message))
        , field_(std::move(field))
        , value_(std::move(value))
    {
    }
}