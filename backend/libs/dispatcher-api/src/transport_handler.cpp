#include <dispatcher/api/transport_handler.hpp>

#include <utility>

namespace dispatcher::api
{
    TransportHandler::TransportHandler(
        TransportEndpoint endpoint,
        HandlerFunction handler,
        bool enabled
    )
        : endpoint_(std::move(endpoint))
        , handler_(std::move(handler))
        , enabled_(enabled)
    {
    }

    const TransportEndpoint& TransportHandler::endpoint() const noexcept
    {
        return endpoint_;
    }

    bool TransportHandler::enabled() const noexcept
    {
        return enabled_;
    }

    bool TransportHandler::disabled() const noexcept
    {
        return !enabled_;
    }

    bool TransportHandler::has_handler() const noexcept
    {
        return static_cast<bool>(handler_);
    }

    bool TransportHandler::valid() const noexcept
    {
        return endpoint_.valid()
            && has_handler();
    }

    bool TransportHandler::matches(
        TransportMethod method,
        const std::string& path
    ) const noexcept
    {
        return endpoint_.matches(
            method,
            path
        );
    }

    bool TransportHandler::compatible_with(
        TransportProtocol protocol
    ) const noexcept
    {
        return endpoint_.compatible_with(protocol);
    }

    TransportResponse TransportHandler::handle(
        const TransportRequest& request
    ) const
    {
        if (!enabled_)
        {
            return TransportResponse::failure(
                TransportStatus::Unavailable,
                TransportError::internal_error(
                    "transport handler is disabled"
                )
            );
        }

        if (!valid())
        {
            return TransportResponse::failure(
                TransportStatus::InternalError,
                TransportError::internal_error(
                    "transport handler is invalid"
                )
            );
        }

        return handler_(request);
    }
}