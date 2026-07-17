#include <dispatcher/api/transport_endpoint_status.hpp>

namespace dispatcher::api
{
    const char* to_string(TransportEndpointStatus status) noexcept
    {
        switch (status)
        {
        case TransportEndpointStatus::Registered:
            return "registered";

        case TransportEndpointStatus::Removed:
            return "removed";

        case TransportEndpointStatus::Found:
            return "found";

        case TransportEndpointStatus::NotFound:
            return "not_found";

        case TransportEndpointStatus::DuplicateEndpoint:
            return "duplicate_endpoint";

        case TransportEndpointStatus::InvalidEndpoint:
            return "invalid_endpoint";
        }

        return "invalid_endpoint";
    }

    bool is_success(TransportEndpointStatus status) noexcept
    {
        switch (status)
        {
        case TransportEndpointStatus::Registered:
        case TransportEndpointStatus::Removed:
        case TransportEndpointStatus::Found:
            return true;

        case TransportEndpointStatus::NotFound:
        case TransportEndpointStatus::DuplicateEndpoint:
        case TransportEndpointStatus::InvalidEndpoint:
            return false;
        }

        return false;
    }

    bool is_failure(TransportEndpointStatus status) noexcept
    {
        return !is_success(status);
    }
}