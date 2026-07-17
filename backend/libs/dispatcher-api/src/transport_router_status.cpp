#include <dispatcher/api/transport_router_status.hpp>

namespace dispatcher::api
{
    const char* to_string(TransportRouterStatus status) noexcept
    {
        switch (status)
        {
        case TransportRouterStatus::Handled:
            return "handled";

        case TransportRouterStatus::InvalidRequest:
            return "invalid_request";

        case TransportRouterStatus::EndpointNotFound:
            return "endpoint_not_found";

        case TransportRouterStatus::HandlerNotFound:
            return "handler_not_found";

        case TransportRouterStatus::HandlerFailed:
            return "handler_failed";
        }

        return "handler_failed";
    }

    bool is_success(TransportRouterStatus status) noexcept
    {
        return status == TransportRouterStatus::Handled;
    }

    bool is_failure(TransportRouterStatus status) noexcept
    {
        return !is_success(status);
    }
}