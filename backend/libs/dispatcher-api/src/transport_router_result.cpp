#include <dispatcher/api/transport_router_result.hpp>

#include <stdexcept>
#include <utility>

namespace dispatcher::api
{
    TransportRouterResult TransportRouterResult::handled(
        TransportResponse response,
        TransportEndpoint endpoint,
        std::string message
    )
    {
        return TransportRouterResult(
            TransportRouterStatus::Handled,
            std::move(response),
            std::move(endpoint),
            std::move(message),
            {},
            {},
            {}
        );
    }

    TransportRouterResult TransportRouterResult::failed(
        TransportRouterStatus status,
        TransportResponse response,
        std::string reason,
        std::string field,
        std::string value
    )
    {
        if (is_success(status))
        {
            status = TransportRouterStatus::HandlerFailed;
        }

        return TransportRouterResult(
            status,
            std::move(response),
            std::nullopt,
            {},
            std::move(reason),
            std::move(field),
            std::move(value)
        );
    }

    bool TransportRouterResult::ok() const noexcept
    {
        return is_success(status_)
            && response_.has_value()
            && endpoint_.has_value();
    }

    bool TransportRouterResult::failed() const noexcept
    {
        return !ok();
    }

    TransportRouterStatus TransportRouterResult::status() const noexcept
    {
        return status_;
    }

    bool TransportRouterResult::has_response() const noexcept
    {
        return response_.has_value();
    }

    const TransportResponse& TransportRouterResult::response() const
    {
        if (!response_.has_value())
        {
            throw std::logic_error(
                "TransportRouterResult does not contain a response"
            );
        }

        return response_.value();
    }

    bool TransportRouterResult::has_endpoint() const noexcept
    {
        return endpoint_.has_value();
    }

    const TransportEndpoint& TransportRouterResult::endpoint() const
    {
        if (!endpoint_.has_value())
        {
            throw std::logic_error(
                "TransportRouterResult does not contain an endpoint"
            );
        }

        return endpoint_.value();
    }

    const std::string& TransportRouterResult::message() const noexcept
    {
        return message_;
    }

    const std::string& TransportRouterResult::reason() const noexcept
    {
        return reason_;
    }

    const std::string& TransportRouterResult::field() const noexcept
    {
        return field_;
    }

    const std::string& TransportRouterResult::value() const noexcept
    {
        return value_;
    }

    bool TransportRouterResult::has_message() const noexcept
    {
        return !message_.empty();
    }

    bool TransportRouterResult::has_reason() const noexcept
    {
        return !reason_.empty();
    }

    bool TransportRouterResult::has_field() const noexcept
    {
        return !field_.empty();
    }

    bool TransportRouterResult::has_value() const noexcept
    {
        return !value_.empty();
    }

    TransportRouterResult::TransportRouterResult(
        TransportRouterStatus status,
        std::optional<TransportResponse> response,
        std::optional<TransportEndpoint> endpoint,
        std::string message,
        std::string reason,
        std::string field,
        std::string value
    )
        : status_(status)
        , response_(std::move(response))
        , endpoint_(std::move(endpoint))
        , message_(std::move(message))
        , reason_(std::move(reason))
        , field_(std::move(field))
        , value_(std::move(value))
    {
    }
}