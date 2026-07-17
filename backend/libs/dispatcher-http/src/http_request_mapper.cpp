#include <dispatcher/http/http_request_mapper.hpp>

#include <dispatcher/api/transport_protocol.hpp>
#include <dispatcher/api/transport_request_context.hpp>

#include <string>
#include <utility>

namespace dispatcher::http
{
    dispatcher::api::TransportRequest HttpRequestMapper::to_transport_request(
        const BeastRequest& request,
        std::string remote_address
    )
    {
        return dispatcher::api::TransportRequest(
            dispatcher::api::TransportRequestContext(
                dispatcher::api::TransportProtocol::Http,
                method_string(request),
                path_from_target(request),
                correlation_id(request),
                {},
                std::move(remote_address)
            )
        );
    }

    std::string HttpRequestMapper::method_string(
        const BeastRequest& request
    )
    {
        return std::string(request.method_string());
    }

    std::string HttpRequestMapper::path_from_target(
        const BeastRequest& request
    )
    {
        auto target = std::string(request.target());

        if (target.empty())
        {
            return "/";
        }

        const auto query_position = target.find('?');

        if (query_position != std::string::npos)
        {
            target = target.substr(0, query_position);
        }

        if (target.empty())
        {
            return "/";
        }

        return target;
    }

    std::string HttpRequestMapper::correlation_id(
        const BeastRequest& request
    )
    {
        auto value = header_value(
            request,
            "x-correlation-id"
        );

        if (!value.empty())
        {
            return value;
        }

        value = header_value(
            request,
            "x-request-id"
        );

        if (!value.empty())
        {
            return value;
        }

        return "http-request";
    }

    std::string HttpRequestMapper::header_value(
        const BeastRequest& request,
        const std::string& header_name
    )
    {
        const auto iterator = request.find(header_name);

        if (iterator == request.end())
        {
            return {};
        }

        return std::string(iterator->value());
    }
}