#include <dispatcher/api/transport_router.hpp>

#include <exception>
#include <utility>

namespace dispatcher::api
{
    TransportEndpointResult TransportRouter::add_handler(
        TransportHandler handler
    )
    {
        if (!handler.valid())
        {
            return TransportEndpointResult::failure(
                TransportEndpointStatus::InvalidEndpoint,
                "transport handler is invalid",
                "handler",
                handler.endpoint().key()
            );
        }

        auto endpoint = handler.endpoint();

        auto result =
            endpoint_registry_.add(endpoint);

        if (result.failed())
        {
            return result;
        }

        handlers_by_key_.emplace(
            endpoint.key(),
            std::move(handler)
        );

        return result;
    }

    TransportEndpointResult TransportRouter::remove_handler(
        TransportMethod method,
        const std::string& path
    )
    {
        const auto endpoint_key =
            TransportEndpoint::make_key(
                method,
                path
            );

        auto result =
            endpoint_registry_.remove(
                method,
                path
            );

        if (result.ok())
        {
            handlers_by_key_.erase(endpoint_key);
        }

        return result;
    }

    TransportRouterResult TransportRouter::dispatch(
        const TransportRequest& request
    ) const
    {
        if (!request.valid())
        {
            return TransportRouterResult::failed(
                TransportRouterStatus::InvalidRequest,
                TransportResponse::failure(
                    TransportStatus::BadRequest,
                    TransportError::invalid_request(
                        "transport request is invalid",
                        "request"
                    )
                ),
                "transport request is invalid",
                "request",
                request.path()
            );
        }

        const auto endpoint_result =
            endpoint_registry_.resolve(
                request.method(),
                request.path()
            );

        if (endpoint_result.failed())
        {
            return TransportRouterResult::failed(
                TransportRouterStatus::EndpointNotFound,
                TransportResponse::failure(
                    TransportStatus::NotFound,
                    TransportError::not_found(
                        "transport endpoint not found",
                        endpoint_result.value()
                    )
                ),
                "transport endpoint not found",
                "endpoint",
                endpoint_result.value()
            );
        }

        const auto& endpoint = endpoint_result.endpoint();

        if (!endpoint.compatible_with(request.protocol()))
        {
            return TransportRouterResult::failed(
                TransportRouterStatus::InvalidRequest,
                TransportResponse::failure(
                    TransportStatus::BadRequest,
                    TransportError::invalid_request(
                        "transport endpoint is not compatible with request protocol",
                        "protocol",
                        to_string(request.protocol())
                    )
                ),
                "transport endpoint is not compatible with request protocol",
                "protocol",
                to_string(request.protocol())
            );
        }

        const auto handler_iterator =
            handlers_by_key_.find(endpoint.key());

        if (handler_iterator == handlers_by_key_.end())
        {
            return TransportRouterResult::failed(
                TransportRouterStatus::HandlerNotFound,
                TransportResponse::failure(
                    TransportStatus::Unavailable,
                    TransportError::internal_error(
                        "transport handler not found",
                        endpoint.key()
                    )
                ),
                "transport handler not found",
                "handler",
                endpoint.key()
            );
        }

        try
        {
            auto response =
                handler_iterator->second.handle(request);

            return TransportRouterResult::handled(
                std::move(response),
                endpoint,
                "transport request handled"
            );
        }
        catch (const std::exception& exception)
        {
            return TransportRouterResult::failed(
                TransportRouterStatus::HandlerFailed,
                TransportResponse::failure(
                    TransportStatus::InternalError,
                    TransportError::internal_error(
                        "transport handler threw an exception",
                        exception.what()
                    )
                ),
                "transport handler threw an exception",
                "handler",
                endpoint.key()
            );
        }
        catch (...)
        {
            return TransportRouterResult::failed(
                TransportRouterStatus::HandlerFailed,
                TransportResponse::failure(
                    TransportStatus::InternalError,
                    TransportError::internal_error(
                        "transport handler threw an unknown exception"
                    )
                ),
                "transport handler threw an unknown exception",
                "handler",
                endpoint.key()
            );
        }
    }

    std::optional<TransportHandler> TransportRouter::find_handler(
        TransportMethod method,
        const std::string& path
    ) const
    {
        const auto endpoint_key =
            TransportEndpoint::make_key(
                method,
                path
            );

        const auto iterator =
            handlers_by_key_.find(endpoint_key);

        if (iterator == handlers_by_key_.end())
        {
            return std::nullopt;
        }

        return iterator->second;
    }

    bool TransportRouter::contains_handler(
        TransportMethod method,
        const std::string& path
    ) const
    {
        return handlers_by_key_.contains(
            TransportEndpoint::make_key(
                method,
                path
            )
        );
    }

    const TransportEndpointRegistry& TransportRouter::endpoint_registry()
        const noexcept
    {
        return endpoint_registry_;
    }

    std::vector<TransportEndpoint> TransportRouter::endpoints() const
    {
        return endpoint_registry_.endpoints();
    }

    std::size_t TransportRouter::handler_count() const noexcept
    {
        return handlers_by_key_.size();
    }

    bool TransportRouter::empty() const noexcept
    {
        return handlers_by_key_.empty();
    }

    void TransportRouter::clear() noexcept
    {
        endpoint_registry_.clear();
        handlers_by_key_.clear();
    }
}